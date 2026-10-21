# Serial Command Protocol

Communication between the Flask backend and Arduino firmware uses plain-text 
commands over USB serial, one command per line, terminated with `\n`.

## Light
- `LIGHT:<r>,<g>,0` — sets red/green PWM values (0–255). Blue omitted in V1 
  due to PWM pin constraints.
- `LIGHT:0` — turns the light off.

## Move
- `DRIVE:<i>` — forward/backward, intensity -100 to 100. Forward has true 
  variable speed; backward is binary (any negative value = max intensity).
- `TURN:<i>` — left/right, intensity -100 to 100, binary (any nonzero value 
  = max intensity).
- `DRIVE:0` / `TURN:0` — stop, halts all motor output regardless of axis.

Move commands are sent continuously (10–20 Hz) while an input is active. A 
backend watchdog sends a stop command if updates stop arriving.

## Scan
- `SCAN` — triggers a 0–180° servo sweep with ultrasonic distance sampling.
- `SCAN_DATA:<angle>:<distance>` — one line per reading, sent by the Arduino 
  during the sweep (distance in cm, 0–400).
- `SCAN_DONE` — sent once the sweep completes; the backend returns all 
  collected readings to the browser as JSON.

Only one command type (Light, Move, Scan) executes at a time. The backend 
rejects Light/Move requests while a scan is in progress.
