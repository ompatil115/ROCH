# Robotics Operations and Control Hub (ROCH)

ROCH (Robotics Operations and Control Hub) is a personal engineering project that explores how robotic devices can be monitored and controlled through a unified browser-based interface and a wearable control glove.

The project is developed in phases. Each phase delivers a milestone across three systems: the **car**, the **HUD** (browser interface), and the **glove**, supported by a shared **backend**.

## Project Status

| Phase | Systems | Status |
|---|---|---|
| Phase 1 | Car V1, HUD V1, Backend V1 | Complete |
| Phase 2 | Glove V1, Car V1.5, Backend V2, HUD V2 | Complete |
| Phase 3+ | To be determined | Planned |

**Phase 2 status**

| System | Status |
|---|---|
| Glove V1 | Firmware, hardware, and tests done |
| Car V1.5 | Firmware and tests done |
| Backend V2 | Done; HTTP and serial glove links tested |
| HUD V2 | Done; keyboard and glove driving tested |

Phase 2 is complete. Release tags are listed under [Versions and Tags](#versions-and-tags).

## Repository Layout

```text
ROCH/
├── backend/        shared Flask app, HUD pages, requirements
├── docs/
│   └── phase2/     Phase 2 test data (.xlsx) and change log (.pdf)
└── devices/
    ├── Car/        README, docs (with media), firmware
    └── Glove/      README, docs (with media), firmware
```

Each device folder has its own documentation. See [`devices/Car/`](devices/Car/) and [`devices/Glove/`](devices/Glove/).

## Backend

A single Flask application in [`backend/`](backend/) serves the car and the glove and hosts the browser HUD. Backend V2 receives glove commands over HTTP (primary) or USB serial (fallback), decides whether the glove or the browser has control, and forwards wheel values to the car. Setup, routes, and details are in [`backend/README.md`](backend/README.md).

### Versions and Tags

Each backend and HUD version has its own file (`app.py` and `templates/index.html` for Phase 1, `app_V2.py` and `templates/index_V2.html` for Phase 2). Git tags mark each version, and `git checkout <tag>` shows the whole repository as it was at that point. A full version step marks a significant change, and a half step marks a minor one. Phase 1 and Phase 2 are separate builds, so their versions are numbered independently.

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

## Phase 2: Glove V1, Car V1.5, Backend V2, HUD V2 (complete)

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
PROX:<dist>,<blocked>|<red>,<green>                                  car telemetry to backend
```

`dist` is in cm, and the car reports 401 when nothing is detected within 400 cm (the HUD shows "400+"). The backend uses only the part before the `|` and ignores the LED colors.

Full specifications are in [`devices/Glove/docs/protocol.md`](devices/Glove/docs/protocol.md) and [`devices/Car/docs/protocol.md`](devices/Car/docs/protocol.md).

### Demonstrations

Three videos cover the full Phase 2 system. The links will be added here.

| Video | Shows |
|---|---|
| Glove driving | The glove driving the car on a stand with the HUD on a monitor behind it: pitch and roll control, the roll/safety button, the pitch-button emergency stop, the 1 s hold, and the proximity stop |
| Keyboard driving | W, A, S and D with the 1 s ramp, faster braking on the opposite key, the X dead stop, and control switching between the keyboard and the glove |
| Link handoff | The HUD status strip with both glove links, the switch between HTTP and serial in both directions, and the vibration motor responding to the car's proximity |

### Testing

Phase 2 test results are in [`docs/phase2/ROCH_Phase2_Data.xlsx`](docs/phase2/ROCH_Phase2_Data.xlsx), and the full list of changes for each round is in [`docs/phase2/ROCH_Phase2_Change_Log.pdf`](docs/phase2/ROCH_Phase2_Change_Log.pdf).

| Test | Result |
|---|---|
| HTTP command timing (Flask development server vs Waitress) | Send rate rose from about 3 to about 9.3 commands per second; median request time fell from 347 ms to about 18 ms |
| Glove HTTP command timing | WHEELS lines arrive about every 83 ms while driving; idle pings about every 2 s |
| Glove Wi-Fi link quality | With the antenna: 0% ping loss, 10.6 ms average. Without it: 8.7% loss |
| Pitch and roll calibration | Pitch of ±45° maps to ±100 speed; roll of 0 to 70° maps to 0 to 100 turn |
| Glove-to-HUD handover (serial and HTTP) | Control flips cleanly, and the HUD gets a conflict while the glove is active |
| Reconnect (serial, and serial with HTTP) | Glove, car and both links drop and return without restarting the app; the HTTP link takes about 5 s |
| Serial fallback and HTTP priority | Falls back to serial when Wi-Fi is down; HTTP wins when both are live |
| Pitch button emergency stop | Wheels go to zero while held, overriding the safety button and the 1 s hold |
| Safety button hold | The last command repeats for 1 s after release, then one stop is sent |
| Obstacle stop, hysteresis, watchdog | Passed. At max speed the car stops at 23 cm from the obstacle (the 25 cm stop point is a buffer, since the car has momentum). It blocks at 25 cm or less, clears above 30 cm, and stops within about 0.5 s of commands ending |
| Keyboard ramps and X dead stop | 1 s ramp, 2x braking, and an immediate stop |
| IMU orientation and button driving | Pitch down drives forward and wrist right turns right; driving with the safety button held |
| Component test sketches (IMU, vibration motor, buttons) | Passed on the glove |

Command latency (end to end) and network throttling tests are planned for Phase 3.

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

## Development History

Earlier development versions of the code, organized by phase and round, are archived in a [Google Drive folder](https://drive.google.com/drive/folders/1LhveoIxMBXKmOTSKXsggR_EjLncx9t-4).

## Roadmap

1. Phase 3: end-to-end command latency and network throttling tests, plus defining the next devices.
