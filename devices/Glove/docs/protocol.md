# Glove Communication Protocol

The glove sends wheel commands to the Raspberry Pi backend and receives the car's proximity telemetry in return. Glove V1 uses one message format on two links:

- **HTTP (primary):** each message is sent as the body of an HTTP `POST` to `http://<pi-ip>:5000/glove`. The reply carries the telemetry.
- **USB serial (fallback):** the same messages as plain-text lines at 115200 baud, terminated with `\n`, used when Wi-Fi is down.

The backend translates glove messages into the car's serial protocol. See the [car protocol](../../Car/docs/protocol.md) for the car side.

---

## Drive

```text
WHEELS:<left>,<right>|<pitch>,<roll>|<neutralPitch>,<neutralRoll>
```

- `left`, `right`: wheel intensity, each -100 to 100. The sign sets direction and the magnitude sets speed. `WHEELS:0,0,...` stops both motors.
- `pitch`, `roll`: the current wrist angles relative to neutral, in degrees (pitch ±45, roll ±70).
- `neutralPitch`, `neutralRoll`: the neutral angles set by the buttons, in degrees.

The second and third groups are for the HUD. The backend accepts a line with only the wheel values (`WHEELS:<left>,<right>`) and forwards only those two values to the car. Out-of-range values are clamped.

## Keep-alive

```text
PING
```

HTTP only. The glove sends `PING` to keep the link marked connected and to get fresh telemetry. It sends one every 200 ms while armed (roll/safety button held, or within the 1000 ms hold) and every 2 s otherwise. A `PING` does not count as driving.

## Telemetry

```text
PROX:<dist>,<blocked>,
```

`dist` is the forward distance in cm and `blocked` is the obstacle-blocked flag (0 or 1). Over HTTP this is the reply body to every `POST`. Over serial the backend writes it as a line. The vibration motor follows the latest value.

## Link behavior

- **HTTP timeout:** the glove waits 300 ms for a reply.
- **Fallback:** when `WiFi.status()` is not connected, the glove sends the same `WHEELS` lines over serial. It retries Wi-Fi every 5 s.
- **Priority:** the backend gives HTTP priority over serial while HTTP has driven within the last second, and ignores serial `WHEELS` lines during that time.
- **Connection state:** the backend reports an HTTP link as connected for 5 s after its last message, and shows both links in `/status`.

---

## Safety behavior

- **Press to drive:** the glove only sends non-zero wheel values while the roll/safety button is held.
- **Hold, then stop:** on release the glove keeps sending the last command for 1000 ms, then sends one `WHEELS:0,0` and goes silent (only `PING` continues).
- **Emergency stop:** while the pitch button is held, the glove forces both wheel values to zero. This overrides the roll/safety button, cancels the 1000 ms hold, and keeps the vibration motor off.
- **Command watchdog:** the car stops both motors if no wheel command arrives within 500 ms, and the backend drops glove control 1 s after the last command. Both run off the glove, so they cover a lost link.
- **Proximity stop:** the car's firmware stops forward motion when an obstacle is 25 cm or closer, regardless of what the glove sends.

These are designed behaviors, not a verified safety guarantee.
