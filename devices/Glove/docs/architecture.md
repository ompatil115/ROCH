# System Architecture

The glove has two versions, V1 (USB serial) and V1.5 (Wi-Fi HTTP/JSON). Both run the same control logic on the glove, and only the link to the Raspberry Pi differs. The responsibilities of each layer are described below.

## Layers

**Glove firmware (C++, on ESP32-C3)**: reads the joystick and the MPU-6050 IMU, computes left and right wheel values, sends them to the backend every 50 ms, and drives the vibration motor from the car's proximity telemetry.

- *Joystick (primary):* throttle and steering are combined with a throttle-gated mix and a squared turn curve. The joystick's natural dead zone handles small unintended movement, so the glove adds no hysteresis of its own.
- *Wrist gesture (secondary):* holding the joystick button captures the current wrist angle as zero (press-to-zero). Rotating about 45° either way from that reference maps to pivot speed.
- *Haptics:* vibration intensity follows the proximity data returned from the car.

**Backend (Python/Flask, on Raspberry Pi)**: relays wheel commands from the glove to the car and returns car telemetry to the glove. The backend is a single Flask application shared across all ROCH devices (see [`backend/`](../../../backend/)). Glove traffic is handled under /glove/car (and later /glove/arm), separate from the car and arm routes.

- *V1:* a background serial reader handles the glove's USB serial link alongside the car's.
- *V1.5:* the glove connects over Wi-Fi, so glove traffic arrives as HTTP requests carrying JSON payloads instead of serial lines.

**Car firmware (C++, on Arduino Uno)**: applies the left and right wheel values it receives and returns proximity telemetry. It also enforces the 500 ms command watchdog and the proximity stop, so the car halts safely if the glove stops sending. See the [car architecture](../../Car/docs/architecture.md).

Input mixing is done on the glove, so the backend and car firmware only forward and apply wheel values. This keeps one wheel command format whether the source is the glove or the browser.

## Data flow

**V1 (USB serial):**
Glove → USB Serial → Pi (backend) → Serial → Car Arduino → (wheels, proximity indicator LED)
Car Arduino → Serial (proximity telemetry) → Pi (backend) → USB Serial → Glove (haptics)

**V1.5 (Wi-Fi HTTP/JSON):**
Glove → Wi-Fi → HTTP/JSON → Flask (`/glove/...`) → Serial → Car Arduino → (wheels, proximity indicator LED)
Car Arduino → Serial (proximity telemetry) → Flask → Glove (haptics)

## Development Approach: Serial First, Then Wi-Fi

V1 used USB serial for bench testing. A wired link made it easy to log values and tune the joystick mix, the press-to-zero gesture, and the haptics without network issues. Once the controls and gestures were satisfactory, V1.5 moved the link to Wi-Fi HTTP/JSON so the glove can be worn untethered. Only the transport changed; the control logic did not.

A first-generation scheme mapped wrist pitch and roll directly to wheel speed. Logged trials showed the joystick was a more controllable primary input, so the wrist moved to the secondary pivot role. The earlier code and data are in [`archive/`](../archive/).

## Wiring

See the schematic in [Media](#media) for the full wiring. The IMU and joystick module share the I2C bus; the vibration motor is driven from a GPIO pin.

| Component | Connection |
|---|---|
| MPU-6050 IMU | I2C (SDA: 9, SCL: 8) |
| Arduino Modulino Joystick | I2C (shared bus) |
| Vibration motor | GPIO 10 |

## Media

- **Physical build:** [`media/robot-image.pdf`](media/robot-image.pdf)
- **Schematic:** [`media/schematic.pdf`](media/schematic.pdf)
  ([KiCad source](media/Glove_V1.kicad_sch))
