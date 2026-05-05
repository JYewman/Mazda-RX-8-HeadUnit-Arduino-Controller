# Mazda RX-8 Head Unit Controller

## About

This project controls the motorised hood on the Mazda RX-8 SatNav screen and
provides an auxiliary input plus custom LCD text feed to the existing RX-8
head unit. It replaces the OEM SatNav hood controller and adds a small
companion controller for the aux input and LCD display.

The reason it exists: the RX-8 is a classic, but its SatNav and head unit
are showing their age. This project lets you remove the original SatNav
internals, add a tablet (or a Raspberry Pi running OpenAuto Pro), and still
keep the motorised hood tilt working. It also adds a much-needed (and
cheap) aux input to head units running firmware 10.1.

You can pick and choose. The SatNav hood controller works as a standalone
unit, and so does the aux input system — the LCD control is optional. The
two Arduinos can also talk to each other for shared LCD display features
and future functionality.

## What's in this repo

| Path | Purpose |
| --- | --- |
| [RX8_SN_CTRL/](RX8_SN_CTRL/) | SatNav hood controller — drives the motor for open / close / tilt, sleeps when ACC is off |
| [RX8_HU_CTRL/](RX8_HU_CTRL/) | Head-unit companion — auxiliary audio input switching and custom LCD text |
| [WIRING.md](WIRING.md) | Full wiring guide for the SatNav hood controller |

## Getting started

The hood controller is an ATmega328P (Arduino Pro Mini) driving a DRV8871
H-bridge that runs the OEM hood motor. ACC sensing wakes it from deep
sleep when the car turns on. The full wiring is documented in
[WIRING.md](WIRING.md).

### Prerequisites

Software:

- Arduino IDE (VS Code with the Arduino extension also works)

Hardware for the hood controller:

- Arduino Pro Mini 5 V / 16 MHz (with the power LED removed for low sleep current)
- FT232 / FTDI USB-serial breakout (for programming the Pro Mini — it has no onboard USB)
- DRV8871 motor driver breakout (single H-bridge, 6.5–45 V, ~3.6 A peak)
- Automotive-rated 12 V → 5 V DC-DC converter (e.g. CPT Car Power Technology 15 W)
- The OEM SatNav hood mechanism (motor + position potentiometer)

Hardware for the head-unit companion is optional and documented in the
comments of [RX8_HU_CTRL/RX8_HU_CTRL.ino](RX8_HU_CTRL/RX8_HU_CTRL.ino).

### Programming the Pro Mini

The Pro Mini has no onboard USB, so it's flashed via an external FT232 /
FTDI breakout connected to the 6-pin header at one end of the board (set
the FTDI's voltage jumper to **5 V**). In the Arduino IDE:

- **Board** → Arduino Pro or Pro Mini
- **Processor** → ATmega328P (5 V, 16 MHz)
- **Port** → the FTDI's USB-serial port

Open the Serial Monitor at 9600 baud to see the controller's debug output.
The full FTDI hookup table is in
[WIRING.md → Programming the Pro Mini](WIRING.md#programming-the-pro-mini).

### Power and sleep behaviour

The hood controller uses `avr/sleep.h` to put the Pro Mini into
`SLEEP_MODE_PWR_DOWN` once ACC has been off for ~50 seconds. With the
power LED removed the Pro Mini draws ~10 µA in sleep, plus another ~5 µA
from the DRV8871. The controller alone will sit happily on a healthy car
battery for months — but **I AM NOT RESPONSIBLE FOR YOUR BATTERY DYING.**
If the car is going to sit for weeks unused, disconnect the battery (or at
least the controller). The car has its own parasitic loads regardless.

### Do's, don'ts and gotchas

- **Don't run the Pro Mini directly off the car's +12 V rail.** Use an
  automotive 12 V → 5 V DC-DC converter and feed the regulated output to
  the Pro Mini's **VCC** pin, *not* RAW. RAW goes through the onboard LDO
  with ~1 V dropout — 5 V in gives roughly 4 V on VCC, below comfortable
  operating margin.
- **Don't drive the motor from the Arduino's 5 V rail.** Use the DRV8871.
  The onboard regulator can't supply motor current and will damage itself
  if you try.
- **Don't connect the car's +12 V accessory rail directly to an Arduino
  input pin** for ignition sensing — it will destroy the input. Take ACC
  off a 5 V source: another Arduino, a Raspberry Pi running OpenAuto Pro,
  or a small 5 V regulator.
- **Speaker noise** is almost always coming from a switching regulator
  somewhere in the system. Where you can, use ground-loop isolators on
  audio paths and bring all grounds back to the chassis at a single star
  point (see [WIRING.md → Star ground](WIRING.md#star-ground)).

## Contributing

PRs welcome. For anything beyond a small fix, please open an issue first
so we can discuss the approach.

<!-- LICENSE -->
## License

Distributed under the GNU GPL v3.0. See [LICENSE](LICENSE) for the full
text.
