# Control Glove (ESP32-C3)

A wearable controller built around an ESP32-C3 that drives ROCH devices from hand input. It reads a joystick and wrist orientation, sends drive commands through the shared backend, and gives haptic feedback from the car's proximity data.

The glove has three versions:

- **V1:** drives the car over USB serial. Used for bench testing the joystick, wrist gestures, and haptics.
- **V1.5:** the same controls over Wi-Fi, using HTTP and JSON.
- **V2 (in progress):** extends the glove to control the robotic arm.

This folder holds the glove's firmware, documentation, and media. The backend is shared across all ROCH devices and lives in [`backend/`](../../backend/).

## Development Approach

The glove was built serial-first. V1 connects over USB serial, which made it easy to log values and iterate on the control scheme at the bench. Once the joystick and gesture behavior was satisfactory, the link moved to Wi-Fi HTTP/JSON in V1.5 so the glove can be worn untethered. The control logic is the same in both versions; only the transport changed.

## Overview

- **Firmware (C++, ESP32-C3):** reads the joystick and IMU, mixes input into left and right wheel values, sends commands every 50 ms, and drives the vibration motor from returned telemetry
- **Backend (Python/Flask, Raspberry Pi):** receives glove commands, relays wheel commands to the car, and returns car telemetry to the glove

See [`docs/architecture.md`](docs/architecture.md) for system design and [`docs/protocol.md`](docs/protocol.md) for the message specification for both V1 and V1.5.

## Features

**V1 (serial)**
- Joystick drive: throttle and steering using a throttle-gated mix and a squared turn curve
- Wrist-gesture pivot: holding the joystick button captures the current wrist angle as zero (press-to-zero); rotating about 45° either way maps to pivot speed
- Haptic feedback: the vibration motor responds to the car's proximity data
- Commands sent and telemetry received every 50 ms over USB serial

**V1.5 (Wi-Fi)**
- Same control scheme as V1, untethered
- Commands and telemetry exchanged with the backend over Wi-Fi as HTTP/JSON, every 50 ms
- Wi-Fi credentials and backend address kept in a git-ignored `secrets.h`

A first-generation scheme mapped wrist pitch and roll directly to wheel speed. Logged trials led to the joystick-primary design above. The earlier code and data are in [`archive/`](archive/).

## Hardware

- Seeed XIAO ESP32-C3
- MPU-6050 IMU
- Arduino Modulino Joystick (ABX00135)
- 3V DC vibration motor

## Setup

**Firmware (V1, serial)**
1. Open [`firmware/Glove_V1/Glove_V1.ino`](firmware/Glove_V1/Glove_V1.ino) in the Arduino IDE
2. Install the ESP32 board package and the libraries for the IMU and joystick module
3. Upload to the ESP32-C3 and connect it to the Raspberry Pi over USB

**Firmware (V1.5, Wi-Fi)**
1. Open [`firmware/Glove_V1.5/Glove_V1.5.ino`](firmware/Glove_V1.5/Glove_V1.5.ino) in the Arduino IDE
2. Install the same board package and libraries
3. Copy `secrets.h.example` to `secrets.h` in the same folder and fill in your Wi-Fi credentials and the backend address. `secrets.h` is git-ignored and must not be committed
4. Upload to the ESP32-C3

**Backend**

Start the shared backend as described in the [root README](../../README.md#backend). For V1.5, the glove and the Raspberry Pi must be on the same network.

## Version History

| Version | Description | Git tag |
|---|---|---|
| V1 | Joystick-primary car driving, wrist-gesture pivot, proximity haptics over USB serial | not yet tagged |
| V1.5 | Same controls over Wi-Fi HTTP/JSON | not yet tagged |
| V2 | Arm control | in progress |

## Status

**V1** and **V1.5** are complete. Both drive the car end-to-end through the backend.

**V2** is in progress. Planned, not yet implemented:
- Arm control through wrist and forearm gestures, with the joystick for precision adjustments
- Mode indicator LED showing car or arm mode
- Press-and-hold mode switching between car and arm
- Flex-sensor gripper input
