# ROCH Glove

A wearable controller built around a Seeed XIAO ESP32-C3 that drives the ROCH car from wrist orientation. It reads an MPU-6050 IMU and two buttons, sends drive commands through the shared backend, and gives haptic feedback from the car's proximity data.

**Status:** Phase 2 build. The firmware and hardware are done, and the glove has been tested with the backend, HUD, and car. See [Status](#status).

**Current version:** Glove V1 (Phase 2). It is one sketch that sends over Wi-Fi (HTTP) as the primary link and falls back to USB serial when Wi-Fi is down.

This folder holds the glove's firmware, documentation, and media. The backend is shared across ROCH devices and lives in [`backend/`](../../backend/).

## Overview

- **Firmware (C++, ESP32-C3):** reads the IMU and buttons, mixes pitch and roll into left and right wheel values, sends commands while the roll/safety button is held, and drives the vibration motor from the car's proximity telemetry
- **Backend (Python/Flask, Raspberry Pi):** receives glove commands, relays wheel commands to the car, and returns car telemetry to the glove

See [`docs/architecture.md`](docs/architecture.md) for system design and [`docs/protocol.md`](docs/protocol.md) for the message specification.

## Controls

- **Pitch sets speed:** tilting the wrist down drives forward and tilting up drives backward. There is a dead zone of ±15° and full speed at 45°.
- **Roll steers:** rolling the wrist right turns right and rolling left turns left. Steering slows the inside wheel, from the dead zone at 15° down to 0 at 70°.
- **Roll/safety button (D9):** must be held to drive. Pressing it sets the current roll as neutral. On release the glove holds its last command for 1000 ms, sends one stop, then stays silent.
- **Pitch button (D10):** pressing it sets the current pitch as neutral. While it is held, the glove sends zero wheel values, so it works as an emergency stop. It overrides the roll/safety button and cancels the 1000 ms hold.
- **Neutral setting:** each button averages 10 IMU readings over about 50 ms, so the neutral position is not set from a single noisy reading.
- **Haptics:** the vibration motor grows stronger as the car approaches an obstacle (starting at 80 cm) and is fully on when the car is blocked. It stays off while the roll/safety button is not armed.

In-place pivoting is in the firmware but turned off in Glove V1 (`ENABLE_PIVOT`).

## Features

- IMU orientation control with per-axis neutral zeroing
- Press-to-drive safety button and a separate emergency-stop button
- Wi-Fi HTTP link to the backend with a USB serial fallback, using the same message format on both
- Wi-Fi reconnect attempt every 5 s, with Wi-Fi power saving turned off for a steadier link
- Silent at zero: after the stop, the glove sends only a keep-alive `PING` until input resumes
- Wi-Fi credentials kept in a git-ignored `secrets.h`

## Hardware

- Seeed XIAO ESP32-C3 (with its external antenna attached; without it the Wi-Fi link was unreliable in testing)
- MPU-6050 IMU
- 2x push buttons (6 mm tactile, 4-pin), mounted on a small board worn on a finger
- 3V DC vibration motor
- LiPo battery with a toggle power switch

### Wiring

| Component | XIAO pin | GPIO |
|---|---|---|
| MPU-6050 SDA | D5 | 7 |
| MPU-6050 SCL | D6 | 21 |
| Vibration motor | D8 | 8 |
| Roll/safety button | D9 | 9 |
| Pitch button | D10 | 10 |

Each button connects between its pin and GND, using the ESP32's internal pull-up (`INPUT_PULLUP`), so a press reads LOW. The two wired legs are on opposite sides of the 4-pin switch. The battery connects through the toggle switch. The schematic is in [`docs/media/schematic.pdf`](docs/media/schematic.pdf) ([KiCad source](docs/media/Glove_V1.kicad_sch)), and a photo of the build is in [`docs/media/glove-image.pdf`](docs/media/glove-image.pdf).

## Setup

**Firmware**
1. Open the Glove V1 sketch in [`firmware/Glove_V1/`](firmware/Glove_V1/) in the Arduino IDE
2. Install the ESP32 board package (no extra libraries are needed)
3. Copy `secrets.h.example` to `secrets.h` in the same folder (`firmware/Glove_V1/`) and fill in `WIFI_SSID` and `WIFI_PASS`. `secrets.h` is git-ignored and must not be committed
4. Set `PI_HOST` in the sketch to the Raspberry Pi's IP address. Reserve a fixed IP for the Pi on your router so it does not change
5. Upload to the ESP32-C3

**Backend**

Start the shared backend as described in [`backend/README.md`](../../backend/README.md). For the Wi-Fi link, the glove and the Raspberry Pi must be on the same network. For the serial fallback, connect the glove to the Pi over USB.

## Version History

| Version | Description | Git tag |
|---|---|---|
| V1 | IMU pitch and roll control with press-to-drive safety, HTTP primary and serial fallback, proximity haptics | not yet tagged |

## Status

Glove V1 firmware, the Wi-Fi link, the serial fallback, the handover between the two, the emergency-stop button, the IMU orientation, and button driving have all been tested. Results are in [`docs/phase2/`](../../docs/phase2/) at the repository root.

Demo videos: [Glove V1 Drive Demo](https://youtu.be/q82pCMLTnlQ) and [HUD V2 Glove Connection Demo](https://youtu.be/KCKdDAnIDC4) (Wi-Fi drop, serial fallback and reconnect). More are in the [root README](../../README.md#demonstrations).
