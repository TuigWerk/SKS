#include "defines.h"
//#include <RunningAverage.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include "EEPROM.h"

//define global variables here

// Button timing and flag
bool buttonPressed = false;
unsigned long pressStart = 0;
int onOffSpeed = 350;
unsigned long longPressMillis = 3 * onOffSpeed;  //Less than 5 times onOffSpeed

// Variables for non-blocking LED blinking
bool blinkOn = false;                     // To blink or not to blink
bool blinkState = true;                   // State of blinking LEDs
int blinkCount;                           // counter to keep track of blinking
unsigned long blinkStart;                 // time when blink command was given
unsigned long lastLEDupdate;              // time of last LED update
unsigned long socBlinkDuration = 200;     // Duration of SOC blink
unsigned long socNonBlinkDuration = 2000; // Duration of SOC when not blinking
int blinkTimes;                           // Target number of blinks
int blinkNumber;                          // Set number of leds to blink, Standard SOC
int blinkDuration;                        // Duration of a blink
bool chargingBlinkState = false;
unsigned long lastChargingBlink = 0;
const unsigned long chargingBlinkInterval = 750;  // 750ms blink interval for charging

//For Idle resting (LoRa off)
unsigned long CPUTimer = 10000;  // reduce CPU frequency after x milliseconds of inactivity
unsigned long lastLoRaRequest;
bool LoRaSleeping = false;

// LED display configuration
const int ledPins[] = { LED1, LED2, LED3, LED4 };  // Added by JdJ: slightly different way of defining

const int numLeds = 4;
const int delayTime = 150;   // Change this value to adjust the speed of Knightrider
const int numSequences = 2;  // Number of complete sequences to run

// Battery SOC level thresholds
const int socLevels = numLeds + 2;
const int LEVEL_THRESHOLDS[socLevels] = {
  // ---------------------------------------------------------------------|
  // V_bat (cV)   | IDLE State      | CHARGING State | SoC Level | Colour | 
  // ---------------------------------------------------------------------|
  //              | => Shutdown     | 1 Blink        | 0         | n.a.   |
          340,    // -----------------------------------------------------|
  //              |  1 Blink        | 1 Blink        | 1         | Yellow |
          353,    // -----------------------------------------------------|
  //              |  1 On           | 1 Blink        | 2         | Yellow |
          368,    // -----------------------------------------------------|
  //              |  2 On           | 1 On & 1 Blink | 3         | Blue   |
          375,    // -----------------------------------------------------|
  //              |  3 On           | 2 On & 1 Blink | 4         | Blue   |
          392,    // -----------------------------------------------------|
  //              |  4 On           | 3 On & 1 Blink | 5         | Blue   |
          415     // -----------------------------------------------------|
  //              |  4 On           | 4 On           | 6         | Blue   |
  // ---------------------------------------------------------------------|
};

// Battery measurement variables
int batteryVoltage;
int socLevel;
bool showOwnBattery;
unsigned long lastBatMeasurement;
unsigned long BatMeasurementInterval;

bool startingUp;
bool poweringDown;
//bool wakeUpOnUSB;
bool wakeUpOnButton;

int releaseCounter = 0;

unsigned long startTime;
unsigned long debounceTime = 200;
unsigned long pressWindow = 200;
bool releaseButtonPressed = false;

// Newly added for encryption & rolling code, and for a different way of sending messages
// variables for identification etc. stored in EEPROM/flash
uint32_t counter;     // counter for messaging - 4 bytes; 2^32 = 4.294.967.296 send and receive actions possible before EOL
uint32_t ID;          // device ID - instead of name - 3 bytes used in 4 bytes variable; 2^24 = 16.777.216 devices possible
uint32_t xteaKey[4];  // key for XTEA encryption - 4 x 4 = 16 bytes key

// variables to handle uart signals to and from LoRa module
char stringBuffer[64];  // buffer for stings - for sending and receiving
int bufferIndex = 0;
bool msgReceived = false;
const unsigned long receiveWindow = 1000; // Window for receiving OTA messages Ping-Pong
const unsigned long getMessageWindow = 1000; //window for communication with LoRa Seeed Studio Module

// motor direction boolean - changing this direction will change all motor movement direction
const bool direction = true;

// Value and command - to be wrapped into packet, encrypted, send, decrypted and used.
uint8_t command;  // command to be send by trigger to foedraal
uint8_t value;    // value to be received by trigger from foedraal

// State tracking
enum SystemState {
  IDLE,
  CHARGING
};
SystemState currentState = IDLE;

// ===================================================================================================

void setup() {
  //Set all IO-pins
  initIO();

  // Allow interupts
  interrupts();

  // Start debug (if in debug mode)
  DEBUG_BEGIN;  // Virtual comm port for output to serial monitor
  DEBUG_PRINTLN("Hello World!");

  // Start LoRa
  initLoRa();

  // // temporaraly for first time running
  // ID = 115;
  // counter = 0;
  // xteaKey[0] = 0xDEADBEEF;
  // xteaKey[1] = 0xCAFEBABE;
  // xteaKey[2] = 0x12345678;
  // xteaKey[3] = 0x9ABCDEF0;

  // EEPROM.put(100, ID);
  // EEPROM.put(104, counter);
  // EEPROM.put(108, xteaKey[0]);
  // EEPROM.put(112, xteaKey[1]);
  // EEPROM.put(116, xteaKey[2]);
  // EEPROM.put(120, xteaKey[3]);

  // Get counter & ID from EEPROM and store in global variable already defined
  EEPROM.get(100, ID);
  EEPROM.get(104, counter);
  EEPROM.get(108, xteaKey[0]);
  EEPROM.get(112, xteaKey[1]);
  EEPROM.get(116, xteaKey[2]);
  EEPROM.get(120, xteaKey[3]);

  // Measure battery and calculate SoC level
  MeasureBat();
  showOwnBattery = false;
  startingUp = true;
  BatMeasurementInterval = 5000;    

  #ifdef TRIGGER_CODE  
    digitalWriteFast(EN_HALL, HIGH); // Power the Hall sensors - ALWAYS - otherwise the release mechanism is triggert...
  #endif 

}

// ======================================= MAIN ======================================
void loop() {

  // Startup sequenze
  if (startingUp && !blinkOn) {
    startingUp = false;
    #ifndef TRIGGER_CODE                      // non def, so FOEDRAAL_CODE
      setLoRaInReceiveMode(getMessageWindow); // Set in receive mode
    #endif
    powerUpRoutine();                         // Take care of LEDs and Buzzers
    buttonPressed = false;
  }

  // Measure battery after every battery Measurment interval and blink SOC LEDs if not charging
  if ((millis() - lastBatMeasurement > BatMeasurementInterval || showOwnBattery == true) && !blinkOn) {
    lastBatMeasurement = millis();
    MeasureBat();
    //debugReport();
    //Blink SOC LEDs if not charging
    if (currentState != CHARGING && !blinkOn) {
      blinkLEDs(1, socLevel-1, socBlinkDuration*(showOwnBattery+1));
    }
    showOwnBattery = false;
  }

  // Check Release state and decide what to do:
  switch (currentState) {
    case IDLE:  //WAIT FOR COMMAND FROM TRIGGER, CHECK USB-CONNECTION AND POWER / BATTERY BUTTON

      //////////////////CHECK POWER BUTTON/////////////////////////////////
      // Check the button status: (Trigger & Foedraal)
      if (digitalRead(SW_ON_OFF) == LOW && buttonPressed == false && !poweringDown) {
        buttonPressed = true;  //set flag to true
        lastLoRaRequest = millis();
        if(LoRaSleeping) {
          wakeUpLoRa(200);
          LoRaSleeping = false;
        }
        vibrate(120);
        pressStart = millis();  // start button timer
        DEBUG_PRINTLN("Power button pressed - checking for long press");
      }

      // check for long press of button - turn off if not released (Trigger & Foedraal)
      if (millis() - pressStart > longPressMillis && buttonPressed == true && !poweringDown) {
        DEBUG_PRINTLN("Long press detected");
        //Power up sequence
        //turn all leds on, so they can be turned off one by one
        turnAllLedsOn();

        //turn leds off one by one
        for (int i = numLeds - 1; i > 0; i--) {  //turn off device before reaching last LED
          delay(onOffSpeed);
          digitalWrite(ledPins[i], HIGH);

          if (digitalRead(SW_ON_OFF) == HIGH) {  // Check if button released
            turnAllLedsOff();
            buttonPressed = false;
            return;  // Exit and don't power down if button released
          }
        }

        // If we made it here, button is still pressed, finish animation and shut down
        delay(onOffSpeed);                
        digitalWrite(ledPins[0], HIGH);   // turn last LED off and shut down
        poweringDown = true;              // This will now be reached when button remains pressed
        powerDownRoutine();               // Take care of LEDs and Buzzers
        buttonPressed = false;
      }

    #ifdef TRIGGER_CODE // Trigger firmware

      // Check if button was released (short press) - get battery foedraal, then display both (Trigger version)
      if (digitalRead(SW_ON_OFF) == HIGH && buttonPressed == true && !poweringDown) {
        buttonPressed = false;
        DEBUG_PRINTLN("Short press detected - getting battery status foedraal");

        if (sendCommandValueWithinWindow(0xF0, 200)) {
          DEBUG_PRINTLN("Trigger: Message for Battery Voltage fully send and back in receive modus");
          // check if foedraal sends back battery status (voltage)
          setLoRaInReceiveMode(getMessageWindow);
          if (commandValueReceivedWithinWindow(&value, receiveWindow)) {
            DEBUG_PRINT("Trigger: Battery voltage of Foedraal is: ");
            DEBUG_PRINT(value + 300);
            DEBUG_PRINTLN("V");
            DEBUG_PRINT("Trigger: Battery voltage of Trigger  is: ");
            DEBUG_PRINT(readBatteryVoltage());
            DEBUG_PRINTLN("V");
            
            // Foedraal Battery Show
            int foedraalSocLevel = voltageToSoCLevel((int)(value+300));
            (foedraalSocLevel < 3) ? SetRGBandOn(YELLOW) : SetRGBandOn(BLUE);          // colors: BLACK, RED, BLUE, GREEN, MAGENTA, YELLOW, CYAN, WHITE
            blinkLEDs(3, foedraalSocLevel-1, socBlinkDuration);
            
          } else {
            DEBUG_PRINTLN("Trigger: Hound (and it's Battery) out of bound - no response / time-out!");
            SetRGBandOn(RED);
            blinkLEDs(1, 0, socNonBlinkDuration);
          }
          showOwnBattery = true;
        }
      }

      ////////////////////// CHECK TRIGGER HANDLES ///////////////////////////
      // One release handle pressed (only for Trigger)
      if ((digitalReadFast(HALL_1) == LOW || digitalReadFast(HALL_2) == LOW) && releaseButtonPressed == false) {
        releaseButtonPressed = true;
        lastLoRaRequest = millis();
        if(LoRaSleeping) {
          wakeUpLoRa(pressWindow / 2);
          LoRaSleeping = false;
        }
        startTime = millis();
        DEBUG_PRINTLN("One Handle pressed");
      }

      // Both Release Button pressed - order to release hound (only for Trigger)
      if (digitalReadFast(HALL_1) == LOW && digitalReadFast(HALL_2) == LOW) {
        if (millis() - startTime < pressWindow) {
          DEBUG_PRINTLN("Sending command to release hound");
          releaseCounter++;
          if (sendCommandValueWithinWindow(0x0F, 200)) {
            DEBUG_PRINTLN("Trigger: Message to Release fully send and now to receive modus");
            // check if foedraal sends back Ack to learn if foedraal has received message to unlock muzzle
            setLoRaInReceiveMode(getMessageWindow);
            if (commandValueReceivedWithinWindow(&value, receiveWindow) && value == 0x06) {
              DEBUG_PRINT("Trigger: Ack received ");
              DEBUG_PRINT(millis() - startTime);
              DEBUG_PRINTLN("ms after first half-release-pressed.");
              Buzzer(1, 200);
              SetRGBandOn(GREEN);
              blinkLEDs(1, 0, socNonBlinkDuration);
            } else {
              DEBUG_PRINTLN("Trigger: Hound out of bound - no first ack/nak response / time-out!");
              buzzerError();
              SetRGBandOn(RED);
              blinkLEDs(1, 0, socNonBlinkDuration);
            }
            // check for next response; the time-value in value is the duration is the shortest of the two motor-opening-durations, if one of motors timed-out, it is false
            if (commandValueReceivedWithinWindow(&value, receiveWindow)) {
              DEBUG_PRINT("Trigger: Shortest motor duration: ");
              DEBUG_PRINT(value * 10);
              DEBUG_PRINTLN("ms");
              DEBUG_PRINT("Trigger: Total duration from first half-release-press: ");
              DEBUG_PRINT(millis() - startTime);
              DEBUG_PRINTLN("ms");
              Buzzer(3, 200);
              SetRGBandOn(GREEN);
              blinkLEDs(1, 0, socNonBlinkDuration);              
            } else {
              DEBUG_PRINTLN("Trigger: Hound out of bound - no second timevalue response / time-out!");
              buzzerError();
              SetRGBandOn(RED);
              blinkLEDs(1, 0, socNonBlinkDuration);             
            }
          }
        }
      }

      //Do this when both buttons are released (only for Trigger)
      if (digitalReadFast(HALL_1) == HIGH && digitalReadFast(HALL_2) == HIGH) {
        if (millis() - startTime > debounceTime) {
          releaseButtonPressed = false;
        }
        startTime = millis();
      }

      // Check if LoRa can be put to sleep (only for Trigger)
      if (millis() - lastLoRaRequest > CPUTimer && !LoRaSleeping) {
        LoRaSleeping = true;
        sleepLoRa(getMessageWindow);
      }

    #else  // Foedraal firmware

      // Check if button was released (short press) - get battery foedraal
      if (digitalRead(SW_ON_OFF) == HIGH && buttonPressed == true && !poweringDown) {
        buttonPressed = false;
        DEBUG_PRINTLN("Short press detected - getting battery status foedraal (my own)");

        // Foedraal Battery Show
        (socLevel < 3) ? SetRGBandOn(YELLOW) : SetRGBandOn(BLUE);          // colors: BLACK, RED, BLUE, GREEN, MAGENTA, YELLOW, CYAN, WHITE
        blinkLEDs(3, socLevel-1, 2*socBlinkDuration);
      }

      unsigned long timeItTookToOpen;
      unsigned long timeItTookToClose;

      // Check for message => get message, parse, decrypt, execute, encypte result, send back
      if(messageAvailable()) {
        if (commandValueReceivedWithinWindow(&command, receiveWindow)) {
          switch (command) {
            case 0xF0:  // battery status request
              value = valueToSend();
              DEBUG_PRINT("Foedraal: Battery voltage is: ");
              DEBUG_PRINT(value + 300);
              DEBUG_PRINTLN("V");
              DEBUG_PRINT("Foedraal: Sending back value: ");
              DEBUG_PRINT(value);
              DEBUG_PRINTLN(". Did you get it?");

              if (sendCommandValueWithinWindow(value, getMessageWindow)) {
                DEBUG_PRINTLN("Foedraal: Message fully send and back in receive modus");
              }
              setLoRaInReceiveMode(getMessageWindow);
              break;

            case 0x0F:  // unlock muzzle request

              value = 6;  //ack
              DEBUG_PRINT("Foedraal: Sending back value: ");
              DEBUG_PRINT(value);
              DEBUG_PRINTLN(". Did you get it?");
              sendCommandValueWithoutWindow(value);  // send ack, but before aswer from LoRa module ...

              timeItTookToOpen = motorTillCurrent(400, 1000, direction);  // ...open muzzle firsta dn only then ...
              
              if (getMessageWithStringWithinWindow("TX DONE", getMessageWindow)) {   // check for message from LoRa module
                DEBUG_PRINTLN("Foedraal: Ack message fully send and already opening muzzle");
              }
              if (sendCommandValueWithinWindow((uint8_t)(timeItTookToOpen / 10), getMessageWindow)) {
                DEBUG_PRINTLN("Foedraal: motor duration message fully send and back in receive modus");
              }
              timeItTookToClose = motorTillCurrent(400, 1000, !direction);
              DEBUG_PRINT("Foedraal: Time it took to open: ");
              DEBUG_PRINT(timeItTookToOpen);
              DEBUG_PRINTLN("ms");
              DEBUG_PRINT("Foedraal: Time it took to close: ");
              DEBUG_PRINT(timeItTookToClose);
              DEBUG_PRINTLN("ms");
              setLoRaInReceiveMode(getMessageWindow);
              break;
          }
        }
      }
    #endif

      // Check if USB cable has been applied - state to CHARGING
      if (digitalReadFast(nPG) == LOW && !blinkOn) {  // If USB is corrected there is power and the Power Good pin will be LOW
        currentState = CHARGING; //IDLE; CHARGING;
      }

      // Update LEDs when blinkOn
      if (blinkOn) {
        updateLEDs();
      }

      break;

    case CHARGING:
      if (digitalReadFast(CHARGE_STAT) == LOW) {
        // Charger is ON - show SoC with blinking pattern
        updateChargingLEDs();
      } else {
        // Charging is OFF but CHARGING state still valid (nPG == LOW), so battery fully charged - turn all LEDs ON to show full charge
        turnAllLedsOn();
      }

      // Check is USB cable has been removed
      if (digitalReadFast(nPG) == HIGH) {  // If USB is corrected there is power and the Power Good pin will be LOW
        DEBUG_PRINTLN("USB power disconnected - Device to idle state and starting up");
        currentState = IDLE;
        turnAllLedsOff();
        startingUp = true;
      }

      break;

  }

  // On powerDown flag power down... and wake up later...
  if (poweringDown && !blinkOn) {
    poweringDown = false;
    sleepRoutine();                   // Go to sleep
    #ifdef TRIGGER_CODE
      digitalWriteFast(EN_HALL, HIGH);  // Power the Hall sensors - ALWAYS - otherwise the release mechanism is triggert...
    #endif  
    if(wakeUpOnButton) {
      wakeUpOrNot(); // Wake up or stay asleep
      while(digitalRead(SW_ON_OFF) == LOW); // Wait for button release
      currentState = IDLE;
    }

    // if(wakeUpOnUSB) {
    //   currentState = CHARGING;
    //   startingUp = true;
    // }
    
    lastLoRaRequest = millis();       // Reset timer for LoRa request - otherwise system might sleep LoRa module too soon
  }


}
