//////////////////////LEDS///////////////////////////////////////

// Turn all LEDs on
void turnAllLedsOn() {
  for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], LOW);
  }
}

// Turn all LEDs off
void turnAllLedsOff() {
  for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], HIGH);
  }
}

// Knightrider LED startup sequence
void LEDsequence() {
  for (int seq = 0; seq < numSequences; seq++) {
    for (int i = 0; i < numLeds; i++) {
      digitalWrite(ledPins[i], LOW);
      delay(delayTime);
      digitalWrite(ledPins[i], HIGH);
    }

    // Backward sequence (excluding first and last positions to avoid duplicates)
    for (int i = numLeds - 2; i > 0; i--) {
      digitalWrite(ledPins[i], LOW);
      delay(delayTime);
      digitalWrite(ledPins[i], HIGH);
    }
  }
  digitalWrite(ledPins[0], LOW);
  delay(delayTime);
  digitalWrite(ledPins[0], HIGH);
}


// Blink SOC LEDs for requested times and duration
void blinkLEDs(int times, int number, int duration) {
  blinkOn = true;
  blinkState = true;           // Start with LEDs on
  blinkCount = 0;              // Reset blink counter
  blinkTimes = times * 2 - 1;  // Calculate total state changes (ON+OFF pairs, but one less OFF)
  blinkNumber = number;        // Set number of leds to blink
  blinkDuration = duration;    // Set blink duration

  // Turn on LEDs immediately
  for (int i = 0; i < blinkNumber; i++) {
    digitalWrite(ledPins[i], LOW);  // Turn on LEDs (active LOW)
  }
  lastLEDupdate = millis();  // Initialize the timer
}

void updateLEDs() {
  if (millis() - lastLEDupdate >= blinkDuration && blinkOn) {
    // Toggle state
    blinkState = !blinkState;
    blinkCount++;

    // Update LEDs based on new state
    if (blinkState) {
      // Turn ON LEDs for socLevel
      for (int i = 0; i < blinkNumber; i++) {
        digitalWrite(ledPins[i], LOW);  // Turn on LEDs (active LOW)
      }
    } else {
      // Turn OFF all LEDs
      turnAllLedsOff();
    }

    // Check if we've completed the required number of state changes
    if (blinkCount >= blinkTimes) {
      blinkOn = false;  // End blink sequence
      SetRGBandOn(BLACK);          // Turn off RGB - bit doggy - but for now
    }

    lastLEDupdate = millis();
  }
}

void updateChargingLEDs() {
  // Handle charge-blinking timing
  if (millis() - lastChargingBlink >= chargingBlinkInterval) {
    chargingBlinkState = !chargingBlinkState;
    lastChargingBlink = millis();
    
    // For SoC set LED states
    for (int i = 0; i < numLeds; i++) {
      if((socLevel-i) > 2) digitalWrite(ledPins[i], LOW); else digitalWrite(ledPins[i], HIGH);
      if((socLevel-i) == 2) digitalWrite(ledPins[i], chargingBlinkState);
      if(socLevel == 1 && i == 0) digitalWrite(ledPins[i], chargingBlinkState);
      
    }
  }
}

void FadeOutRGB(int color, int speed) {      
  int i;
  digitalWriteFast(LED_RED, !(color & 0b100));  
  digitalWriteFast(LED_GREEN, !(color & 0b010));
  digitalWriteFast(LED_BLUE, !(color & 0b001));

  for (i = 255; i > 0; i -= 5) {
    analogWrite(DIM_RGB, i);
    delay(speed);
  }
  analogWrite(DIM_RGB, 255);
}

void FadeInRGB(int color, int speed) {        
  int i;
  digitalWriteFast(LED_RED, !(color & 0b100));  
  digitalWriteFast(LED_GREEN, !(color & 0b010));
  digitalWriteFast(LED_BLUE, !(color & 0b001));

  for (i = 0; i < 255; i += 5) {
    analogWrite(DIM_RGB, i);
    delay(speed);
  }
  analogWrite(DIM_RGB, 255);
}

void SetRGBandOn(uint8_t color) {          // colors: BLACK, RED, BLUE, GREEN, MAGENTA, YELLOW, CYAN, WHITE
  digitalWriteFast(LED_RED, ((color & 0b00000100) == 0b00000100) ? LOW : HIGH);  
  digitalWriteFast(LED_GREEN, ((color & 0b00000010) == 0b00000010) ? LOW : HIGH);  
  digitalWriteFast(LED_BLUE, ((color & 0b00000001) == 0b00000001) ? LOW : HIGH);  
  analogWrite(DIM_RGB, 255);
}