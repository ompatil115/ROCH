# Robotics Operations and Control Hub (ROCH)

ROCH (Robotics Operations and Control Hub) is a personal engineering project that explores how robotic devices can be monitored and controlled through a unified browser-based interface and a wearable control glove.

The project is developed in phases. Each phase delivers a milestone across three systems: the **car**, the **HUD** (browser interface), and the **glove**, supported by a shared **backend**.

## Project Status

| Phase | Systems | Status |
|---|---|---|
| Phase 1 | Car V1, HUD V1, Backend V1 | Complete |
| Phase 2 | Glove V1, Car V1.5, Backend V2, HUD V2 | In progress |
| Phase 3+ | To be determined | Planned |

**Phase 2 progress**

| System | Status |
|---|---|
| Glove V1 | Firmware and bench tests done; button-driving and IMU orientation testing in progress; hardware design being finalized |
| Car V1.5 | Firmware complete |
| Backend V2 | Working; HTTP and serial glove links tested |
| HUD V2 | Working; system integration and demonstrations still to do |

## Repository Layout

```text
ROCH/
├── backend/        shared Flask app, HUD, requirements
└── devices/
    ├── Car/        README, docs (with media), firmware
    └── Glove/      README, docs, firmware
```

Each device folder has its own documentation. See [`devices/Car/`](devices/Car/) and [`devices/Glove/`](devices/Glove/).

## Backend

A single Flask application in [`backend/`](backend/) serves the car and the glove and hosts the browser HUD. Backend V2 receives glove commands over HTTP (primary) or USB serial (fallback), decides whether the glove or the browser has control, and forwards wheel values to the car. Setup, routes, and details are in [`backend/README.md`](backend/README.md).

### Versions and Tags

The backend is one `app.py` that is updated over time. Git tags mark each version, and `git checkout <tag>` shows the whole repository as it was at that point. A full version step marks a significant change, and a half step marks a minor one. Phase 1 and Phase 2 are separate builds, so their versions are numbered independently.

| Phase | Versions | Tags |
|---|---|---|
| Phase 1 | Car V1, HUD V1, Backend V1 | `car-v1.0`, `hud-v1.0`, `backend-v1.0` (all on the same commit) |
| Phase 2 | Glove V1, Car V1.5, Backend V2, HUD V2 | Tags will be added as each version is finalized |

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

## Phase 2: Glove V1, Car V1.5, Backend V2, HUD V2 (in progress)

Phase 2 is a separate build from Phase 1. It adds a wearable control glove and updates the car, backend, and HUD to support glove driving.

### Glove V1

The glove is built around a Seeed XIAO ESP32-C3 with an MPU-6050 IMU, two buttons, and a vibration motor for haptic feedback. It is one sketch that sends drive commands to the car through the Raspberry Pi over Wi-Fi (HTTP) and falls back to USB serial when Wi-Fi is down. It receives the car's proximity telemetry in return. See [`devices/Glove/`](devices/Glove/).

**Control design:**
- **Pitch sets speed:** tilting the wrist down drives forward and tilting up drives backward, with a dead zone of ±15° and full speed at 45°.
- **Roll steers:** rolling the wrist right turns right and rolling left turns left. Steering slows the inside wheel, from the dead zone at 15° down to 0 at 70°.
- **Roll/safety button:** must be held to drive. Pressing it sets the current roll as neutral.
- **Pitch button:** pressing it sets the current pitch as neutral. While it is held, the glove sends zero wheel values, so it also works as an emergency stop.
- **Neutral setting:** each button averages 10 IMU readings over about 50 ms, so the neutral position is not set from a single noisy reading.
- **Haptic feedback:** the vibration motor grows stronger as the car gets closer to an obstacle and is fully on when the car is blocked.

### Car V1.5

Car V1.5 is a firmware update from the V1 move/light/scan protocol to a wheel-based protocol that supports glove driving, with forward distance sensing, a proximity warning and stop, and a command timeout. The car firmware is complete.

### Backend V2

Backend V2 adds the `/glove` route for HTTP glove input, reports both glove links in `/status`, and gives HTTP priority over serial while HTTP is driving. Details are in [`backend/README.md`](backend/README.md).

### HUD V2

HUD V2 updates the browser interface for the wheel protocol. It shows live proximity and wheel values, and shows each connected glove link (HTTP and/or serial), red while the glove is driving.

### Protocol (glove, Raspberry Pi, car)

```text
WHEELS:<left>,<right>|<pitch>,<roll>|<neutralPitch>,<neutralRoll>   glove to backend (HTTP body or serial line)
PING                                                                 glove to backend (HTTP only, keeps the link alive)
PROX:<dist>,<blocked>,                                               backend to glove
WHEELS:<left>,<right>                                                backend to car, each -100..100
PROX:<dist>,<blocked>,<red>,<green>                                  car telemetry to backend
```

Full specifications are in [`devices/Glove/docs/protocol.md`](devices/Glove/docs/protocol.md) and [`devices/Car/docs/protocol.md`](devices/Car/docs/protocol.md).

### Safety

These are the designed safety behaviors of the Phase 2 system. They are described as designed, not as a verified safety guarantee.

| Layer | Behavior |
|---|---|
| Glove | The roll/safety button must be held to drive; on release the glove holds for 1000 ms, sends one stop, then goes silent |
| Glove | The pitch button forces zero wheel values while held, overriding the roll/safety button |
| Glove | HTTP requests time out after 300 ms, and the glove falls back to serial when Wi-Fi is down |
| Backend | Wheel values are clamped, and the glove takes priority over the browser (`/wheels` returns 409 while the glove drives) |
| Backend | Glove control expires 1 s after the last command; the HTTP link is marked disconnected after 5 s of silence |
| Car | Stops both motors if no command arrives within 500 ms |
| Car | Stops forward motion when an obstacle is 25 cm or closer, and clears at 30 cm |

## Roadmap

1. Finish Phase 2: remaining glove tests (IMU orientation and button driving) and hardware.
2. Integrate the glove, backend, HUD, and car as one system and record demonstrations.
3. Define the next phase.
