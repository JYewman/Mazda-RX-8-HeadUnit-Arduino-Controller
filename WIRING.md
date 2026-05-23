# RX-8 SatNav Hood Controller — Wiring

Wiring guide for the Arduino-based SatNav hood controller built around a
DRV8871 motor driver, with permanent-12 V power and an ACC-sense input from
the Raspberry Pi head unit.

## Bill of materials

| Item | Notes |
| --- | --- |
| Arduino Pro Mini 5 V / 16 MHz | Power LED removed for ~10 µA sleep current. Programmed via an external FT232 / FTDI breakout. |
| DRV8871 motor driver breakout | Single H-bridge, 6.5–45 V, ~3.6 A peak |
| CPT Car Power Technology 12 V → 5 V 15 W DC-DC converter | Automotive-rated, 5 V @ 3 A. Has its own load-dump suppression and reverse-polarity protection on the converter side. |
| Electrolytic cap, **220 µF / 35 V** | Bulk decoupling at the DRV8871 VM input |
| Ceramic cap, **100 nF** | High-frequency decoupling beside the bulk cap |
| Resistor, **10 kΩ** | Pull-down on the ACC sense line (D2 → GND) |
| Push buttons × 2 | Open/close, Tilt (illuminated, switched via BTNENABLE) |
| 10 kΩ linear pot, mechanically linked to hood | Position feedback (already in the OEM mechanism) |

## Pin assignments

These match the constants in [RX8_SN_CTRL.ino](RX8_SN_CTRL/RX8_SN_CTRL.ino):

| Pro Mini pin | Code constant | Wired to | Purpose |
| --- | --- | --- | --- |
| D2 | `ACCPIN` | RPi 5 V ACC line + 10 kΩ pull-down to GND | Wakes MCU from sleep on ACC change |
| D4 | `TILTPIN` | Tilt button (other side → BTNENABLE) | Active-low, internal pull-up |
| D6 | `OPENPIN` | Open/close button (other side → BTNENABLE) | Active-low, internal pull-up |
| D10 | `MOTORDIRBACK` | DRV8871 **IN2** | Motor direction line B |
| D11 | `BTNENABLE` | Common side of both buttons + button LEDs | Powers buttons/illumination only when awake |
| D12 | `MOTORDIR` | DRV8871 **IN1** | Motor direction line A |
| A5 | `HOODPOSPIN` | Hood-position pot wiper | Hood travel feedback (200 = open, 910 = closed) |
| VCC | — | Buck 5 V output, pot top, button LEDs (via BTNENABLE) | **Not RAW** — see power architecture |
| GND | — | Common ground (chassis) | See star-ground section |

D3 is **unused** (formerly `MOTORENABLE` — DRV8871 has no enable pin).

## Power architecture

```text
  +12 V perm tap ──┬── DRV8871 "Power +" (VM)
  (cabin fusebox,  │
   already fused)  └── CPT 12 V→5 V converter (input)
                              │
                          converter output (5 V)
                              │
                              └── Pro Mini VCC pin
                                     │
  Chassis ── star ground ◄──── Pro Mini GND ◄─┘
                              │
                              ├── DRV8871 "Power −" (heavy gauge)
                              ├── DRV8871 GND header
                              ├── Buck input/output GND (same node internally)
                              └── RPi GND (so the ACC signal shares a reference)

  RPi 5 V (ACC) ──── D2 ──── 10 kΩ ──── GND
                          (Pro Mini input, pull-down)
```

Key points:

- **5 V into the Pro Mini's VCC pin**, **not** RAW. RAW goes through the
  onboard LDO with ~1 V of dropout — feeding 5 V there gives roughly 4 V on
  VCC, below comfortable operating margin. VCC bypasses the regulator
  entirely, which is what you want here.
- The buck's input GND and output GND are the **same node** on every
  non-isolated module — you don't need to bond them externally, but they
  do need to reach the chassis star point.
- The 12 V feed to the DRV8871 is **permanent**, not switched through ACC.
  The motor driver sleeps at ~5 µA when both inputs are LOW, so there's
  no parasitic drain.

## DRV8871 module wiring

The board has two screw-terminal pairs and a 4-pin logic header.

| Terminal | Connect to |
| --- | --- |
| **Power +** (screw) | Switched output of fuse + reverse-polarity diode, with bulk cap to GND |
| **Power −** (screw) | Chassis (heavy-gauge wire — motor return current) |
| **Motor 1** (screw) | Hood motor lead A (swap with Motor 2 if open/close come out reversed) |
| **Motor 2** (screw) | Hood motor lead B |
| **GND** (header) | Chassis (light-gauge — logic reference) |
| **VM** (header) | **Leave unconnected** — same node as Power + |
| **IN1** (header) | Pro Mini D12 (`MOTORDIR`) |
| **IN2** (header) | Pro Mini D10 (`MOTORDIRBACK`) |

Direction truth table (per DRV8871 datasheet):

| IN1 | IN2 | Behaviour |
| --- | --- | --- |
| L | L | Coast (motor disconnected) — the stop state |
| H | L | Forward — used for OPEN |
| L | H | Reverse — used for CLOSE / TILT |
| H | H | Brake (both low-side FETs on) — unused |

## Buttons

Both buttons share the `BTNENABLE` line so they go dead during sleep, saving
the LED illumination current and stopping accidental wake-up.

```text
   D11 (BTNENABLE) ──┬── 220 Ω ── LED ── (other LED leg) ── GND
                     │
                     └── button common ──┬── Tilt button NO ── D4
                                         └── Open button NO ── D6
```

Internal pull-ups are enabled on D4 and D6 (`INPUT_PULLUP`), so when
`BTNENABLE` is HIGH and a button is pressed, the corresponding input is
pulled to ~0 V. When `BTNENABLE` is LOW (sleep), the pull-up still keeps the
inputs at 5 V even if a button is pressed — pressing a button while asleep
does nothing.

## ACC sense input (D2)

This is the line that wakes the MCU from sleep. It needs two things:

1. **Pull-down (10 kΩ)** between D2 and GND — when the RPi is unpowered
   (car off), the line is otherwise floating and PCINT will false-wake on
   noise. The ATmega328P has no internal pull-down (only pull-up), so
   this has to be external.
2. **Common ground with the RPi** — without it the "5 V" from the RPi has
   no defined relationship to the Pro Mini's logic reference.

## Star ground

Motor current and logic current share a chassis tie point but never share
copper:

```text
                          ┌── (thick) ── DRV8871 Power −
   Battery − = chassis ───┼── (thick) ── Buck input GND
   bolt                   └── (thin)  ── Pro Mini GND, RPi GND, signal grounds
```

If logic GND piggybacks off the motor return wire, the motor's voltage drop
shows up as a wobble on the Pro Mini's "0 V" reference, which can cause
spurious resets or analog reads when the motor runs. With a single chassis
bolt as the meeting point, the motor-current loop and the logic-current loop
share no resistive path.

## Programming the Pro Mini

The Pro Mini has no onboard USB. Flash with an external FT232 / FTDI
breakout via the 6-pin header at one end of the board:

| FTDI | Pro Mini |
| --- | --- |
| VCC (set jumper to **5 V**) | VCC |
| GND | GND |
| TX | RXI |
| RX | TXO |
| DTR | DTR / GRN |
| CTS | not connected |

In the Arduino IDE: **Board → Arduino Pro or Pro Mini**, **Processor →
ATmega328P (5 V, 16 MHz)**, then upload as normal. The Serial Monitor at
9600 baud shows the controller's debug output.

## Sleep current expectation

With the power LED removed and ACC LOW for >50 s, the Pro Mini should drop
to **~10 µA** in `SLEEP_MODE_PWR_DOWN`. The DRV8871 adds another ~5 µA.

To measure: power VCC through a multimeter set to mA/µA, pull D2 to GND, and
wait for the off-timer to expire.

| Reading | Likely cause |
| --- | --- |
| ~10 µA | Expected — sleeping correctly |
| ~150 µA | Brown-out detector enabled (acceptable, harmless) |
| ~3 mA | Power LED still in circuit |
| > 5 mA | Not actually sleeping — check that the off-timer fired and `sleepNow()` was reached |

## Whole-system current at the 12 V input

The table above is the **per-Pro Mini** number — what you'd measure at the
chip's VCC with everything else disconnected. What the **car battery
actually sees**, with the buck converter in the loop, is different:

- **~25–35 mA** at idle (ACC off, Pro Mini sleeping), dominated entirely
  by the CPT buck converter's no-load quiescent draw.
- The Pro Mini and DRV8871 combined contribute essentially nothing at
  this scale (~15 µA total). If the system-side reading is healthy but
  you suspect the Pro Mini isn't sleeping, fall back to the per-Pro Mini
  diagnostic above.

To measure: multimeter in series on the +12 V feed into the buck
converter, ACC held LOW (or disconnected from D2), and wait out the
sleep timer.

Sanity check on battery life: 30 mA × 24 h ≈ 720 mAh/day. A healthy
60 Ah RX-8 battery has roughly 30 Ah of usable depth-of-discharge before
risking start failures, so a weekly-driven car never sees the bottom of
it. Cars that sit unused for weeks at a time should be on a trickle
charger regardless of this controller.

## Pre-flight checklist

Before the first power-up:

- [ ] +12 V tap pulled from a fused cabin permanent circuit (note the rating)
- [ ] Wire gauge to the controller sized for the upstream cabin fuse rating
- [ ] Wiring polarity double-checked — there's no reverse-polarity protection
- [ ] Buck output goes to **Pro Mini VCC**, never RAW
- [ ] All grounds meet at one chassis bolt (motor return on its own thick wire)
- [ ] 10 kΩ pull-down installed between D2 and GND
- [ ] Tap point verified with a multimeter as 5 V (ACC on) / 0 V (ACC off) before connecting to D2
- [ ] DRV8871 "VM" header pin **not** connected
- [ ] Bulk cap (220 µF) and 100 nF ceramic close to DRV8871 Power +
- [ ] Power LED removed from the Pro Mini
- [ ] Hood pot reads ~200 fully open and ~910 fully closed (open the serial
      monitor at 9600 baud and confirm before connecting the motor)
- [ ] First motor test on a bench supply with current limit set to 1 A — not
      the car battery. Confirm direction matches `OPEN` / `CLOSE` before
      installing.
