/*
 * Arduino Mazda RX-8 Head Unit Controller
 *
 * Copyright (c) 2024 Joshua Yewman
 *
 * Arduino based Mazda RX-8 SatNav hood tilt control unit. Manages tilt, power-on and sleep tilt.
 *
 * GNU GPLv3
 */

// Define the sleep library
#include <avr/sleep.h>

/*
 * Define boolean fixed vars
 */
const bool OPEN = true;
const bool CLOSE = false;
const bool HOODOPENED = true;
const bool HOODCLOSED = false;

/*
 * Define Pin Usage
 */
const int ACCPIN = 2;       // ACC pin number
const int MOTORENABLE = 3;  // Motor enable pin number
const int TILTPIN = 4;      // Tilt button pin number
const int OPENPIN = 5;      // Open/close button pin number (WAS PIN 6)
const int MOTORDIRBACK = 6; // Motor direction pin number (WAS PIN 10)
const int BTNENABLE = 7;    // Enable control buttons & button illumination (WAS PIN 11)
const int MOTORDIR = 8;     // Motor direction pin number  (WAS PIN 12)

/*
 * Define Fixed Variables
 */
const int HOODOPENEDVALUE = 200; // Analogue potentiometer value when hood is open
const int HOODCLOSEDVALUE = 910; // Analogue potentiometer value when hood is closed
const int HOODPOSTOLERANCE = 10; // Analogue potentiometer value tolerance
const int TILTDURATION = 15;     // Time (ms) to run the motor for a single hood tilt
const int BUTTONDELAY = 400;     // Minimum time between button presses
const int ACCDETECTDELAY = 5000; // Time (ms) that ACC needs to be on before car is considered 'on'
const int MAXTILT = 2;           // Max hood tilt level
const int MAXPOWERTIME = 3000;   // Define the Max Power Time for the arduino, when ACC is dropped, this is the max time until the power relay changes state.

/*
 * Define Global Variables
 */
boolean carOff = true;                  // Initialise carOff state
boolean RPIoff = true;                  // Initalises RPi power state
boolean onHoodStatus = HOODCLOSED;      // Track desired hood status when car on, controlled by checkOpenButton()
boolean currentHoodStatus = HOODCLOSED; // Track current physical hood status, controlled by operateHood()
int openButtonState = 0;                // Initialise openButtonState
int tiltButtonState = 0;                // Initialise tiltButtonState
int tiltLevel = 0;                      // Initialise hood tilt level (0:none - 2:tilted
int carOffTime = 0;                     // Time since ACC off
int motorRunTime = 0;                   // Time the motor has run for
unsigned long onTime;                   // Time since the Ardino has been on

/*
 * Setup the Arduino's initial state, runs once on boot or if the device is reset
 */
void setup()
{
  pinMode(mode0, OUTPUT);
  pinMode(mode1, OUTPUT);
  pinMode(ssPin, OUTPUT);
  // Setup the input pins
  pinMode(OPENPIN, INPUT);       // Set the pushbutton pin as an input:
  pinMode(TILTPIN, INPUT);       // Set the TILTPIN as an input:
  pinMode(ACCPIN, INPUT);        // Set the ACC pin as an input:
  pinMode(BTNENABLE, OUTPUT);    // Set the BTN pin as an output;
  digitalWrite(BTNENABLE, HIGH); // Activeate the pull up for the Buttons
  digitalWrite(OPENPIN, HIGH);   // Activate the pull up for the Open button pin
  digitalWrite(TILTPIN, HIGH);   // Activate the pull up for the Tilt button pin

  // Setup the motor control pins as outputs
  pinMode(MOTORENABLE, OUTPUT);   // Set the motor enable pin as an output
  pinMode(MOTORDIR, OUTPUT);      // Set the motor control pin1 as an output
  pinMode(MOTORDIRBACK, OUTPUT);  // Set the motor control pin2 as an output
  digitalWrite(MOTORENABLE, LOW); // Disable the motor

  // Start serial communication at 9600 bits per second for debugging
  Serial.begin(9600);
  Serial.println("RX-8 Navhood control running...");
}

/*
 * The main function, runs continuously
 */
void loop()
{
  if (digitalRead(ACCPIN) == HIGH)
  {                    // Accessories are on (car on)
    checkResume();     // Check if the car was previously off
    checkOpenButton(); // Check if the Open button has been pressed
    checkTiltButton(); // Check if the Tilt button has been pressed
  }
  else
  { // Accessories are off (car off)
    if (carOff)
    {
      // Serial.println("Car off time" + carOffTime);
      checkCarOffTime();
    }
    else
    {
      checkOff();
    }
  }
}

/*
 * Check if we are resuming from a 'carOff' event, restore previous hood position
 */
void checkResume()
{
  if (carOff == true)
  {                        // Only run if car off
    delay(ACCDETECTDELAY); // Wait a ACCDETECTDELAY time
    if (digitalRead(ACCPIN) == HIGH)
    { // Check to see if Accessories are still on
      if (onHoodStatus == HOODOPENED)
      {
        operateHood(OPEN, false); // Restore the previous hood position
        restorePosition();
      }
      carOff = false;
      carOffTime = 0;
    }
  }
}

/*
 * Check if the Open button has been pressed and perform the open action
 */
void checkOpenButton()
{
  openButtonState = digitalRead(OPENPIN); // Get the current state of the Open button
  if (openButtonState == LOW)
  { // Button has been pressed
    if (analogRead(A5) < (HOODCLOSEDVALUE - HOODPOSTOLERANCE))
    { // Hood is open
      operateHood(CLOSE, false);
      onHoodStatus = HOODCLOSED;
    }
    else
    { // Hood is closed
      operateHood(OPEN, false);
      restorePosition();
      onHoodStatus = HOODOPENED;
    }
    delay(BUTTONDELAY); // A delay so we have time to capture the button release
  }
}

/*
 * Check if the Tilt button has been pressed and perform the tilt action
 */
void checkTiltButton()
{
  tiltButtonState = digitalRead(TILTPIN); // Get the current state of the Tilt button
  if (tiltButtonState == LOW)
  { // Button has been pressed
    if (currentHoodStatus == HOODOPENED)
    { // Hood is currently physically open
      if (tiltLevel == MAXTILT)
      { // If at max tilt, return to fully opened
        operateHood(OPEN, false);
        tiltLevel = 0;
      }
      else
      { // Otherwise, tilt it
        operateHood(CLOSE, true);
        tiltLevel++;
      }
      delay(BUTTONDELAY); // A delay so we have time to capture the button release
    }
  }
}

/*
 * Check if the car is off and closes the hood
 */
void checkOff()
{
  delay(ACCDETECTDELAY); // Wait a ACCDETECTDELAY time
  if (digitalRead(ACCPIN) == LOW)
  { // Check if Accessories is still off
    if (analogRead(A5) < (HOODCLOSEDVALUE - HOODPOSTOLERANCE))
    { // Hood is open, close it
      operateHood(CLOSE, false);
    }
    carOff = true;
    carOffTime += 2;
  }
}

/*
 * Check how long the car has been off and puts the Arduino to sleep
 */
void checkCarOffTime()
{
  if (carOffTime == 50)
  {
    sleepNow(); // Make the Arduino go into sleep mode
  }
  delay(2000);
  Serial.print("carOffTimer: ");
  Serial.println(carOffTime);
  carOffTime += 2; // Increment the car off timer (s)
}

/*
 * Restore the previous tilt position if any
 */
void restorePosition()
{
  for (int i = 0; i < tiltLevel; i++)
  { // Tilt to previous desired level if any
    delay(BUTTONDELAY);
    operateHood(CLOSE, true);
  }
}

/*
 * Operate the navigation hood by driving the motor
 *
 * Args:
 *    dir: A boolean determining the direction to drive the hood (OPEN or CLOSE)
 *    tilt: A boolean flag to signify if we only want to tilt the hood
 */
void operateHood(bool dir, bool tilt)
{
  if (tilt)
  { // If we only want to tilt the hood, dir is ignored
    Serial.println("TILT");
    digitalWrite(MOTORDIR, LOW);     // Set motor drive direction to close
    digitalWrite(MOTORENABLE, HIGH); // Enable the motor
    delay(TILTDURATION);             // Keep motor running for TILTDURATION
    digitalWrite(MOTORENABLE, LOW);  // Stop the motor
  }
  else
  {
    if (dir)
    { // If direction is OPEN
      String debugString = "Opening hood, potentiometer: ";
      Serial.println(debugString + analogRead(A5));
      digitalWrite(MOTORDIRBACK, LOW);
      digitalWrite(MOTORDIR, HIGH);    // Set motor drive direction to open
      digitalWrite(MOTORENABLE, HIGH); // Enable the motor
      while (analogRead(A5) > HOODOPENEDVALUE && motorRunTime < 35)
      { // Run until we're fully open. If it's run for over 3s stop
        delay(100);
        motorRunTime++;
      }
      digitalWrite(MOTORENABLE, LOW); // Stop the motor
      motorRunTime = 0;
      currentHoodStatus = HOODOPENED;
      Serial.println("Done: Hood Open");
    }
    else
    { // If direction is CLOSE
      String debugString = "Closing hood, potentiometer: ";
      Serial.println(debugString + analogRead(A5));
      digitalWrite(MOTORDIR, LOW); // Set motor drive direction to close
      digitalWrite(MOTORDIRBACK, HIGH);
      digitalWrite(MOTORENABLE, HIGH); // Enable the motor
      while (analogRead(A5) < HOODCLOSEDVALUE && motorRunTime < 35)
      { // Run until we're fully closed. If it's run for over 3s stop
        delay(100);
        motorRunTime++;
      }
      digitalWrite(MOTORENABLE, LOW); // Stop the motor
      motorRunTime = 0;
      currentHoodStatus = HOODCLOSED;
      Serial.println("Done: Hood Close");
    }
  }
}

/*
 * Setup an interrupt and enter sleep mode
 */
void sleepNow()
{
  Serial.println("Entering sleep mode");
  digitalWrite(BTNENABLE, LOW);
  delay(100);
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // Set type of sleep mode
  sleep_enable();                      // Enable sleep mode
  attachInterrupt(0, wakeUp, HIGH);    // Use interrupt 0 (pin 2 ie. ACC input to wake device)
  sleep_mode();                        // Put device to sleep
  sleep_disable();                     // Execution resumes from here after waking up
  detachInterrupt(0);
  delay(100);
  Serial.println("Resuming from Sleep");
  digitalWrite(BTNENABLE, HIGH); // Enable the buttons and illumination
}

/*
 * The wakeUp() interrupt service routine will run when we get input from ACC (pin 2)
 * Since we just want the device to wake up we do nothing here
 */
void wakeUp()
{
}
