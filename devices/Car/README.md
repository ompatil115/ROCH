# Embedded Robotic Vehicle Control System (Car)

A robotic vehicle built on a Raspberry Pi and Arduino, supporting browser and glove control, RGB lighting, and ultrasonic distance sensing over a local network. Built as a first full-stack embedded systems project, from protocol design through firmware, backend, and interface.

The car has two versions:

- **V1:** browser-controlled movement, lighting, and 180° distance scanning.
- **V1.5:** a firmware and protocol update that adds glove driving through the Raspberry Pi, with proximity telemetry returned to the glove.

## Overview

- **Firmware (C++, Arduino Uno):** motor, lighting, and ultrasonic sensor control (V1 also drove a scanning servo)
- **Backend (Python/Flask, Raspberry Pi):** serial-to-HTTP bridge, multithreaded command handling
- **Interface (HTML/CSS/JS):** browser-based control panel with a color picker, a directional movement pad, and a radar-style scan graph (V1)

See [`docs/architecture.md`](docs/architecture.md) for system design and [`docs/protocol.md`](docs/protocol.md) for the full command specification for both V1 and V1.5.

## Features

**V1**
- RGB lighting control through a gradient slider
- Directional drive and turn control with variable-speed forward movement
- 180° ultrasonic distance scanning with a radar-style scan graph
- Thread-safe backend handling concurrent serial I/O and command locking
- Motor safety watchdog that halts output if control input stops (originally implemented in the Python backend)

**V1.5**
- Independent left and right wheel control for differential drive
- Glove driving: joystick and wrist-gesture commands arrive through the Raspberry Pi
- Proximity telemetry (distance, blocked flag, indicator color) returned to the glove, with a proximity warning and automatic stop
- Hysteresis in the car's command processing to remove command flicker near thresholds
- Safety watchdog moved from the Python backend into the Arduino firmware, with the same 500 ms timeout behavior

## Protocol

Full command specification for both V1 and V1.5 is in [`docs/protocol.md`](docs/protocol.md).

## Hardware

- Raspberry Pi 5 (headless, Linux)
- Arduino Uno R3
- DRV8833 dual motor driver
- 2x 6V DC motors
- HC-SR04 ultrasonic sensor
- RGB LED
- SG90 micro servo (V1 only, used for scanning)

## Setup

**Firmware**
1. Open the firmware sketch for your version in `firmware/` (`Car_V1.ino` or `Car_V1.5.ino`) in the Arduino IDE
2. Install the required libraries (V1 also needs `Servo`)
3. Upload to an Arduino Uno wired per the pin map in `docs/architecture.md`

**Backend**
```bash
cd backend
pip install -r requirements.txt
python3 app.py
```
Connect the Arduino to the Pi via USB, then visit `http://<pi-ip>:5000` from any device on the same network.

## Version History

| Version | Description | Git tag |
|---|---|---|
| V1 | Browser control, lighting, 180° scanning | `car-v1.0`, `hud-v1.0` |
| V1.5 | Glove driving, wheel-based protocol, proximity telemetry | current |

To view the V1 state of the repository, run `git checkout car-v1.0`.

## Status

**V1** is complete: all three subsystems (light, move, scan) work end to end, browser to hardware and back. V1 ran against a hardware constraint: attaching the servo library consumes a timer the Uno needs for PWM on two pins, leaving too few PWM-capable pins for full RGB and full variable-speed control in both directions. See `docs/architecture.md` for the tradeoff and what V1 scoped out as a result.

**V1.5** is complete. The scanning servo is no longer used, so that timer constraint no longer applies, and the car now takes wheel commands from the glove and browser.
