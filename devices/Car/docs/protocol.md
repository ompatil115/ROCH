# Serial Command Protocol

Communication between the Flask backend and the Arduino firmware uses plain-text commands over USB serial, one command per line, terminated with `\n`.

The car has two protocol versions:

- **V1:** separate Light, Move, and Scan commands.
- **V1.5:** a wheel-based drive command and a proximity telemetry message, supporting glove driving.

---

## V1

### Light
- `LIGHT:<r>,<g>,0` — sets red/green PWM values (0–255). Blue omitted in V1 due to PWM pin constraints.
- `LIGHT:0` — turns the light off.

### Move
- `DRIVE:<i>` — forward/backward, intensity -100 to 100. Forward has true variable speed; backward is binary (any negative value = max intensity).
- `TURN:<i>` — left/right, intensity -100 to 100, binary (any nonzero value = max intensity).
- `DRIVE:0` / `TURN:0` — stop, halts all motor output regardless of axis.

Move commands are sent continuously (10–20 Hz) while an input is active. A backend watchdog sends a stop command if updates stop arriving.

### Scan
- `SCAN` — triggers a 0–180° servo sweep with ultrasonic distance sampling.
- `SCAN_DATA:<angle>:<distance>` — one line per reading, sent by the Arduino during the sweep (distance in cm, 0–400).
- `SCAN_DONE` — sent once the sweep completes; the backend returns all collected readings to the browser as JSON.

Only one command type (Light, Move, Scan) executes at a time. The backend rejects Light/Move requests while a scan is in progress.

---

## V1.5

V1.5 replaces the V1 Move and Scan commands with a single wheel command and a proximity telemetry message. Glove and browser input are both translated by the backend into these messages, so the firmware sees one command format regardless of the source.

### Drive
- `WHEELS:<left>,<right>` — left and right wheel intensity, each -100 to 100. The sign sets direction and the magnitude sets speed. `WHEELS:0,0` stops both motors.

Wheel commands are sent continuously, about every 50 ms, while an input is active. Mixing of joystick and gesture input into left and right values happens on the glove or browser side, so the firmware only applies the values it receives.

### Telemetry
- `PROX:<dist>,<blocked>,<red>,<green>` — sent from the Arduino to the backend. `dist` is the forward distance in cm, `blocked` is the obstacle-blocked flag, and `red` and `green` are the current color values of the proximity indicator LED.

The backend forwards this telemetry to the glove and the browser.

### Safety behavior (firmware)
- **Command watchdog:** if no wheel command arrives within 500 ms, the firmware stops both motors. In V1 this watchdog ran in the Python backend; in V1.5 it runs on the Arduino.
- **Proximity stop:** the firmware warns as an obstacle approaches and stops forward motion when the obstacle is too close.
- **Hysteresis:** command processing uses hysteresis around thresholds to prevent flicker.
