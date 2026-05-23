# inoSKS Web Flasher

Browser-based firmware flasher for the inoSKS device (ATmega4809 + Optiboot via MegaCoreX).
Uses the Web Serial API and STK500v1 — no installs required for the end user.

## Updating the bundled firmware

1. Compile the sketch in the Arduino IDE with the **MegaCoreX → ATmega4809** board selected.
2. Use **Sketch → Export Compiled Binary**.
3. Copy the produced `.hex` file (e.g. `inoSKS.ino.hex`) into `firmware/inoSKS.hex` in this folder.
4. Commit and push.

## Deploying to GitHub Pages

This folder is self-contained static HTML/JS/CSS — no build step.

If this is published from a repo root that *is* the Pages site (e.g. `tuigwerk.github.io`), copy the contents of `web-flasher/` to the repo root. If you'd rather host it at a subpath like `tuigwerk.github.io/flasher/`, put the folder under that path and update `FIRMWARE_URL` in `flasher.js` if you change the firmware path.

In repo Settings → Pages, set the source to the appropriate branch / folder.

## Browser support

Web Serial works in **Chrome, Edge, and Opera** on desktop (Windows, macOS, Linux, ChromeOS). It does **not** work in Firefox or Safari, and not on iOS at all. The page detects this and shows a message.

## How it works

1. Loads `firmware/inoSKS.hex`, parses Intel HEX into a flat flash image padded with `0xFF`.
2. Asks the user to pick the device's serial port (`navigator.serial.requestPort()`).
3. Opens the port at 115200 8N1 and toggles **DTR** (or **RTS** if configured) to trigger Optiboot's auto-reset.
4. Speaks STK500v1: sync → enter prog mode → load address + program page (128 B) loop → optional read-back verify → leave prog mode.
5. Closes the port. The MCU jumps to the new firmware.

## Advanced options

- **Verify after flashing** — reads the flash back and compares against the source. On by default; turn off only if you need raw speed.
- **Log raw TX/RX bytes (debug)** — dumps every byte sent to and received from the device into the log panel. Useful when diagnosing protocol issues.
- **Baud rate override** — leave on "Use target default" (115200) unless you have customised Optiboot.

## Files

- `index.html` — UI shell
- `styles.css` — styling
- `flasher.js` — HEX parser, STK500v1 implementation, UI wiring
- `firmware/inoSKS.hex` — bundled firmware (placeholder until you drop in a real build)
