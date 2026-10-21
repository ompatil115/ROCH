# System Architecture

The car has two versions, V1 (browser control) and V1.5 (adds glove driving). Both use the same three-layer structure, with the responsibilities of each layer described below.

## Layers

**Browser (JavaScript)**: renders the interface (light slider, movement pad, and in V1 the scan button and radar-style scan graph), computes derived values (RGB from slider position, direction and intensity from pad position), and sends commands to the backend over HTTP.

**Backend (Python/Flask, on Raspberry Pi)**: mediates between the browser, the glove, and the hardware. It translates HTTP requests into serial commands and parses serial responses back into JSON for the browser.

- *V1:* three concurrent threads: the main Flask request handler, a background serial reader (continuously parsing scan data and watching for completion), and a watchdog that halts motor output if movement commands stop arriving.
- *V1.5:* the watchdog moved into the Arduino firmware. The backend runs the Flask request handler plus background serial readers for the car and for glove traffic, relaying wheel commands to the car and car telemetry back to the glove.

**Firmware (C++, on Arduino Uno)**: executes hardware control: PWM for motors and lighting, ultrasonic distance sampling (and, in V1, servo positioning). It parses incoming serial commands and dispatches to the matching function.

- *V1.5 adds:* a 500 ms command watchdog that stops the motors, proximity warning and automatic stop based on the ultrasonic sensor, hysteresis in command processing to prevent flicker near thresholds, and periodic proximity telemetry back to the backend.

Computation was deliberately kept off the Arduino where possible (color math, direction and intensity logic) to minimize firmware complexity, since the Uno also handles time-sensitive sensor timing and motor control. In V1.5, input mixing happens on the glove, so the car firmware only applies left and right wheel values.

## Data flow

**V1:**
Browser → HTTP → Flask → Serial → Arduino → (physical action, or serial response) → Flask (background thread) → JSON → Browser

**V1.5:**
Glove or Browser → HTTP → Flask → Serial → Arduino → (wheels and lighting)
Arduino → Serial (proximity telemetry) → Flask → Glove and Browser

## V1 Hardware Constraint: PWM Pin Availability

The Arduino Uno has six PWM-capable pins. Attaching the Servo library claims Timer1, which disables PWM on two of them, leaving four usable PWM pins. Full-scope control (variable-speed in all four directions, plus full RGB) requires seven. V1 scoped down accordingly:

- **Light:** blue channel omitted entirely (saves one PWM pin); red and green retain full gradient control.
- **Move:** only forward drive retains true variable speed (one PWM pin, shared across both motors); backward and turning are binary: any nonzero input runs at max intensity, since those directions use the non-PWM motor pins.

This was a scoping decision, not a bug.

   **V1.5 resolution:** the scanning servo is no longer used, so Timer1 and its PWM pins are free again and the constraint no longer applies. The blue channel was not restored; V1.5 lighting is focused on proximity indication.

## Physical Build

V1 used LEGO for the chassis, a deliberate rapid-prototyping choice to validate the electrical and software architecture without investing time in a custom frame before the control system was proven. Despite the material, the chassis followed real mechanical reasoning: a modular design (motors, battery, sensor/servo, and "brain" as separable units) for easy disassembly during debugging, with the battery, the heaviest component, placed directly over the wheel modules to minimize torque load during turns.

## Media

- **Full system demo (browser control → robot response):** https://youtu.be/HuvUpwN8BuI
- **Modular design:** https://youtu.be/toOqbU9Gd-k
- **Modular design breakdown:** https://youtu.be/CyCH8XhwyrI
- **Physical build:** [`media/robot-photo.jpg`](media/robot-image.jpg)
- **Schematic:** [`media/schematic.png`](media/schematic.png)
  ([KiCad source](media/Car_V1.kicad_sch))
