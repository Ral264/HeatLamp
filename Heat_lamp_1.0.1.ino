/***************************************************************************************************
* Programmer: Ryan Lucius                                                                          *
* Email: Ral264@msstate.edu                                                                        *                                        
*                                                                                                  *
* Date created: 4-13-15                                                                            *
* Last modified: 4-20-15                                                                           *
*                                                                                                  *
* Program description: This program is written to control a relay that                             *
* turns off and on a heat lamp for a pet snake in order to regulate temperatures                   *
* based off of time and current temperature, with current temperature overriding                   *
* the amount of time that it has been on. A later version of this code also includes               *
* humidity control, however this code was not added here and is now lost.                          *
*                                                                                                  *
*                                                                                                  * 
* "Timer.h" can be downloaded from:                                                                * 
* https://github.com/JChristensen/Timer                                                            *
*                                                                                                  *
****************************************************************************************************/

#include "Timer.h"
#include <LiquidCrystal.h>
#include <Wire.h>
/* 
relayPin: Assigned to digital pin 11 - controls the relay
to turn power off and on from the lamp
tempPin: Assigned to analog pin 0 - reads the current temperature
update_time: Assigned to 2500ms - is used to control how often the 
ISR timer causes an interrupt to update the state
LEDBLINK_PIN: Assigned to digital pin 3 - is used as the 'heartbeat' pin
to run while the program is running
LEDBLINK_MS: assigned to 1000ms - defines how often the LED blinks on and off
*/

#define relayPin 11 
#define tempPin A0
#define update_time 1000
#define LEDBLINK_PIN 3
#define LEDBLINK_MS 1000

/*
LCD setup:

LCD RS pin set to 10
LCD Enable set to 9
LCD D4 set tp pin 7
LCD D5 set to pin 6
LCD D6 set to pin 5
LCD D7 set to pin 4
LCD A set to 5V -  used for backlight
LCD K set to GND - used for backlight
*/


// LCD is configured for 4-bit mode rather than 8-bit
// The 4 pins on the LCD used are D4, D5, D6, D7
//                RS  E  D4  D5  D6  D7
LiquidCrystal lcd(10, 9, 7,  6,  5,  4); // initialize the lcd display

Timer t; // create a timer object t
float TEMP_C; // store global variable to keep up with current temperature
float READING; // create global variable to read output from pin
byte SECONDS, MINUTES, HOURS; // used to store the current time 
const int DS3231 = 0x68; // the address 0x68 is used for communicating with the clock

// create type state_t to store the possible states
typedef enum {
  LAMP_ON,
  LAMP_OFF,
} state_t;

void setup() {
  lcd.begin(16, 2); // set up the lcd to have 16 colums and 2 rows
  analogReference(INTERNAL); // set analog reference to internal for better accuracy
  pinMode(relayPin, OUTPUT); // set up relayPin for output 
  pinMode(tempPin, INPUT); // set up temperature pin for input
  
  //      time in ms    function to call
  t.every(update_time, update_state); // initialize the timer call update_state() every update_time ms
  Serial.begin(4800); // set the serial port with baud rate 4800
  pinMode(LEDBLINK_PIN, OUTPUT); // set up heartbeat pin
  Wire.begin(); // set up communication between the arduino and the clock
}

void update_state(void){
  readTime();
  char AMPM[2];
  // create e_state as a type state_t to store current state of the machine
  static state_t e_state = LAMP_ON; // turn the lamp on on power up
  
  READING = analogRead(tempPin); // read the current value on the temperature pin
  TEMP_C = (READING / 9.31) - 5; // calculate the temperature in celcius
  
  switch (e_state){
    
    case LAMP_ON:
      lcd.clear(); // clear the display
      digitalWrite(relayPin, HIGH); // turn on the lamp
      Serial.println("ON"); // print "ON" to serial port
      
      Serial.print(TEMP_C); Serial.print(" degrees C, "); // print the temperature in C to serial port
      Serial.print(((9.0/5.0) * TEMP_C) + 32); Serial.println(" degrees F");  // print the temperature in F to serial port
      
      lcd.setCursor(0, 0); // set the lcd to print on line 1
      
      // print the time to the LCD display 
      if (HOURS >= 12){ 
        if (HOURS > 12){
          lcd.print(HOURS - 12); 
        }
        if (HOURS == 12){
          lcd.print(HOURS);
        }
        AMPM[0] = 'P';
        AMPM[1] = 'M';
        lcd.print(":");
      }
      else{
        lcd.print(HOURS);
        lcd.print(":");
        AMPM[0] = 'A';
        AMPM[1] = 'M';
      }
      if (MINUTES < 10){
        lcd.print("0"); // 0 is not automatically added if number is 0 to 9 minutes
      }
      lcd.print(MINUTES);
      lcd.print("::");
      if (SECONDS < 10){
        lcd.print("0"); // 0 is not automatically added if number is 0 to 9 minutes
      }
      lcd.print(SECONDS);
      lcd.print(AMPM[0]);
      lcd.print(AMPM[1]);
      lcd.setCursor(0, 1); // set the lcd to print on line 2
      // print the temperature on the lcd in F
      lcd.print("Temp: ");  
      lcd.print(((9.0/5.0) * TEMP_C) + 32);
      lcd.print("F");
      
      // check if the temperature is over 33C
      if (TEMP_C > 33){
        e_state = LAMP_OFF; // change the state to LAMP_OFF
      }
      
      // turn the lamp off between 8pm and 7:59am
      else if ((HOURS >= 20 || HOURS < 8) && TEMP_C > 22){
        e_state = LAMP_OFF; // change the state to LAMP_OFF
      }
      break;
    
    case LAMP_OFF:
      digitalWrite(relayPin, LOW); // turn the lamp off
      Serial.println("OFF"); // print that the lamp is off to the serial port
      
      lcd.setCursor(0, 0); // set the lcd to print on line 1
      
      // print the time to the LCD display
      if (HOURS >= 12){
        if (HOURS > 12){
          lcd.print(HOURS - 12);
        }
        if (HOURS == 12){
          lcd.print(HOURS);
        }
        lcd.print(":");
        AMPM[0] = 'P';
        AMPM[1] = 'M';
      }
      else{
        lcd.print(HOURS);
        lcd.print(":");
        AMPM[0] = 'A';
        AMPM[1] = 'M';
      }
      if (MINUTES < 10){
        lcd.print("0"); // 0 is not automatically added before the number if time is 0-9 minutes
      }
      lcd.print(MINUTES);
      lcd.print("::");
      if (SECONDS < 10){
       lcd.print("0"); // 0 is not automatically added before the number if time is 0-9 minutes
      } 
      lcd.print(SECONDS);
      lcd.print(AMPM[0]);
      lcd.print(AMPM[1]);
      
      lcd.setCursor(0, 1); // set the lcd to print on line 2
      
      // print the temperature on the lcd in F
      lcd.print("Temp: ");
      lcd.print(((9.0/5.0) * TEMP_C) + 32);
      lcd.print("F");
      
      Serial.print(TEMP_C); Serial.print(" degrees C, "); // print the temperature in C to serial port 
      Serial.print(((9.0/5.0) * TEMP_C) + 32); Serial.println(" degrees F"); // print the temperature in F to serial port
      
      // check to see if the temperature has dropped below 22C
      if (TEMP_C < 22){
        e_state = LAMP_ON; // change the state to LAMP_ON
      } 
      
      // turn the lamp on if it is between 8am and 8pm
      else if ((HOURS >= 8 && HOURS < 20) && (TEMP_C < 31)){
        e_state = LAMP_ON; // change the state to LAMP_ON
      }
      break;

  }
}

/*
doHeartBeat() was used from arduino's ledBlink() code sample to heartbeat throughout
the program. Renamed doHeartBeat() to better follow class protocol. Source:
http://playground.arduino.cc/Main/LEDHeartbeat
*/

void doHeartBeat(){
  static unsigned int ledStatus = LOW; // set the LED initially off
  static unsigned long ledBlinkTime = 0; // start the blink time at 0
  
  if ((long)(millis()-ledBlinkTime) >= 0){
    ledStatus = (ledStatus==HIGH ? LOW : HIGH); // choose to turn LED off or on
    digitalWrite(LEDBLINK_PIN, ledStatus); // set the pin to off or on depending on current state of LED
    ledBlinkTime = millis() + LEDBLINK_MS; // set next time to go off
  }
}

byte decToBcd(byte val) {
  return ((val/10*16) + (val%10));
}
byte bcdToDec(byte val) {
  return ((val/16*10) + (val%16));
}

/*
The real time clock module is connected to analog pins 4 and 5. Communication
is transferred using I2C protocol using the Wire.h library. 
*/

void readTime() {
  Wire.beginTransmission(DS3231); // begin transmission with clock module
  Wire.write(byte(0)); 
  Wire.endTransmission();
  Wire.beginTransmission(DS3231); // begin transmission with clock
  Wire.requestFrom(DS3231, 7); // begin reading from clock
  SECONDS = bcdToDec(Wire.read()); // store seconds
  MINUTES = bcdToDec(Wire.read()); // store minutes
  HOURS = bcdToDec(Wire.read()); //   store hour
  
  // print time to serial monitor for testing
  if (HOURS > 12){
    Serial.print(HOURS - 12);
  }
  else{
    Serial.print(HOURS);
  }
  Serial.print(":");
  if (MINUTES < 10){
    Serial.print("0"); // Serial monitor does not automatically put the '0' in front of the minute if time is below 10 minutes
  }
  Serial.print(MINUTES); Serial.print(":");
  Serial.println(SECONDS);
  Wire.endTransmission(); // end transmission with the clock
}

void loop() {
  /* The timer library requires that the timer be updated constantly
  in the while loop. this allows for the timer to keep up with when to interrupt.
  This timer is used rather than the built in ISR timer, because it simplifies using
  the timer and allows for calling a function, rather than just a single pin */
  
  t.update(); // update the timer
  doHeartBeat(); // blink for the entire time the program is running
}
