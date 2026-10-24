# ROCH Backend

A single Flask application (`app.py`) that runs on the Raspberry Pi. It connects the browser HUD, the glove, and the car, and serves the HUD page.

**Status:** under active refinement. Routes and setup details may change.

## Overview

- Serves the browser HUD at `/`.
- Sends wheel commands to the car's Arduino over USB serial.
- Reads car proximity telemetry and exposes it to the HUD.
- Reads glove commands over USB serial and forwards them to the car.
- Decides whether the glove or the browser has control of the car at any moment.

Each device has its own routes, so a command name only needs to be unique within its device.

## Setup

Requirements: Python 3 on the Raspberry Pi, with the car (and glove, if used) connected over USB.

```bash
cd backend
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
python app.py
```

Then open `http://<pi-ip>:5000` from any device on the same network. Serial port paths are set in `app.py`.

## Routes

| Route | Method | Purpose |
|---|---|---|
| `/` | GET | Browser HUD |
| `/car/state` | GET | Latest car telemetry and which input currently has control |
| `/car/wheels` | POST | Send left and right wheel values (each -100 to 100) |

`/car/wheels` returns an error if the values are invalid (400), if the glove currently has control (409), or if the car is not connected (503).

## Control Arbitration

The glove has control while it is sending non-zero commands, and for 500 ms after the last one. Outside that window the browser can drive. `/car/state` reports the current driver as `glove`, `browser`, or `none`. A single lock covers both the control decision and the serial write to the car.

## Serial Links

| Device | Baud rate | Notes |
|---|---|---|
| Car (Arduino Uno) | 9600 | Receives `WHEELS` commands, sends `PROX` telemetry |
| Glove | 115200 | Port opens when the glove connects and retries if it drops |

Command and telemetry formats are in [`devices/Car/docs/protocol.md`](../devices/Car/docs/protocol.md) and [`devices/Glove/docs/protocol.md`](../devices/Glove/docs/protocol.md).

## Versions and Tags

The backend is one `app.py` that is updated over time. Git tags mark each version, and `git checkout <tag>` shows the whole repository as it was at that point.

| Phase | Tags |
|---|---|
| Phase 1 | `car-v1.0`, `hud-v1.0` |
| Phase 2 | Tags will be added as each version is finalized |
