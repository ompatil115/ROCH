# ROCH Backend

A single Flask application that runs on the Raspberry Pi. It connects the browser HUD, the glove, and the car, and serves the HUD page.

Each backend version has its own files, so every version stays runnable from this folder:

| Version | Backend | HUD page |
|---|---|---|
| Backend V1 / HUD V1 (Phase 1) | `app.py` | `templates/index.html` |
| Backend V2 / HUD V2 (Phase 2) | `app_V2.py` | `templates/index_V2.html` |

Each backend loads the HUD page that matches its version. The sections below describe Backend V2.

**Current version:** Backend V2 (Phase 2). Backend V1 (Phase 1) is tagged `backend-v1.0`.

## Overview

- Serves the browser HUD at `/`.
- Sends wheel commands to the car's Arduino over USB serial.
- Reads car proximity telemetry and exposes it to the HUD.
- Receives glove commands over HTTP (primary) or USB serial (fallback) and forwards them to the car.
- Decides whether the glove or the browser has control of the car at any moment.
- Runs on waitress (8 threads) at `0.0.0.0:5000`.

## Setup

Requirements: Python 3 on the Raspberry Pi, with the car connected over USB. The glove connects over WiFi, or over USB if used as the fallback.

```bash
cd backend
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
python app_V2.py
```

Then open `http://<pi-ip>:5000` from any device on the same network. The car port and baud rates are constants at the top of `app_V2.py`. Reserve a fixed IP for the Pi on your router so the glove can find it.

## Routes

| Route | Method | Purpose |
|---|---|---|
| `/` | GET | Browser HUD |
| `/state` | GET | Latest proximity, wheel values, and whether the glove is driving (polled by the HUD every 100 ms) |
| `/status` | GET | Connection status for the car and the glove (polled by the HUD every 1 s) |
| `/wheels` | POST | Send left and right wheel values from the browser (each -100 to 100) |
| `/glove` | POST | Glove input over HTTP |

`/wheels` returns an error if the values are invalid (400), if the glove currently has control (409), or if the car is not connected (503).

### `/glove`

The request body is plain text: a `WHEELS` line or `PING`. The reply is plain text in the form `PROX:dist,blocked,`. A bad body returns 400. `PING` keeps the link alive without counting as driving.

### `/status`

```json
{
  "car":   { "connected": true, "port": "/dev/ttyACM0", "type": "serial", "baud": 9600, "age": 0.1 },
  "glove": {
    "inUse": false,
    "age": null,
    "serial": { "connected": false, "port": null, "type": "serial", "baud": 115200 },
    "http":   { "connected": true, "ip": "192.168.1.68", "type": "http", "age": 0.4 }
  }
}
```

`age` is seconds since the last message. The HUD shows each connected glove link, red when the glove is driving and gray when no link is connected.

## HUD V2

The browser HUD (`templates/index_V2.html`) polls `/state` every 100 ms and `/status` every 1 s. It shows the distance and proximity bars, the left and right wheel values, and the glove's pitch, roll, and neutral angles. A status strip shows the car and each glove link.

The HUD drives from a pad or the keyboard:
- **W, A, S, D or the arrow keys:** speed and turn ramp up over 1 s, and pressing the opposite key brakes twice as fast.
- **X:** dead stop. Holding it keeps the car stopped and ignores the drive keys.
- **Blocked:** forward motion is ignored while the car reports blocked.
- **Glove takeover:** the HUD pad turns red and its controls are disabled while the glove is driving, and the HUD takes over again when the glove stops sending.

Demo video: [HUD V2 Keyboard Drive Demo](https://youtu.be/Pk3QCdzDuiY)

## Control Arbitration

The glove has control while it is sending `WHEELS` lines, and for 1 s after the last one (`GLOVE_TIMEOUT`). Outside that window the browser can drive. `/state` reports `gesture` as 1 while the glove has control.

When the glove is connected over both links, HTTP wins while it has driven within `GLOVE_TIMEOUT`, and serial `WHEELS` lines are ignored. If HTTP goes quiet, serial takes over without a restart. The HTTP link counts as connected for 5 s after the last message (`HTTP_LINK_TIMEOUT`). A lock covers both the control decision and the serial write to the car.

Incoming values are clamped: wheels to ±100, pitch to ±45, roll to ±70.

## Serial Links

| Device | Baud rate | Notes |
|---|---|---|
| Car (Arduino Uno) | 9600 | Receives `WHEELS` commands, sends `PROX` telemetry |
| Glove (fallback only) | 115200 | Port is found automatically among the `/dev/ttyACM*` devices, opens when the glove connects, and retries if it drops |

Command and telemetry formats are in [`devices/Car/docs/protocol.md`](../devices/Car/docs/protocol.md) and [`devices/Glove/docs/protocol.md`](../devices/Glove/docs/protocol.md).

## Safety Behavior

Designed behavior on the backend side. Car and glove limits are in their own READMEs.

- Wheel values are clamped before they reach the car.
- The glove takes priority over the browser, and `/wheels` is refused (409) while it is driving.
- Glove control expires 1 s after the last `WHEELS` line, and the HTTP link is marked disconnected after 5 s without any message.
- Bad glove bodies are rejected (400) and never forwarded.

## Versions and Tags

Each version has its own backend and HUD file (see the table at the top), and git tags mark each version. `git checkout <tag>` shows the whole repository as it was at that point.

| Phase | Version | Tag |
|---|---|---|
| Phase 1 | Backend V1 | `backend-v1.0` |
| Phase 2 | Backend V2 | Tag will be added when finalized |
