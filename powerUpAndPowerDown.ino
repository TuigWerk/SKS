/////////////////////////POWER UP & POWER DOWN//////////////////////

void powerUpRoutine() {
  LEDsequence();
  buzzerOn();
  blinkLEDs(1, socLevel-1, socNonBlinkDuration);
}

// LED and buzzer routine - 
void powerDownRoutine() {
  if (!poweringDown) return;
  turnAllLedsOff();
  delay(200);
  buzzerOff();
  blinkLEDs(3, socLevel-1, socBlinkDuration);
  //vibrate(800); 
}

// Preparing for going to sleep and waking up by Button Press
void sleepRoutine() {
  DEBUG_PRINTLN("Going fully asleep...");
  if (!LoRaSleeping) {
    LoRaSleeping = true;
    sleepLoRa(getMessageWindow);
  }                                     // turn LoRa to bed - wait for LoRa module for 200 ms for zzzzz, otherwise insomnia
  initIO();                             // re-initialise all pins to make sure all are off and pulled the right direction - hall sensors to sleep
  sleep_enable();                       // Enabling sleep mode
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);  // Sets the lowest possible  (!?)
  wakeUpOnButton = false;               // Set wake up flag to false
  //wakeUpOnUSB = false;                  // Set wake up flag to false
  Serial.flush();                       // Flush serial tx - make sure send-buffer is empty
  Serial.end();                         // End UART for debugging
  pinMode(0, OUTPUT);                   // and set pin as output LOW 
  digitalWriteFast(0, LOW);             // to prevent current leagake via USB chip
  
  attachInterrupt(digitalPinToInterrupt(SW_ON_OFF), Dummy, FALLING); // Interrupt on button
  //attachInterrupt(digitalPinToInterrupt(nPG), Dummy, FALLING); // Interrupt on USB
  sleep_cpu();                          // Activating sleep mode
  sleep_disable();                      // when returning, deactivate sleepmode
  detachInterrupt(digitalPinToInterrupt(SW_ON_OFF)); // Detach Interrupt on button
  //detachInterrupt(digitalPinToInterrupt(nPG)); // Detach Interrupt on USB
  // if(digitalReadFast(SW_ON_OFF) == LOW) wakeUpOnButton = true; // Check if we woke up by Button press
  // if(digitalReadFast(nPG) == LOW) wakeUpOnUSB = true;    // or by inserting USB cable 
  if(digitalReadFast(SW_ON_OFF) == LOW) {

    // digitalWriteFast(LED4, LOW);
    // delay(100);
    // digitalWriteFast(LED4, HIGH);
   
    wakeUpOnButton = true; // Check if we woke up by Button press
    //wakeUpOnUSB = false;    // or by inserting USB cable 
  }
  // if(digitalReadFast(nPG) == LOW) {
    
  //   // digitalWriteFast(LED2, LOW);
  //   // delay(100);
  //   // digitalWriteFast(LED2, HIGH);
    
  //   wakeUpOnUSB = true;    // or by inserting USB cable 
  //   wakeUpOnButton = false; // Check if we woke up by Button press
  // }
  pinMode(0, INPUT);                    // and set pin as output LOW 
  DEBUG_BEGIN;                          // Start UART for debugging if in DEBUG mode

  DEBUG_PRINTLN("Half awake...");
}

// Decide to wake up or not, measure length of (Button Press)
void wakeUpOrNot() {
  pressStart = millis();
  DEBUG_PRINTLN("Deciding to wake up or not");
  delay(onOffSpeed);
  vibrate(120);
  for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], LOW);
    if (digitalReadFast(SW_ON_OFF) == HIGH) {  // Check if button released
      turnAllLedsOff();
      DEBUG_PRINTLN("Short press detected - going back to sleep");
      poweringDown = true;
      startingUp = false;
      return;
    }
    delay(onOffSpeed);
  }
  DEBUG_PRINTLN("Long press detected - staying awake");
  startingUp = true;
  //vibrate(500);
}

void Dummy(void) {
}

