void LEDtest(int speed) {
  digitalWriteFast(LED1, HIGH);
  delay(speed);
  digitalWriteFast(LED1, LOW);
  delay(speed);
  digitalWriteFast(LED2, HIGH);
  delay(speed);
  digitalWriteFast(LED2, LOW);
  delay(speed);
  digitalWriteFast(LED3, HIGH);
  delay(speed);
  digitalWriteFast(LED3, LOW);
  delay(speed);
  digitalWriteFast(LED4, HIGH);
  delay(speed);
  digitalWriteFast(LED4, LOW);
  delay(speed);
}

void motorTest(void) {
  int adcval;
  digitalWriteFast(nAMP_SHDN, HIGH);  // enable the opamp

  while (digitalReadFast(SW_ON_OFF) == HIGH)
    ;
  digitalWriteFast(M1_IN1, HIGH);  // turn on motor
  delay(100);                      // wait for motor current to stabilize
  for (int i = 0; i < 500; i++) {
    adcval = analogRead(I_MOTOR1);
    if (adcval >= 300) break;
    delay(50);
  }
  digitalWriteFast(M1_IN2, HIGH);  // brake
  delay(1);
  digitalWriteFast(M1_IN1, LOW);
  digitalWriteFast(M1_IN2, LOW);

  Serial.println("Stall!");

  // while (digitalReadFast(SW_ON_OFF) == HIGH)
  //   ;
  // digitalWriteFast(M1_IN2, HIGH);
  // // delay(5000);
  // for (int i = 0; i < 100; i++) {
  //   Serial.println(analogRead(I_MOTOR1));
  //   delay(50);
  // }
  // digitalWriteFast(M1_IN1, HIGH);  // brake
  // delay(1);
  // digitalWriteFast(M1_IN1, LOW);
  // digitalWriteFast(M1_IN2, LOW);

  // digitalWriteFast(nAMP_SHDN, LOW);  // disable the opamp
}


//test hallsensoren

void testHallSensors(void) {

	digitalWriteFast(LED1, HIGH);
	digitalWriteFast(LED2, HIGH);
	digitalWriteFast(LED3, HIGH);
	digitalWriteFast(LED4, HIGH);

	while(1) {
		if(digitalReadFast(HALL_1) == LOW) {
			digitalWriteFast(LED4, LOW);
			delay(10);
		}

		if(digitalReadFast(HALL_2) == LOW) {
			digitalWriteFast(LED1, LOW);
			delay(10);
		}

		if(digitalReadFast(HALL_1) == HIGH) {
			digitalWriteFast(LED4, HIGH);
			delay(10);
		}

		if(digitalReadFast(HALL_2) == HIGH) {
			digitalWriteFast(LED1, HIGH);
			delay(10);
		}
	}
}

void debugReport() {
  /* Check the battery voltage and determine SOC level, then blink the corresponding LEDs if the device is not charging. */
  DEBUG_PRINTLN();
  DEBUG_PRINT("Device ID: ");
  DEBUG_PRINT(ID);
  DEBUG_PRINT(", Message Count: ");
  DEBUG_PRINT(counter);
  DEBUG_PRINT(", Release Count: ");
  DEBUG_PRINTLN(releaseCounter);
  DEBUG_PRINT("Battery Voltage: ");
  DEBUG_PRINT(readBatteryVoltage());
  DEBUG_PRINT(",  SoC Level: ");
  DEBUG_PRINT(socLevel);
  DEBUG_PRINT(",  SoC Level based Threshold: ");
  DEBUG_PRINTLN(LEVEL_THRESHOLDS[socLevel]);
  DEBUG_PRINT("Current State: ");
  DEBUG_PRINT(currentState == IDLE ? "IDLE" : "CHARGING");
  DEBUG_PRINT(", Charger State: ");
  DEBUG_PRINT(digitalReadFast(CHARGE_STAT) ? "OFF" : "ON");
  DEBUG_PRINTLN();
  DEBUG_PRINTLN();
}
