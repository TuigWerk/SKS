///////////////////////BUZZER///////////////////////////

// Tone sequence when turning on
void buzzerOn() {
  tone(PIEZO, 2960, 100);
  delay(100);
  tone(PIEZO, 3951, 100);
  delay(100);
  tone(PIEZO, 4434, 200);
  delay(100);
}

// Tone sequence when turning off
void buzzerOff() {
  tone(PIEZO, 2960, 100);
  delay(100);
  tone(PIEZO, 2637, 100);
  delay(100);
  tone(PIEZO, 1976, 300);
  delay(100);
}

void buzzerError() {
  tone(PIEZO, 2371, 300);  // E note - longer duration
  delay(400);                  // Longer pause between notes
  tone(PIEZO, 1976, 500);  // B note 1976 - perfect fifth interval or C note 2093 - creates minor third interval
  delay(100);
}


void Buzzer(int times, int duration) {
  for (int i = 0; i < times; i++) {
    digitalWrite(RUMBLE_ON, HIGH);
    tone(PIEZO, 3951, duration);
    delay(duration);
    digitalWrite(RUMBLE_ON, LOW);
    delay(duration);
  }
}

void vibrate(int duration) {
  digitalWrite(RUMBLE_ON, HIGH);
  delay(duration);
  digitalWrite(RUMBLE_ON, LOW);
}