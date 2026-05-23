// LoRa related stuff - Seeed Studio
void initLoRa(void){
  const unsigned long getMessageWindow = 1000;
  DEBUG_PRINTLN("Baudrate: 230400");
  LoRa.begin(230400);              // Comm port for output to LoRa Module

  DEBUG_PRINTLN("AT");
  LoRa.println("AT");
  if(!getMessageWithStringWithinWindow(": ", getMessageWindow)) {
    DEBUG_PRINTLN("AT test failed - change baudrate of LoRa module");    
    LoRa.begin(9600);              // Comm port for output to LoRa Module
    DEBUG_PRINTLN("Baudrate set to: 9600, now trying to set it to 230400");

    DEBUG_PRINTLN("AT+UART=BR, 230400");
    LoRa.println("AT+UART=BR, 230400");
    if(!getMessageWithStringWithinWindow("+UART: BR, 230400", getMessageWindow)) ErrorMode();
    
    DEBUG_PRINTLN("AT+RESET");
    LoRa.println("AT+RESET");
    if(!getMessageWithStringWithinWindow("+RESET: OK", getMessageWindow)) ErrorMode();

    LoRa.end();
    LoRa.begin(230400);              // Comm port for output to LoRa Module
    DEBUG_PRINTLN("Baudrate: 230400");

    LoRa.println("AT");
    DEBUG_PRINTLN("AT");
    if(!getMessageWithStringWithinWindow("+AT: OK", getMessageWindow)) ErrorMode();
  }
  DEBUG_PRINTLN("AT+MODE=TEST");
  LoRa.println("AT+MODE=TEST");
  if(!getMessageWithStringWithinWindow("+MODE: TEST", getMessageWindow)) ErrorMode();
      
  DEBUG_PRINTLN("AT+MODE=?");
  LoRa.println("AT+MODE=?");
  if(!getMessageWithStringWithinWindow("+MODE: TEST", getMessageWindow)) ErrorMode();
  
  DEBUG_PRINTLN("AT+TEST=RFCFG,866,SF9,500,12,15,14,ON,OFF,OFF");
  LoRa.println("AT+TEST=RFCFG,866,SF9,500,12,15,14,ON,OFF,OFF");
  //DEBUG_PRINTLN("AT+TEST=RFCFG,866,SF12,125,12,15,14,ON,OFF,OFF");
  //LoRa.println("AT+TEST=RFCFG,866,SF12,125,12,15,14,ON,OFF,OFF");
  if(!getMessageWithStringWithinWindow("NET:OFF", getMessageWindow)) ErrorMode();
  DEBUG_PRINTLN("Initialisation done!");
  DEBUG_PRINTLN();

}

void ErrorMode() {
  unsigned long timerMillis = millis();
  while(millis()-timerMillis < 5000){
    digitalWriteFast(LED1, LOW);
    digitalWriteFast(LED2, LOW);
    digitalWriteFast(LED3, LOW);
    digitalWriteFast(LED4, LOW);
    delay(750);
    digitalWriteFast(LED1, HIGH);
    digitalWriteFast(LED2, HIGH);
    digitalWriteFast(LED3, HIGH);
    digitalWriteFast(LED4, HIGH);
    delay(750);
  }
  // reset LoRa module via pin
  digitalWriteFast(LORA_WAKEUP, LOW);
  delay(5);
  digitalWriteFast(LORA_WAKEUP, HIGH);
  delay(5);
  
  // reste microcontroller
  resetFunc(); //call reset
}

void sleepLoRa(unsigned long getMessageWindow) {
  LoRa.println("AT+LOWPOWER");
  if(!getMessageWithStringWithinWindow("SLEEP", getMessageWindow)) {
    DEBUG_PRINTLN("LoRa Module having insomnia! - Timeout!");
    ErrorMode();
  }
}

void wakeUpLoRa(unsigned long getMessageWindow) {
  LoRa.println("AT");
  if(!getMessageWithStringWithinWindow("WAKEUP", getMessageWindow)) DEBUG_PRINTLN("LoRa still sleeping or was never asleep...");
}

bool getMessageWithStringWithinWindow(const char* lookingFor, unsigned long timeWindow){
    unsigned long startMillisGetMessage = millis();
    bool strFound = false;
    while(1) {
    if(messageAvailable()) {
      msgReceived = false; // ready to receive new message (one line -till LF- for the LoRa module) 
      DEBUG_PRINTLN(stringBuffer);
      if(strstr(stringBuffer, lookingFor)){
        strFound = true;
        break;
      }
    }
    if(millis() - startMillisGetMessage > timeWindow){
      DEBUG_PRINTLN("String not found in LoRa messages - time out!");
      break;
    }
  }
  return strFound;
}

void sendCommandValueWithoutWindow(uint8_t command) {

  uint32_t packet[2];
  uint8_t nibble;

  counter++;
  EEPROM.put(104, counter);
  
  packet[0] = (ID << 8) | (uint32_t)command;
  packet[1] = counter;

  xteaEncrypt(packet, xteaKey);

  strcpy(stringBuffer, "AT+TEST=TXLRPKT,\"");
  for (int i = 0; i < 16; i++) {
    nibble = (uint8_t)((packet[i/8] >> ((7-i%8)*4)) & 0x0F);
    if (nibble < 10) {
      stringBuffer[17+i] = (char)(nibble + 48);
    } else {
      stringBuffer[17+i] = (char)(nibble + 55);
    }
  }
  stringBuffer[33] = '\"';
  stringBuffer[34] = '\0';

  DEBUG_PRINTLN(stringBuffer);
  LoRa.println(stringBuffer);
  
}

bool sendCommandValueWithinWindow(uint8_t command, unsigned long LoRaTimeOut) {

  sendCommandValueWithoutWindow(command);  
  return getMessageWithStringWithinWindow("TX DONE", LoRaTimeOut);
}

bool commandValueReceivedWithinWindow(uint8_t* ptrCommandValue, unsigned long receiveWindow) {

  unsigned long startMillisReceiving;
  bool properCommandValueReceived = false;

  //LoRa.println("AT+TEST=RXLRPKT");
  startMillisReceiving = millis();
  while(1){
    if(messageAvailable()){
      msgReceived = false; // ready to receive new message (one line -till LF- for the LoRa module) 
      DEBUG_PRINT("From commandValueReceiveWithinWindow: ");
      DEBUG_PRINTLN(stringBuffer);
      *ptrCommandValue = parseMessage();
      if (*ptrCommandValue < 0xFC) {
        properCommandValueReceived = true;   // Found a proper command/value
        break;                // 0xFC till 0xFF are error codes
      }
    }
    if(millis() - startMillisReceiving > receiveWindow){
      // DEBUG_PRINTLN("No proper command/value in all messages received within window - time out! Error code in command/value variable");
      break;
    }
  }
  return properCommandValueReceived;
}

bool setLoRaInReceiveMode(unsigned long LoRaTimeOut) {
  LoRa.println("AT+TEST=RXLRPKT");
  return getMessageWithStringWithinWindow("RXLRPKT", LoRaTimeOut);
}

bool messageAvailable(void) {
  char incomingByte;
  while(LoRa.available()>0 && msgReceived == false) {
    incomingByte = LoRa.read();
    if (incomingByte == 0x0A) {
      stringBuffer[bufferIndex] = 0; // replace LF 0x0A with NULL terminator
      msgReceived = true;
      bufferIndex = 0;
    } else if (incomingByte == 0x00 || incomingByte == 0x0D) {
      // flush NULL and CR terminator: do nothing
    } else {
      stringBuffer[bufferIndex] = incomingByte;
      bufferIndex++;
      stringBuffer[bufferIndex] = 0; // string end
    }
    if (bufferIndex == 63) {
      msgReceived = true;
      bufferIndex = 0;
    }
  }
  return msgReceived;
}

uint8_t parseMessage(void) {

  // See if you can find HEX code - if not, return 254 (0xFE)
  char* startPtr = strchr(stringBuffer, '"');                                   // search for first " in buffer
  char* endPtr = strrchr(stringBuffer, '"');                                // search for last " in buffer
  if (!startPtr || !endPtr || endPtr <= startPtr || endPtr - startPtr - 1 != 8 * 2) {     // check all ok
    //DEBUG_PRINTLN("Unable to find proper Hex string.");                     // if not, print this
    return 0xFE;                                                            // and return 0xFF as error code
  }
  
  // When you have HEX code, decrypt
  uint32_t packet[2];
  *endPtr = 0;                                                 // terminate stringBuffer after hexidecimaal packet 
  packet[1] = (uint32_t)(strtoul(startPtr + 9, NULL, 16));         // get second 8 byte part of hex
  *(endPtr-8) = 0;                                             // terminate stringBuffer after half hexidecimaal packet 
  packet[0] = (uint32_t)(strtoul(startPtr + 1, NULL, 16));         // get second 8 byte part of hex
  xteaDecrypt(packet, xteaKey);
  uint32_t sendID = packet[0] >> 8;

  // When decryped, check send ID vs. own ID - if unequal, return 253 (0xFD)
  DEBUG_PRINT("The ID in the message: ");
  DEBUG_PRINTLN(packet[0] >> 8);
  if(sendID != ID) {
    return 0xFD;
  }

  // When ID checked, check send counter vs. own counter - if smaller (old), return 252 (0xFC)
  uint32_t sendCounter = packet[1];
  DEBUG_PRINT("The counter in the message: ");
  DEBUG_PRINTLN(sendCounter);
  DEBUG_PRINT("The counter in the system: ");
  DEBUG_PRINTLN(counter);
  if(sendCounter < counter) {
    return 0xFC;
  }

  // Re-align counter (only if no error) with received counter and return command or value
  counter = sendCounter;
  EEPROM.put(104, counter);
  return (uint8_t)packet[0];
}

