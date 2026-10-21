# System Architecture

Three layers, each responsible for a distinct part of the system:

**Browser (JavaScript)** — renders the interface (light slider, movement pad, 
scan button/chart), computes derived values (RGB from slider position, 
direction/intensity from pad position), and sends commands to the backend 
over HTTP.

**Backend (Python/Flask, on Raspberry Pi)** — mediates between browser and 
hardware. Translates HTTP requests into serial commands, and parses serial 
responses back into JSON for the browser. Runs three concurrent threads: 
the main Flask request handler, a background serial reader (continuously 
parsing scan data and watching for completion), and a watchdog that halts 
motor output if movement commands stop arriving.

**Firmware (C++, on Arduino Uno)** — executes hardware control: PWM for 
motors and lighting, servo positioning, ultrasonic distance sampling. Parses 
incoming serial commands and dispatches to the matching function.

Computation was deliberately kept off the Arduino where possible (color 
math, direction/intensity logic) to minimize firmware complexity, since the 
Uno also handles time-sensitive sensor timing and motor control.

## Data flow
Browser → HTTP → Flask → Serial → Arduino → (physical action, or serial 
response) → Flask (background thread) → JSON → Browser


## V1 Hardware Constraint: PWM Pin Availability

The Arduino Uno has six PWM-capable pins. Attaching the Servo library 
claims Timer1, which disables PWM on two of them, leaving four usable 
PWM pins. Full-scope control (variable-speed in all four directions, 
plus full RGB) requires seven. V1 scoped down accordingly:

- **Light:** blue channel omitted entirely (saves one PWM pin); red and 
  green retain full gradient control.
- **Move:** only forward drive retains true variable speed (one PWM pin, 
  shared across both motors); backward and turning are binary — any 
  nonzero input runs at max intensity, since those directions use the 
  non-PWM motor pins.

This is a scoping decision, not a bug. Planned Version 2 fix: add a PCA9685 
PWM driver board, which provides 16 additional PWM channels, removing 
this constraint entirely.


## Physical Build

V1 used LEGO for the chassis, a deliberate rapid-prototyping choice to 
validate the electrical and software architecture without investing time 
in a custom frame before the control system was proven. Despite the 
material, the chassis followed real mechanical reasoning: a modular 
design (motors, battery, sensor/servo, and "brain" as separable units) 
for easy disassembly during debugging, with the battery — the heaviest 
component — placed directly over the wheel modules to minimize torque 
load during turns. Phase 2 replaces this with a soldered, purpose-built 
frame.


## Media

- **Full system demo (browser control → robot response):** https://youtu.be/HuvUpwN8BuI
- **Modular design:** https://youtu.be/toOqbU9Gd-k
- **Modular design breakdown:** https://youtu.be/CyCH8XhwyrI
- **Physical build:** [`media/robot-photo.jpg`](media/robot-photo.jpg)
- **Schematic:** [`media/schematic.png`](media/schematic.png) 
  ([KiCad source](media/robot-car.kicad_sch))
