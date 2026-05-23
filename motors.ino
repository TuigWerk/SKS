
///////////////////// MOTOR CONTROL ////////////////////////

unsigned long motorTillCurrent(int stallCurrent, unsigned long timeOut, bool clockWise) {
  int adcvalM1;
  int adcvalM2;
  unsigned long startMillis;
  unsigned long durationM1 = timeOut;
  unsigned long durationM2 = timeOut;

  digitalWriteFast(nAMP_SHDN, HIGH);  // enable the opamp
  delayMicroseconds(50);
  startMillis = millis();

  // turn on motor 1 and 2 in the right direction
  // M1 = Rightside (Black). M2 = Left side (Red reprint)
  digitalWriteFast(M1_IN1, clockWise);
  digitalWriteFast(M1_IN2, !clockWise);
  digitalWriteFast(M2_IN2, clockWise);
  digitalWriteFast(M2_IN1, !clockWise);
  delay(100);  // wait for motor current to stabilize

  do {
    adcvalM1 = analogRead(I_MOTOR1);
    adcvalM2 = analogRead(I_MOTOR2);
    if (adcvalM1 >= stallCurrent) {
      digitalWriteFast(M1_IN1, HIGH);
      digitalWriteFast(M1_IN2, HIGH);
      durationM1 = millis() - startMillis;
    }
    if (adcvalM2 >= stallCurrent) {
      digitalWriteFast(M2_IN2, HIGH);
      digitalWriteFast(M2_IN1, HIGH);
      durationM2 = millis() - startMillis;
    }
    delay(50);
  } while (millis() - startMillis < timeOut && (adcvalM1 < stallCurrent || adcvalM2 < stallCurrent));

  delay(10);
  digitalWriteFast(M1_IN1, LOW);
  digitalWriteFast(M1_IN2, LOW);
  digitalWriteFast(M2_IN2, LOW);
  digitalWriteFast(M2_IN1, LOW);
  delay(10);
  // reserse a bit
  digitalWriteFast(M1_IN1, !clockWise);
  digitalWriteFast(M1_IN2, clockWise);
  digitalWriteFast(M2_IN2, !clockWise);
  digitalWriteFast(M2_IN1, clockWise);
  delay(10);
  digitalWriteFast(M1_IN1, HIGH);
  digitalWriteFast(M1_IN2, HIGH);
  digitalWriteFast(M2_IN2, HIGH);
  digitalWriteFast(M2_IN1, HIGH);
  delay(10);
  digitalWriteFast(M1_IN1, LOW);
  digitalWriteFast(M1_IN2, LOW);
  digitalWriteFast(M2_IN2, LOW);
  digitalWriteFast(M2_IN1, LOW);
  
  digitalWriteFast(nAMP_SHDN, LOW);  // disable the opamp

  return min(durationM1, durationM2);
}