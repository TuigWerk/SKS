/////////////////BATTERY & SOC////////////////////////////////////////

// Measure battery voltage function
void MeasureBat() {
  // Store previous level for change detection
  socLevel = voltageToSoCLevel(readBatteryVoltage());

  // Update measurement interval based on battery level
  if (socLevel <= 1) {
    BatMeasurementInterval = 1000;  // More frequent checks for low battery
  } else {
    BatMeasurementInterval = 5000;  // Standard interval for normal battery
  }

  // Go to deep sleep if the battery voltage is critically low
  if (socLevel == 0 && currentState != CHARGING && !poweringDown) {
    DEBUG_PRINTLN("Shutting down to save battery");
    poweringDown = true;
    powerDownRoutine();
  }
}

// Convert battery voltage to corresponding battery level (based on isCharging or not)
int voltageToSoCLevel(int batteryVoltage) {
  for (int i = 0; i < socLevels; i++) {                      // 7 socLevels 0-6
    if (batteryVoltage < LEVEL_THRESHOLDS[i]) {              // 6 thersholds: thershold(0-5)
      return i;
    }
  }
  return socLevels;  // If not smaller, then bigger then last Threshold
}

int readBatteryVoltage(void) {
  long value;
  digitalWriteFast(MEAS_BATT, HIGH);  // enable voltage divider
  delay(5);                           // allow voltage to stabilize (5 * R * C)
  value = analogRead(BATT_VOLT);      // !!!!!  Do we need a measurement under load??? ****<<<++++++
  digitalWriteFast(MEAS_BATT, LOW);   // disable voltage divider
  return (int)(600 * value / 1023);
}

uint8_t valueToSend(void) {
  return (uint8_t)(readBatteryVoltage() - 300);
}
