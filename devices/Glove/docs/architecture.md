# System Architecture

The glove has two versions, V1 (USB serial) and V1.5 (Wi-Fi HTTP/JSON). Both run the same control logic on the glove, and only the link to the Raspberry Pi differs. The responsibilities of each layer are described below.

## Layers

**Glove firmware (C++, on ESP32-C3)**: reads the joystick and the MPU-6050 IMU, computes left and right wheel values, sends them to the backend every 50 ms while input is active, and drives the vibration motor from the car's proximity telemetry.

- *Joystick (primary):* throttle and steering are combined, with a squared turn curve on the steering axis. A dead zone of 10, applied in code, ignores small unintended movement.
- *Wrist gesture (secondary):* holding the joystick button captures the current wrist angle as a reference. Rotating about 45° either way from that reference maps to pivot speed, from 0 to 100%.
- *Silent at zero:* when the joystick is at rest, the glove sends a single zero command and then stops sending until input resumes.
- *Haptics:* vibration intensity follows the proximity data returned from the car.
- *V1.5 additionally:* the glove checks the IMU at boot and warns if it is not found, and it stops using telemetry that is more than 500 ms old.

**Backend (Python/Flask, on Raspberry Pi)**: relays wheel commands from the glove to the car and returns car telemetry to the glove. The backend is a single Flask application shared across ROCH devices (see [`backend/`](../../../backend/)).

- *V1:* a background serial reader handles the glove's USB serial link alongside the car's.
- *V1.5:* the glove connects over Wi-Fi, so glove traffic arrives as HTTP requests carrying JSON payloads on `/glove/car` instead of serial lines.

**Car firmware (C++, on Arduino Uno)**: applies the left and right wheel values it receives and returns proximity telemetry. It also enforces the 500 ms command watchdog and the proximity stop, so the car halts safely if the glove stops sending. See the [car architecture](../../Car/docs/architecture.md).

Input mixing is done on the glove, so the backend and car firmware only forward and apply wheel values. This keeps one wheel command format whether the source is the glove or the browser.

## Data flow

**V1 (USB serial):**
Glove → USB Serial → Pi (backend) → Serial → Car Arduino → (wheels, proximity indicator LED)
Car Arduino → Serial (proximity telemetry) → Pi (backend) → USB Serial → Glove (haptics)

**V1.5 (Wi-Fi HTTP/JSON):**
Glove → Wi-Fi → HTTP/JSON → Flask (`/glove/car`) → Serial → Car Arduino → (wheels, proximity indicator LED)
Car Arduino → Serial (proximity telemetry) → Flask → Glove (haptics)

## Development Approach: Serial First, Then Wi-Fi

V1 uses USB serial for bench testing. A wired link makes it easy to log values and tune the joystick mix, the pivot gesture, and the haptics without network issues. Once the controls and gestures are satisfactory, V1.5 moves the link to Wi-Fi HTTP/JSON so the glove can be worn untethered. Only the transport changes; the control logic does not.

A first-generation scheme (Gen 1) mapped wrist pitch and roll directly to wheel speed. After testing those gestures, the scheme was not satisfying in use, so the joystick became the primary input and the wrist moved to the secondary pivot role. The Gen 1 firmware is kept in [`firmware/`](../firmware/).

## Wiring

The IMU is on the I2C bus, and the vibration motor is driven from a GPIO pin. The joystick wiring is being updated for a new joystick module with a separate push button, and its connections will be documented once finalized.

| Component | Connection |
|---|---|
| MPU-6050 IMU | I2C (SDA: 9, SCL: 8) |
| Joystick module | To be documented |
| Vibration motor | GPIO 10 |

## Media

A photo of the prototype build and demonstration videos will be added. The schematic will be added after the joystick wiring is finalized.
