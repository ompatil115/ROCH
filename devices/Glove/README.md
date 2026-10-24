# ROCH Glove

A wearable controller built around an ESP32-C3 that drives the ROCH car from hand input. It reads a joystick and wrist orientation, sends drive commands through the shared backend, and gives haptic feedback from the car's proximity data.

**Status: in development.** The firmware is still being polished, and testing and test documentation are still to come. See [Status](#status).

The glove has three firmware versions:

- **Gen 1:** the original tilt-control version, where wrist pitch and roll set wheel speed. Kept in [`firmware/`](firmware/) for reference.
- **V1:** joystick-primary control over USB serial, used for bench testing.
- **V1.5:** the same controls over Wi-Fi, using HTTP and JSON.

This folder holds the glove's firmware, documentation, and media. The backend is shared across ROCH devices and lives in [`backend/`](../../backend/).

## Development Approach

The glove is built serial-first. V1 connects over USB serial, which makes it easy to log values and iterate on the control scheme at the bench. Once the joystick and gesture behavior is satisfactory, V1.5 moves the link to Wi-Fi HTTP/JSON so the glove can be worn untethered. The control logic is the same in both versions; only the transport changes.

Gen 1 mapped wrist pitch and roll directly to wheel speed. After testing those gestures, the scheme was not satisfying in use, so the joystick became the primary control and the wrist gesture moved to a secondary pivot role. Formal re-testing of the current design, with video and a spreadsheet, is still to come.

## Overview

- **Firmware (C++, ESP32-C3):** reads the joystick and IMU, mixes input into left and right wheel values, sends commands at up to 20 per second (50 ms interval) while input is active, and drives the vibration motor from returned telemetry
- **Backend (Python/Flask, Raspberry Pi):** receives glove commands, relays wheel commands to the car, and returns car telemetry to the glove

See [`docs/architecture.md`](docs/architecture.md) for system design and [`docs/protocol.md`](docs/protocol.md) for the message specification for V1 and V1.5.

## Features

**V1 (serial)**
- Joystick drive: throttle and steering with a squared turn curve and a dead zone
- Wrist-gesture pivot: holding the joystick button captures the current wrist angle as a reference; rotating about 45° either way maps to pivot speed
- Haptic feedback: the vibration motor responds to the car's proximity data
- Commands sent over USB serial at up to 20 per second while input is active
- Silent at zero: on release the glove sends one zero command, then stays silent until input resumes

**V1.5 (Wi-Fi)**
- Same control scheme as V1, untethered
- Commands and telemetry exchanged with the backend over Wi-Fi as HTTP/JSON; the rate is limited by network round-trip time
- Boot-time IMU check that warns if the sensor is not found
- Stale telemetry is no longer used for haptics after 500 ms
- Wi-Fi credentials and backend address kept in a git-ignored `secrets.h`

## Hardware

- Seeed XIAO ESP32-C3
- MPU-6050 IMU
- Joystick module with push button
- 3V DC vibration motor

## Setup

**Firmware**
1. Open the sketch for your version in [`firmware/`](firmware/) (`Glove_V1/Glove_V1.ino` for USB serial, or `Glove_V1.5/Glove_V1.5.ino` for Wi-Fi) in the Arduino IDE
2. Install the ESP32 board package and the library for the IMU
3. V1.5 only: copy `secrets.h.example` to `secrets.h` in the same folder and fill in `WIFI_SSID`, `WIFI_PASSWORD`, and `BACKEND_URL`. `secrets.h` is git-ignored and must not be committed
4. Upload to the ESP32-C3 (V1: also connect it to the Raspberry Pi over USB)

**Backend**

Start the shared backend as described in [`backend/README.md`](../../backend/README.md). For V1.5, the glove and the Raspberry Pi must be on the same network.

## Version History

| Version | Description | Git tag |
|---|---|---|
| Gen 1 | Tilt control (pitch and roll set wheel speed) | not tagged |
| V1 | Joystick-primary driving, wrist-gesture pivot, proximity haptics over USB serial | not yet tagged |
| V1.5 | Same controls over Wi-Fi HTTP/JSON | not yet tagged |

## Status

The glove is in development. The firmware for V1 and V1.5 is written but not finalized, and the V1 hardware design is still being finished. Still to do:

- Finish polishing the firmware
- Calibrate and test the controls
- Document the tests with video and a spreadsheet
- Integrate with the backend and HUD as one system
