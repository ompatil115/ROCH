# Robotics Operations and Control Hub (ROCH)

ROCH (Robotics Operations and Control Hub) is a personal engineering project that explores how robotic devices can be monitored and controlled through a unified browser-based interface and a wearable control glove.

The project is developed in phases. Each phase delivers a milestone across three systems: the **car**, the **HUD** (browser interface), and the **glove**.

## Project Status

| Phase | Systems | Status |
|---|---|---|
| Phase 1 | Car V1, HUD V1 | Complete |
| Phase 2 | Car V1.5, Glove, Backend, HUD V1.5 | In progress |
| Phase 3+ | To be determined | Planned |

**Phase 2 progress**

| System | Status |
|---|---|
| Car V1.5 | Firmware complete |
| Glove | Firmware being polished; V1 hardware design being finalized; calibration and testing in progress |
| Backend | Being refined |
| HUD V1.5 | Code in progress; system integration and demonstrations still to do |

## Repository Layout

```text
ROCH/
├── backend/        shared Flask app (HTTP API and browser HUD)
└── devices/
    ├── Car/        firmware, documentation, media
    └── Glove/      firmware, documentation
```

Each device folder has its own documentation. See [`devices/Car/`](devices/Car/) and [`devices/Glove/`](devices/Glove/).

## Backend

A single Flask application in [`backend/`](backend/) serves the car and the glove and hosts the browser HUD. Each device has its own routes, so a command name only needs to be unique within its device. The backend is under active refinement. Setup, routes, and details are in [`backend/README.md`](backend/README.md).

### Versions and Tags

The backend is one `app.py` that is updated over time. Git tags mark each version, and `git checkout <tag>` shows the whole repository as it was at that point.

| Phase | Tags |
|---|---|
| Phase 1 | `car-v1.0`, `hud-v1.0` |
| Phase 2 | Tags will be added as each version is finalized |

## Phase 1: Car V1 and HUD V1

Phase 1 demonstrates end-to-end remote control and two-way communication between a browser interface and physical robotic hardware.

Car V1 functions:

1. Controllable multicolor lighting.
2. Forward, reverse, left, and right movement.
3. Distance scanning across a 180-degree field of view.

HUD V1 is a browser interface on the local network that sends commands to the car and shows the results, including a distance display with a radar-style scan graph.

### System Architecture

- **Browser interface:** lighting, movement, and scan controls, plus the scan graph.
- **Raspberry Pi backend:** the shared Python/Flask service (see [Backend](#backend)) that handles HTTP requests, manages state, and communicates over USB serial.
- **Arduino firmware:** low-level control of the motors, servo, ultrasonic sensor, and lighting.

```text
Browser Interface
       |
     HTTP
       |
Raspberry Pi 5
Python / Flask
       |
   USB Serial
       |
 Arduino Uno
       |
Motors, Servo, Ultrasonic Sensor, Lighting
```

See [`devices/Car/`](devices/Car/) for build documentation, firmware, and media.

## Phase 2: Glove, Car V1.5, HUD V1.5 (in progress)

Phase 2 adds a wearable control glove and updates the car and HUD to support glove driving. This phase is still in development.

### Glove

The glove is built around an ESP32-C3 with an I2C IMU, a joystick, and a vibration motor for haptic feedback. It sends drive commands to the car through the Raspberry Pi and receives car proximity telemetry. See [`devices/Glove/`](devices/Glove/).

**Current control design:**
- **Joystick (primary):** throttle and steering using a squared turn curve.
- **Wrist gesture (secondary):** holding the joystick button captures the current wrist angle as a reference; rotating about 45 degrees either way maps to pivot speed.
- **Haptic feedback:** the vibration motor responds to the car's proximity data.

An earlier generation mapped wrist pitch and roll directly to wheel speed. Its firmware is kept in [`devices/Glove/`](devices/Glove/).

### Car V1.5

Car V1.5 is a firmware update from the V1 move/light/scan protocol to a wheel-based protocol that supports glove driving, with forward distance sensing, a proximity warning and stop, and a command timeout. The car firmware is complete.

### HUD V1.5

HUD V1.5 is in development. It will update the browser interface to work with the wheel-based protocol and glove input.

### Protocol (glove, Raspberry Pi, car)

```text
WHEELS:<left>,<right>                 each -100..100      toward car
PROX:<dist>,<blocked>,<red>,<green>                       car telemetry
```

The car stops its motors if no command arrives within 500 ms. Full specifications are in [`devices/Car/docs/protocol.md`](devices/Car/docs/protocol.md) and [`devices/Glove/docs/protocol.md`](devices/Glove/docs/protocol.md).

## Roadmap

1. Finish Phase 2: glove firmware and calibration, backend refinement, and HUD code.
2. Integrate the glove, backend, HUD, and car as one system and record demonstrations.
3. Define the next phase.