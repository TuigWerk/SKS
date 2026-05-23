// SKS web flasher — STK500v1 over Web Serial.
// Default target: ATmega4809 + Optiboot (MegaCoreX). Also supports classic Arduino Nano.

const PAGE_SIZE = 128;          // Both targets use 128-byte pages

const TARGETS = {
  SKS: {
    label: "SKS (ATmega4809)",
    firmware: "firmware/inoSKS.hex",
    baud: 115200,
    flashSize: 48 * 1024,        // ATmega4809: 48 KB flash
    // Single DTR pulse is enough for the SKS USB-serial bridge
    reset: { dtr: true, rts: false, holdMs: 100, settleMs: 100 },
  },
};

// STK500v1 constants
const STK = {
  OK: 0x10,
  INSYNC: 0x14,
  CRC_EOP: 0x20,
  GET_SYNC: 0x30,
  GET_PARAMETER: 0x41,
  SET_DEVICE: 0x42,
  ENTER_PROGMODE: 0x50,
  LEAVE_PROGMODE: 0x51,
  LOAD_ADDRESS: 0x55,
  PROG_PAGE: 0x64,
  READ_PAGE: 0x74,
  READ_SIGN: 0x75,
};

// ---------- UI helpers ----------
const $ = (id) => document.getElementById(id);
const logEl = () => $("log");
const setStatus = (text, kind = "") => {
  const s = $("status");
  s.textContent = text;
  s.className = kind;
};
const setProgress = (pct) => { $("bar").style.width = `${Math.max(0, Math.min(100, pct))}%`; };
const log = (msg) => {
  const t = new Date().toLocaleTimeString();
  logEl().textContent += `[${t}] ${msg}\n`;
  logEl().scrollTop = logEl().scrollHeight;
};
const wireLog = (dir, bytes) => {
  if (!$("wireLogChk")?.checked) return;
  const hex = Array.from(bytes).map((b) => b.toString(16).padStart(2, "0")).join(" ");
  // Truncate very long lines so a 128-byte page write doesn't drown the log
  const shown = hex.length > 200 ? hex.slice(0, 200) + ` … (${bytes.length} bytes)` : hex;
  log(`${dir} ${shown}`);
};

// ---------- Intel HEX parser ----------
// Returns Uint8Array of flash image padded with 0xFF up to highest written address (page-aligned).
function parseIntelHex(text, flashSize) {
  const lines = text.split(/\r?\n/).filter((l) => l.startsWith(":"));
  let extLinear = 0;
  let extSeg = 0;
  let highest = 0;
  const chunks = [];
  for (const line of lines) {
    if (line.length < 11) throw new Error("HEX-regel te kort");
    const bytes = [];
    for (let i = 1; i < line.length; i += 2) bytes.push(parseInt(line.substr(i, 2), 16));
    const len = bytes[0];
    const addr = (bytes[1] << 8) | bytes[2];
    const type = bytes[3];
    const data = bytes.slice(4, 4 + len);
    const checksum = bytes[4 + len];
    const sum = bytes.slice(0, 4 + len).reduce((a, b) => (a + b) & 0xff, 0);
    if (((~sum + 1) & 0xff) !== checksum) throw new Error("HEX-controlegetal klopt niet");
    if (type === 0x00) {
      const fullAddr = extLinear * 0x10000 + extSeg * 16 + addr;
      chunks.push({ addr: fullAddr, data });
      if (fullAddr + data.length > highest) highest = fullAddr + data.length;
    } else if (type === 0x01) {
      break; // EOF
    } else if (type === 0x02) {
      extSeg = (data[0] << 8) | data[1];
    } else if (type === 0x04) {
      extLinear = (data[0] << 8) | data[1];
    }
    // type 0x03 (start seg) and 0x05 (start linear) are ignored — irrelevant for AVR.
  }
  // Round up to page boundary
  const size = Math.ceil(highest / PAGE_SIZE) * PAGE_SIZE;
  if (size > flashSize) throw new Error(`Firmware (${size}B) overschrijdt beschikbaar flash (${flashSize}B)`);
  const image = new Uint8Array(size).fill(0xff);
  for (const c of chunks) image.set(c.data, c.addr);
  return image;
}

// ---------- Serial primitives ----------
class SerialIO {
  constructor(port) {
    this.port = port;
    this.reader = null;
    this.writer = null;
    this.buf = [];
  }
  async open(baud) {
    await this.port.open({ baudRate: baud, dataBits: 8, stopBits: 1, parity: "none", flowControl: "none" });
    this.reader = this.port.readable.getReader();
    this.writer = this.port.writable.getWriter();
    this._pump();
  }
  async _pump() {
    try {
      while (true) {
        const { value, done } = await this.reader.read();
        if (done) break;
        if (value && value.length) {
          wireLog("RX <", value);
          for (const b of value) this.buf.push(b);
        }
      }
    } catch (_) { /* port closed */ }
  }
  async close() {
    try { this.reader?.releaseLock(); } catch (_) {}
    try { this.writer?.releaseLock(); } catch (_) {}
    try { await this.port.close(); } catch (_) {}
  }
  async write(bytes) {
    const arr = new Uint8Array(bytes);
    wireLog("TX >", arr);
    await this.writer.write(arr);
  }
  async read(n, timeoutMs = 1500) {
    const deadline = performance.now() + timeoutMs;
    while (this.buf.length < n) {
      if (performance.now() > deadline) throw new Error(`Leestime-out (ontvangen ${this.buf.length}/${n})`);
      await new Promise((r) => setTimeout(r, 5));
    }
    return this.buf.splice(0, n);
  }
  drain() { this.buf.length = 0; }
  async setReset({ dtr, rts, holdMs, settleMs }) {
    // Assert configured lines (true = signal asserted = line LOW at the MCU)
    await this.port.setSignals({
      ...(dtr ? { dataTerminalReady: true } : {}),
      ...(rts ? { requestToSend: true } : {}),
    });
    await new Promise((r) => setTimeout(r, holdMs));
    // Release them
    await this.port.setSignals({
      ...(dtr ? { dataTerminalReady: false } : {}),
      ...(rts ? { requestToSend: false } : {}),
    });
    // Let the bootloader come up
    await new Promise((r) => setTimeout(r, settleMs));
  }
}

// ---------- STK500v1 ----------
async function cmd(io, payload, replyLen = 0, timeout = 3000) {
  io.drain();
  await io.write([...payload, STK.CRC_EOP]);
  const head = await io.read(1, timeout);
  if (head[0] !== STK.INSYNC) throw new Error(`Expected INSYNC, got 0x${head[0].toString(16)}`);
  const body = replyLen > 0 ? await io.read(replyLen, timeout) : [];
  const tail = await io.read(1, timeout);
  if (tail[0] !== STK.OK) throw new Error(`Expected OK, got 0x${tail[0].toString(16)}`);
  return body;
}

// Strict sync: send one GET_SYNC, wait for INSYNC+OK, retry on timeout or garbage.
// Total budget ~1.5 s, which is comfortably inside Optiboot's bootloader window
// after a fresh reset, but long enough to ride out a slow USB-serial chip.
async function sync(io) {
  const attempts = 10;
  const perAttemptMs = 150;
  for (let i = 0; i < attempts; i++) {
    io.drain();
    await io.write([STK.GET_SYNC, STK.CRC_EOP]);
    const deadline = performance.now() + perAttemptMs;
    while (performance.now() < deadline) {
      if (io.buf.length >= 2) {
        // Slide forward to the next INSYNC byte (skip stray noise like 0x00)
        while (io.buf.length && io.buf[0] !== STK.INSYNC) io.buf.shift();
        if (io.buf.length >= 2) {
          if (io.buf[1] === STK.OK) { io.buf.splice(0, 2); return; }
          // Wrong second byte — drop the bad INSYNC and keep looking
          io.buf.shift();
        }
      }
      await new Promise((r) => setTimeout(r, 5));
    }
  }
  throw new Error("Kon niet synchroniseren met de bootloader. Controleer de kabel, het bord en de resetlijn.");
}

// Tight request/response with no logging or computation between writes.
// Returns the (possibly empty) reply body.
async function txrx(io, payload, bodyLen = 0, timeoutMs = 2000) {
  await io.write([...payload, STK.CRC_EOP]);
  const head = await io.read(1, timeoutMs);
  if (head[0] !== STK.INSYNC) throw new Error(`Expected INSYNC, got 0x${head[0].toString(16)}`);
  const body = bodyLen > 0 ? await io.read(bodyLen, timeoutMs) : [];
  const tail = await io.read(1, timeoutMs);
  if (tail[0] !== STK.OK) throw new Error(`Expected OK, got 0x${tail[0].toString(16)}`);
  return body;
}

async function flash(io, image, verify, onProgress) {
  const pages = image.length / PAGE_SIZE;
  const imageSizeMsg = `Afbeelding: ${image.length} bytes (${pages} pagina's)`;

  // --- Critical section: no logging, no DOM writes between commands. ---
  io.drain();
  await txrx(io, [STK.GET_SYNC]);
  await txrx(io, [STK.ENTER_PROGMODE]);

  // Write loop. Progress updates happen but only after each PROG_PAGE — the
  // bootloader is idle by then because it just finished writing flash.
  for (let p = 0; p < pages; p++) {
    const byteAddr = p * PAGE_SIZE;
    const wordAddr = byteAddr >> 1; // STK500 uses word addresses
    await txrx(io, [STK.LOAD_ADDRESS, wordAddr & 0xff, (wordAddr >> 8) & 0xff]);
    const pageData = image.slice(byteAddr, byteAddr + PAGE_SIZE);
    // Page-program command can take ~5ms to write the flash, so give it 3s.
    await txrx(io, [STK.PROG_PAGE, 0x00, PAGE_SIZE, 0x46, ...pageData], 0, 3000);
    onProgress((p + 1) / pages * (verify ? 0.5 : 1.0));
  }

  if (verify) {
    for (let p = 0; p < pages; p++) {
      const byteAddr = p * PAGE_SIZE;
      const wordAddr = byteAddr >> 1;
      await txrx(io, [STK.LOAD_ADDRESS, wordAddr & 0xff, (wordAddr >> 8) & 0xff]);
      const got = await txrx(io, [STK.READ_PAGE, 0x00, PAGE_SIZE, 0x46], PAGE_SIZE);
      for (let i = 0; i < PAGE_SIZE; i++) {
        if (got[i] !== image[byteAddr + i]) {
          throw new Error(`Verificatie mislukt op 0x${(byteAddr + i).toString(16)}: geschreven 0x${image[byteAddr + i].toString(16)}, gelezen 0x${got[i].toString(16)}`);
        }
      }
      onProgress(0.5 + (p + 1) / pages * 0.5);
    }
  }

  await txrx(io, [STK.LEAVE_PROGMODE]);
  // --- End critical section. Logging is safe again. ---

  log(imageSizeMsg);
  log("Flashen + " + (verify ? "verificatie " : "") + "voltooid");
}

// ---------- Orchestration ----------
async function loadFirmware(file, target) {
  if (file) {
    log(`Gebruik geüpload bestand: ${file.name}`);
    return parseIntelHex(await file.text(), target.flashSize);
  }
  log(`Ophalen van ${target.firmware}...`);
  const res = await fetch(target.firmware);
  if (!res.ok) throw new Error(`Kon firmware niet laden (${res.status}). Zorg ervoor dat ${target.firmware} bestaat.`);
  return parseIntelHex(await res.text(), target.flashSize);
}

async function flashFlow() {
  const btn = $("flashBtn");
  btn.disabled = true;
  setProgress(0);
  setStatus("Firmware laden...");
  try {
    const target = TARGETS[$("target").value];
    log(`Doel: ${target.label}`);
    const image = await loadFirmware($("customHex").files[0], target);
    setStatus("Selecteer de seriële poort van je apparaat...");
    const port = await navigator.serial.requestPort();
    const io = new SerialIO(port);
    const baudOverride = parseInt($("baud").value, 10);
    const baud = Number.isFinite(baudOverride) ? baudOverride : target.baud;
    await io.open(baud);
    log(`Poort geopend op ${baud} baud`);

    setStatus("Resetten naar bootloader...");
    await io.setReset(target.reset);

    setStatus("Flashen...");
    await flash(io, image, $("verifyChk").checked, (frac) => {
      setProgress(frac * 100);
      setStatus(`Flashen... ${Math.round(frac * 100)}%`);
    });

    await io.close();
    setProgress(100);
    setStatus("Klaar! Je apparaat wordt opnieuw opgestart.", "good");
    log("Alles klaar.");
  } catch (e) {
    console.error(e);
    setStatus(`Fout: ${e.message}`, "bad");
    log(`FOUT: ${e.message}`);
  } finally {
    btn.disabled = false;
  }
}

// ---------- Init ----------
function init() {
  if (!("serial" in navigator)) {
    $("unsupported").hidden = false;
    return;
  }
  $("app").hidden = false;
  $("flashBtn").addEventListener("click", flashFlow);
}
init();
