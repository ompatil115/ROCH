# Glove Communication Protocol

The glove sends wheel commands to the Raspberry Pi backend and receives the car's proximity telemetry in return. The message content is the same in both versions, and only the transport differs:

- **V1:** plain-text lines over USB serial, one message per line, terminated with `\n`.
- **V1.5:** HTTP requests carrying JSON over Wi-Fi.

The backend translates glove messages into the car's serial protocol. See the [car protocol](../../Car/docs/protocol.md) for the car side.

---

## V1 (USB serial)

### Drive
- `WHEELS:<left>,<right>` — left and right wheel intensity, each -100 to 100. The sign sets direction, and the magnitude sets speed. `WHEELS:0,0` stops both motors.

### Telemetry
- `PROX:<dist>,<blocked>,<red>,<green>` — received from the backend. `dist` is the forward distance in cm, `blocked` is the obstacle-blocked flag, and `red` and `green` are the current color values of the car's proximity indicator LED.

While input is active, the glove sends a `WHEELS` message at an interval of 50 ms. It updates its vibration motor from the latest `PROX` message.

---

## V1.5 (Wi-Fi HTTP/JSON)

V1.5 carries the same drive and telemetry data as V1, sent as JSON over HTTP to the shared backend. Glove traffic has its own route, separate from the browser's, so the backend can tell glove input from browser input.

| Route | Method | Purpose |
|---|---|---|
| `/glove/car` | POST | Send wheel values, receive car telemetry |

### Drive request

While input is active, the glove sends the wheel values to `/glove/car` at an interval of 50 ms. The actual rate depends on the network round-trip time.

```json
{
  "left": 40,
  "right": 40
}
```

`left` and `right` each range from -100 to 100, as in V1.

### Telemetry response

The response to each request returns the car's latest proximity data.

```json
{
  "dist": 87,
  "blocked": false,
  "red": 0,
  "green": 255
}
```

The fields match the `PROX` message: `dist` is the forward distance in cm, `blocked` is the obstacle-blocked flag, and `red` and `green` are the proximity indicator LED color values.

---

## Safety behavior

- **Command watchdog:** The car stops both motors if no wheel command arrives within 500 ms. This covers a lost Wi-Fi link or a stopped glove, since the watchdog runs on the car's Arduino, not on the glove or the backend.
- **Proximity stop:** The car's firmware stops forward motion when an obstacle is too close, regardless of what the glove sends.
- **Silent at zero:** When the joystick is at rest, the glove sends a single zero command and then stops sending until input resumes.
- **Telemetry timeout (V1.5):** If no telemetry arrives for 500 ms, the glove stops using the stale data for haptics.
