#include "defines.h"

void initIO(void) {
  digitalWriteFast(LED1, HIGH);
  digitalWriteFast(LED2, HIGH);
  digitalWriteFast(LED3, HIGH);
  digitalWriteFast(LED4, HIGH);
  //digitalWriteFast(LED5, HIGH);

  digitalWriteFast(MEAS_BATT, LOW);
  digitalWriteFast(nAMP_SHDN, LOW);
  digitalWriteFast(EN_HALL, LOW);

  digitalWriteFast(LED_BLUE, HIGH);
  digitalWriteFast(LED_GREEN, HIGH);
  digitalWriteFast(LED_RED, HIGH);
  digitalWriteFast(DIM_RGB, HIGH);

  digitalWriteFast(M1_IN1, LOW);
  digitalWriteFast(M1_IN2, LOW);
  digitalWriteFast(M2_IN1, LOW);
  digitalWriteFast(M2_IN2, LOW);

  digitalWriteFast(LORA_WAKEUP, HIGH);
  digitalWriteFast(RUMBLE_ON, LOW);
  digitalWriteFast(PIEZO, LOW);

  pinMode(SW_ON_OFF, INPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(LED4, OUTPUT);
  pinMode(LED5, INPUT_PULLUP); // old LED pin - not in use anymore - set to input

  pinMode(MEAS_BATT, OUTPUT);
  pinMode(nAMP_SHDN, OUTPUT);
  pinMode(EN_HALL, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(DIM_RGB, OUTPUT);

  pinMode(BATT_VOLT, INPUT);
  pinMode(I_MOTOR1, INPUT);
  pinMode(I_MOTOR2, INPUT);
  pinMode(CHARGE_STAT, INPUT);
  pinMode(nPG, INPUT);
  pinMode(HALL_1, INPUT);

  pinMode(M1_IN1, OUTPUT);
  pinMode(M1_IN2, OUTPUT);
  pinMode(M2_IN1, OUTPUT);
  pinMode(M2_IN2, OUTPUT);
  pinMode(LORA_WAKEUP, OUTPUT);
  pinMode(RUMBLE_ON, OUTPUT);
  pinMode(PIEZO, OUTPUT);

  pinMode(NOT_USED11, OUTPUT);
  pinMode(NOT_USED12, OUTPUT);
  pinMode(NOT_USED13, OUTPUT);
  pinMode(NOT_USED18, OUTPUT);
  pinMode(NOT_USED19, OUTPUT);
  pinMode(NOT_USED21, OUTPUT);
  pinMode(NOT_USED26, OUTPUT);
  pinMode(NOT_USED29, OUTPUT);
  pinMode(NOT_USED37, OUTPUT);
}

// reset function
void resetFunc() {
  wdt_enable(WDT_PERIOD_8KCLK_gc); // 8 ms watchdog 
 while(true); // infinite loop without feeding the dog, should reset in 8ms
}