/*
 * Arduino Mazda RX-8 Head Unit Controller
 *
 * Copyright (c) 2024 Joshua Yewman
 *
 * Arduino based Mazda RX-8 Auxillary control via TAPE/MD port
 *
 * GNU GPLv3
 */

#define DATA_PIN 2

volatile uint8_t buff = 0;           // we only need the first 2 nibbles of any given message
volatile uint8_t buffPos = 0;        // keeps track of which bit we're on
volatile unsigned long int lowTime;  // keeps track of how long the bus was low
volatile unsigned long int highTime; // keeps track of how long the bus was high

void setup()
{
    // set up pins
    pinMode(DATA_PIN, INPUT);
    attachInterrupt(0, isr, CHANGE); // attach the interrupt

    interrupts(); // enable interrupts
    Serial.begin(9600);
}

void loop()
{
    if (buffPos > 7)
    {                       // if full command is ready
        detachInterrupt(0); // don't want interrupt firing when trying to send data
        if (buff == 0x08)
        { // Check if Tape/MD has been called
            delay(10);
            // Power on notification
            sendNibble(0x8); // for head unit
            sendNibble(0x8); // wakeup notification command
            sendNibble(0x1); // wakeup notification data
            sendNibble(0x2); // checksum
            delay(10);
            // Casette present
            sendNibble(0x8); // for head unit
            sendNibble(0xB); // detailed status
            sendNibble(0x9); // data
            sendNibble(0x0); // data
            sendNibble(0x4); // data
            sendNibble(0x0); // data
            sendNibble(0x0); // data
            sendNibble(0xC); // data
            sendNibble(0x3); // checksum
            Serial.println("Tape/MD Check");
        }

        else if (buff == 0x09)
        { // Init Tape/MD
            delay(10);
            // Casette present
            sendNibble(0x8); // for head unit
            sendNibble(0xB); // detailed status
            sendNibble(0x9); // data
            sendNibble(0x0); // data
            sendNibble(0x4); // data
            sendNibble(0x0); // data
            sendNibble(0x0); // data
            sendNibble(0xC); // data
            sendNibble(0x3); // checksum
            delay(10);
            // Tape Playing
            sendNibble(0x8); // for head unit
            sendNibble(0x9); // status
            sendNibble(0x4); // data
            sendNibble(0x1); // data
            sendNibble(0x5); // checksum
            delay(10);
            // Tape Playback
            sendNibble(0x8); // for head unit
            sendNibble(0xB); // detailed status
            sendNibble(0x9); // data
            sendNibble(0x0); // data
            sendNibble(0x4); // data
            sendNibble(0x0); // data
            sendNibble(0x0); // data
            sendNibble(0x1); // data
            sendNibble(0x0); // checksum
            Serial.println("Init Tape/MD");
        }
        else if (buff == 0x01)
        { // Some control command
            delay(10);
            // Tape Playing
            sendNibble(0x8); // for head unit
            sendNibble(0x9); // status
            sendNibble(0x4); // data
            sendNibble(0x1); // data
            sendNibble(0x5); // checksum
            delay(10);
            // Tape Playback
            sendNibble(0x8); // for head unit
            sendNibble(0xB); // detailed status
            sendNibble(0x9); // data
            sendNibble(0x0); // data
            sendNibble(0x4); // data
            sendNibble(0x0); // data
            sendNibble(0x0); // data
            sendNibble(0x1); // data
            sendNibble(0x0); // checksum
            Serial.println("Control command");
        }
        else
        {
            buff = 0;
            buffPos = 0;
        }
        attachInterrupt(0, isr, CHANGE); // re-enable interrupt
    }
}

// function to pull bus low
void dataPullLow(uint8_t pin)
{
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}
void dataFloat(uint8_t pin)
{
    pinMode(pin, INPUT);
}

void isr()
{
    // on pin bus change
    if (!digitalRead(DATA_PIN))
    {                       // if bus low
        lowTime = micros(); // store time when bus went low
        if ((micros() - highTime) > 7500)
        { // clear everything if its been over 7.5ms since last pulse
            buff = 0;
            buffPos = 0;
        }
    }
    else
    { // if bus high
        if (abs((int)(micros() - lowTime) - 1700) < 300)
        {                               // if bus was low for about 1.7 ms
            buff |= 1 << (7 - buffPos); // write 1 to that bit
        }
        buffPos++; // increment buffer position no matter what
        highTime = micros();
    }
}

void sendNibble(uint8_t message)
{
    for (int i = 0; i < 4; i++)
    { // send bits 3-0
        if (message & 1 << (3 - i))
        {                            // if bit to transmit is a 1
            dataPullLow(DATA_PIN);   // pull data pin low
            delayMicroseconds(1700); // wait for appointed time
            dataFloat(DATA_PIN);     // let bus go high again
            delayMicroseconds(1200); // wait for appointed time
        }
        else
        {                            // if bit to transmit is a 0
            dataPullLow(DATA_PIN);   // pull data pin low
            delayMicroseconds(500);  // wait for appointed time
            dataFloat(DATA_PIN);     // let bus go high again
            delayMicroseconds(2400); // wait for appointed time
        }
    }
}