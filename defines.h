
//Comment out the below line to have debug messages on the serial
#define DEBUG_ENABLED

//Comment out the below line to program for Foedraal
#define TRIGGER_CODE 

#ifdef DEBUG_ENABLED
#define DEBUG_BEGIN Serial.begin(115200)
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_DELAY(x) delay(x)
#else
#define DEBUG_BEGIN
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_DELAY(x)
#endif

#define LoRa Serial2

// colors for RGB LED
#define BLACK 0b00000000
#define RED 0b00000100
#define GREEN 0b00000010
#define BLUE 0b00000001
#define MAGENTA 0b00000101
#define YELLOW 0b00000110
#define CYAN 0b00000011
#define WHITE 0b00000111

#define SW_ON_OFF 2
#define LED1 3
#define LED2 4
#define LED3 5
#define LED4 6
#define LED5 7 // not in use, but defined to set to input - pull up

#define MEAS_BATT 8
#define nAMP_SHDN 9
#define EN_HALL 10

#define NOT_USED11 11
#define NOT_USED12 12
#define NOT_USED13 13

#define LED_BLUE 14
#define LED_GREEN 15
#define LED_RED 16
#define DIM_RGB 17

#define NOT_USED18 18
#define NOT_USED19 19

#define nPG 20          // nPG (Power Good Pin): Indicates valid input power (3-V to 6.6-V). It is low when input power is valid (power is good) and high-impedance when the input voltage is below the valid threshold or absent.

#define NOT_USED21 21

#define BATT_VOLT A0 //D22
#define I_MOTOR1 A1  //D23
#define I_MOTOR2 A2  //D24
#define CHARGE_STAT 25. // STAT (Status Pin): Indicates the charging state. It is typically low during charging and high-impedance when charging is complete or disabled.

#define NOT_USED26 26

#define HALL_1 27
#define HALL_2 28

#define NOT_USED29 29

#define M1_IN1 30
#define M1_IN2 31
#define M2_IN1 32
#define M2_IN2 33

#define LORA_WAKEUP 36

#define NOT_USED37 37

#define RUMBLE_ON 38
#define PIEZO 39