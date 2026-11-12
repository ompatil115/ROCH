# System Architecture

The glove is one firmware (Glove V1) with two links to the Raspberry Pi: Wi-Fi HTTP as the primary link and USB serial as the fallback. The control logic is the same on both, and only the transport differs. The responsibilities of each layer are described below.

## Layers

**Glove firmware (C++, on ESP32-C3)**: reads the MPU-6050 IMU and the two buttons, computes left and right wheel values, sends them to the backend, and drives the vibration motor from the car's proximity telemetry.

- *Orientation:* pitch (down is forward) sets speed and roll (right is a right turn) steers. A dead zone of ±15° ignores small movement. Steering slows the inside wheel rather than reversing it.
- *Neutral:* each button sets the neutral angle for its own axis by averaging 10 IMU readings taken about 5 ms apart. Pitch is also divided by the cosine of the neutral roll (floored at 0.5) so a rolled wrist does not change the pitch reading too much.
- *Press to drive:* wheel values are only non-zero while the roll/safety button is held. On release the glove holds the last command for 1000 ms, sends one stop, then goes silent.
- *Emergency stop:* the pitch button forces zero wheel values while held.
- *Haptics:* vibration intensity follows the proximity data returned from the car, and is off while the glove is not armed.
- *Link:* the glove posts each `WHEELS` line over HTTP when Wi-Fi is up and prints the same line to serial when it is not. It posts `PING` to keep the link alive.

**Backend (Python/Flask, on Raspberry Pi)**: relays wheel commands from the glove to the car and returns car telemetry to the glove. The backend is a single Flask application shared across ROCH devices (see [`backend/`](../../../backend/)).

- *HTTP:* glove messages arrive on `/glove`, and the reply carries the latest `PROX` line.
- *Serial fallback:* a background serial reader handles the glove's USB serial link alongside the car's, and writes `PROX` lines back to it.
- *Priority:* HTTP wins while it has driven within the last second, so the glove can switch links without a restart.

**Car firmware (C++, on Arduino Uno)**: applies the left and right wheel values it receives and returns proximity telemetry. It also enforces the 500 ms command watchdog and the proximity stop, so the car halts safely if the glove stops sending. See the [car architecture](../../Car/docs/architecture.md).

Input mixing is done on the glove, so the backend and car firmware only forward and apply wheel values. This keeps one wheel command format whether the source is the glove or the browser.

## Data flow

**Wi-Fi (primary):**
Glove → Wi-Fi → HTTP POST (`/glove`) → Flask → Serial → Car Arduino → (wheels, proximity indicator LED)
Car Arduino → Serial (proximity telemetry) → Flask → HTTP reply (`PROX`) → Glove (haptics)

**USB serial (fallback):**
Glove → USB Serial → Flask → Serial → Car Arduino
Car Arduino → Serial (proximity telemetry) → Flask → USB Serial → Glove (haptics)

## Design Notes

Glove V1 was built and tested in stages: the IMU, the vibration motor, and the buttons each have their own test sketch, and the serial link was working before the HTTP link was added. HTTP then became the primary link, with serial kept as a fallback.

Over a poor Wi-Fi link only a few commands got through, which showed up as choppy driving. Attaching the XIAO's external antenna fixed it.

## Wiring

| Component | XIAO pin | GPIO |
|---|---|---|
| MPU-6050 SDA | D5 | 7 |
| MPU-6050 SCL | D6 | 21 |
| Vibration motor | D8 | 8 |
| Roll/safety button | D9 | 9 |
| Pitch button | D10 | 10 |

Buttons connect between the pin and GND and use `INPUT_PULLUP`. The LiPo battery connects through a toggle power switch. D8 is GPIO8, a boot strapping pin on the ESP32-C3, so keep the vibration motor circuit from pulling it low at reset.

## Media

- **Schematic:** [`media/schematic.pdf`](media/schematic.pdf) ([KiCad source](media/Glove_V1.kicad_sch))
- **Build photo:** [`media/glove-image.pdf`](media/glove-image.pdf)
- **Demonstration video:** [Glove V1 Drive Demo](https://youtu.be/q82pCMLTnlQ) (more in the [root README](../../../README.md#demonstrations))
