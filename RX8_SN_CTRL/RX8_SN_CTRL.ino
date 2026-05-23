/*
 * Arduino Mazda RX-8 Head Unit Controller
 *
 * Copyright (c) 2024 Joshua Yewman
 *
 * Arduino based Mazda RX-8 SatNav hood tilt control unit. Manages tilt,
 * power-on and sleep tilt.
 *
 * GNU GPLv3
 */

#include <avr/sleep.h>

// Hood direction
const bool OPEN  = true;
const bool CLOSE = false;

// Hood physical state
const bool HOODOPENED = true;
const bool HOODCLOSED = false;

/*
 * Pin assignments
 */
const uint8_t ACCPIN       = 2;   // ACC input (ignition accessory line, PCINT18)
const uint8_t TILTPIN      = 4;   // Tilt button
const uint8_t OPENPIN      = 6;   // Open/close button
const uint8_t MOTORDIRBACK = 10;  // DRV8871 IN2
const uint8_t BTNENABLE    = 11;  // Button enable + illumination
const uint8_t MOTORDIR     = 12;  // DRV8871 IN1
const uint8_t HOODPOSPIN   = A5;  // Hood-position potentiometer

/*
 * Tunables
 */
const int HOODOPENEDVALUE  = 980;   // Below lowest observed reading (~982) — watchdog drives motor into mechanical stop
const int HOODCLOSEDVALUE  = 1017;  // Above mechanical max (~1015) — same pattern, watchdog handles close
const int HOODPOSTOLERANCE = 3;     // Pot reading tolerance — small because operational span is only ~29 counts
const int TILTDURATION     = 150;  // Motor run-time per tilt step (ms)
const int BUTTONDELAY      = 400;  // Min time between button presses (ms)
const int MAXTILT          = 2;    // Max tilt level

const unsigned long ACCDETECTDELAY     = 5000UL;   // ACC must be stable this long (ms)
const unsigned long SLEEP_AFTER_OFF_MS = 50000UL;  // Time after car-off before MCU sleeps (ms)
const int           MOTOR_TIMEOUT_MS   = 3500;     // Motor watchdog (ms)
const int           MOTOR_POLL_MS      = 100;      // Pot poll interval while motor runs (ms)
const int           BRAKE_DURATION_MS  = 50;      // Active brake hold after each motor stop (ms)

/*
 * State
 */
bool carOff             = true;        // Whether the car is currently considered off
bool onHoodStatus       = HOODCLOSED;  // Desired hood state when car is on
bool currentHoodStatus  = HOODCLOSED;  // Last known physical hood state
int  tiltLevel          = 0;           // Current tilt level (0..MAXTILT)
unsigned long carOffStartMs = 0;       // millis() when car-off was confirmed

void setup()
{
  pinMode(OPENPIN,   INPUT_PULLUP);
  pinMode(TILTPIN,   INPUT_PULLUP);
  pinMode(ACCPIN,    INPUT);            // Externally driven by ACC line
  pinMode(BTNENABLE, OUTPUT);
  digitalWrite(BTNENABLE, HIGH);        // Power buttons + illumination

  pinMode(MOTORDIR,     OUTPUT);
  pinMode(MOTORDIRBACK, OUTPUT);
  digitalWrite(MOTORDIR,     LOW);  // Coast (both IN1/IN2 LOW)
  digitalWrite(MOTORDIRBACK, LOW);

  Serial.begin(9600);
  Serial.println(F("RX-8 Navhood control running..."));
}

void loop()
{
  // Mirror ACC straight onto BTNENABLE: buttons (and their LED illumination)
  // are live only while ACC is HIGH. The moment ACC drops, the buttons go
  // dead — no waiting for the off-timer to expire.
  digitalWrite(BTNENABLE, digitalRead(ACCPIN));

  if (digitalRead(ACCPIN) == HIGH)
  {
    checkResume();
    checkOpenButton();
    checkTiltButton();
  }
  else if (carOff)
  {
    checkCarOffTime();
  }
  else
  {
    checkOff();
  }
}

/*
 * Resuming from a previously-off state. Confirm ACC stays HIGH continuously
 * for ACCDETECTDELAY, then restore the previous hood position. Bails out
 * immediately if ACC drops, so a momentary blip doesn't cost 5 seconds.
 */
void checkResume()
{
  if (!carOff) return;
  if (!accStableFor(HIGH, ACCDETECTDELAY)) return;

  if (onHoodStatus == HOODOPENED)
  {
    operateHood(OPEN, false);
    restorePosition();
  }
  carOff = false;
}

/*
 * Open/close button: toggle the hood.
 */
void checkOpenButton()
{
  if (digitalRead(OPENPIN) != LOW) return;

  if (analogRead(HOODPOSPIN) < (HOODCLOSEDVALUE - HOODPOSTOLERANCE))
  {
    operateHood(CLOSE, false);
    onHoodStatus = HOODCLOSED;
  }
  else
  {
    operateHood(OPEN, false);
    restorePosition();
    onHoodStatus = HOODOPENED;
  }
  delay(BUTTONDELAY);
}

/*
 * Tilt button: step the tilt one notch, wrapping back to fully open at MAXTILT.
 * Only acts if the hood is physically open.
 */
void checkTiltButton()
{
  if (digitalRead(TILTPIN) != LOW) return;
  if (currentHoodStatus != HOODOPENED) return;

  if (tiltLevel >= MAXTILT)
  {
    operateHood(OPEN, false);
    tiltLevel = 0;
  }
  else
  {
    operateHood(CLOSE, true);
    tiltLevel++;
  }
  delay(BUTTONDELAY);
}

/*
 * Confirm ACC has been LOW continuously for ACCDETECTDELAY, then close the
 * hood and start the off-timer.
 */
void checkOff()
{
  if (!accStableFor(LOW, ACCDETECTDELAY)) return;

  if (analogRead(HOODPOSPIN) < (HOODCLOSEDVALUE - HOODPOSTOLERANCE))
  {
    operateHood(CLOSE, false);
  }
  carOff = true;
  carOffStartMs = millis();
}

/*
 * Once the car has been off for SLEEP_AFTER_OFF_MS, put the MCU to sleep.
 */
void checkCarOffTime()
{
  const unsigned long elapsed = millis() - carOffStartMs;
  if (elapsed >= SLEEP_AFTER_OFF_MS)
  {
    sleepNow();
    return;
  }

  static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 2000)
  {
    lastPrint = millis();
    Serial.print(F("carOffTimer (s): "));
    Serial.println(elapsed / 1000UL);
  }
}

/*
 * After re-opening, restore any tilt level the user had set previously.
 */
void restorePosition()
{
  for (int i = 0; i < tiltLevel; i++)
  {
    delay(BUTTONDELAY);
    operateHood(CLOSE, true);
  }
}

/*
 * Drive the hood motor.
 *   dir:  OPEN or CLOSE (ignored when tilt == true)
 *   tilt: true performs a single tilt step
 */
void operateHood(bool dir, bool tilt)
{
  if (tilt)
  {
    Serial.println(F("TILT"));
    digitalWrite(MOTORDIR,     LOW);   // IN1 = L
    digitalWrite(MOTORDIRBACK, HIGH);  // IN2 = H -> reverse (close direction)
    delay(TILTDURATION);
    motorBrake();
    return;
  }

  if (dir == OPEN)
  {
    Serial.print(F("Opening hood, potentiometer: "));
    Serial.println(analogRead(HOODPOSPIN));
    digitalWrite(MOTORDIRBACK, LOW);   // IN2 = L
    digitalWrite(MOTORDIR,     HIGH);  // IN1 = H -> forward
    runMotorUntil(true);
    motorBrake();
    currentHoodStatus = HOODOPENED;
    Serial.println(F("Done: Hood Open"));
  }
  else
  {
    Serial.print(F("Closing hood, potentiometer: "));
    Serial.println(analogRead(HOODPOSPIN));
    digitalWrite(MOTORDIR,     LOW);   // IN1 = L
    digitalWrite(MOTORDIRBACK, HIGH);  // IN2 = H -> reverse
    runMotorUntil(false);
    motorBrake();
    currentHoodStatus = HOODCLOSED;
    Serial.println(F("Done: Hood Close"));
  }
}

/*
 * Active dynamic brake, then release to coast. Both IN1/IN2 HIGH shorts the
 * motor terminals through the DRV8871's low-side FETs so the rotor's kinetic
 * energy dumps into the windings as heat instead of bouncing the mechanism
 * back. After BRAKE_DURATION_MS, release to coast (both LOW) so no FETs are
 * left energised while idle.
 */
void motorBrake()
{
  digitalWrite(MOTORDIR,     HIGH);
  digitalWrite(MOTORDIRBACK, HIGH);
  delay(BRAKE_DURATION_MS);
  digitalWrite(MOTORDIR,     LOW);
  digitalWrite(MOTORDIRBACK, LOW);
}

/*
 * Hold the motor on until the hood reaches its endpoint or the watchdog fires.
 *   opening: true waits for pot to drop past HOODOPENEDVALUE,
 *            false waits for pot to rise past HOODCLOSEDVALUE.
 */
void runMotorUntil(bool opening)
{
  int elapsed = 0;
  while (elapsed < MOTOR_TIMEOUT_MS)
  {
    int pos = analogRead(HOODPOSPIN);
    if (opening ? (pos <= HOODOPENEDVALUE) : (pos >= HOODCLOSEDVALUE)) return;
    delay(MOTOR_POLL_MS);
    elapsed += MOTOR_POLL_MS;
  }
}

/*
 * Busy-wait until ACC has held the requested level continuously for `windowMs`.
 * Returns false the moment the level breaks, so callers don't burn the full
 * window on a glitch.
 */
bool accStableFor(uint8_t level, unsigned long windowMs)
{
  const unsigned long start = millis();
  while (millis() - start < windowMs)
  {
    if (digitalRead(ACCPIN) != level) return false;
  }
  return true;
}

/*
 * Power down the MCU. Uses pin-change interrupt (PCINT18 = D2) instead of INT0
 * because INT0 in SLEEP_MODE_PWR_DOWN can only wake on level-LOW, which would
 * fire instantly (ACC is LOW when we sleep). PCINT wakes on any edge, so any
 * change to ACC brings the chip back; loop() then re-evaluates the state.
 */
void sleepNow()
{
  Serial.println(F("Entering sleep mode"));
  Serial.flush();              // UART clocks halt in PWR_DOWN

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  cli();
  sleep_enable();
  PCMSK2 |= (1 << PCINT18);    // Watch D2 for changes
  PCIFR  |= (1 << PCIF2);      // Clear any latched flag
  PCICR  |= (1 << PCIE2);      // Enable PCINT2 group
  sei();
  sleep_cpu();
  // --- resumes here on wake ---
  sleep_disable();
  PCICR  &= ~(1 << PCIE2);
  PCMSK2 &= ~(1 << PCINT18);

  Serial.println(F("Resuming from Sleep"));
}

/*
 * Pin-change ISR for PORTD. We only need the MCU to exit sleep — loop()
 * handles the rest — so use EMPTY_INTERRUPT (just a bare `reti` in asm,
 * no C prologue/epilogue). This also sidesteps an LTO inliner pathology
 * in AVR-GCC 7.3 that hangs on `ISR(...) {}` with surrounding register code.
 */
EMPTY_INTERRUPT(PCINT2_vect);
