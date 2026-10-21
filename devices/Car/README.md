# Embedded Robotic Vehicle Control System (Robot Car V1)

A browser-controlled robotic vehicle built on a Raspberry Pi and Arduino, 
supporting remote-controlled movement, RGB lighting, and ultrasonic distance 
scanning over a local network. Built as a first full-stack embedded systems 
project, from protocol design through firmware, backend, and interface.

## Overview

- **Firmware (C++, Arduino Uno)** — motor, servo, and ultrasonic sensor control
- **Backend (Python/Flask, Raspberry Pi)** — serial-to-HTTP bridge, 
  multithreaded command handling, safety watchdog
- **Interface (HTML/CSS/JS)** — browser-based control panel: color picker, 
  directional movement pad, and a live scan visualization

See [`docs/architecture.md`](docs/architecture.md) for system design and 
[`docs/protocol.md`](docs/protocol.md) for the full serial command spec.

## Features

- Real-time RGB lighting control via a gradient slider
- Directional drive/turn control with variable-speed forward movement
- 180° ultrasonic distance scanning with live results display
- Thread-safe backend handling concurrent serial I/O and command locking
- Motor safety watchdog that halts output if control input stops

## Hardware

- Raspberry Pi 5 (headless, Linux)
- Arduino Uno R3
- DRV8833 dual motor driver
- 2x 6V DC motors
- SG90 micro servo
- HC-SR04 ultrasonic sensor
- RGB LED

## Setup

**Firmware**
1. Open `firmware/robot_control.ino` in the Arduino IDE
2. Install the `Servo` and `SR04` libraries
3. Upload to an Arduino Uno wired per the pin map in `docs/architecture.md`

**Backend**
```bash
cd backend
pip install -r requirements.txt
python3 app.py
```
Connect the Arduino to the Pi via USB, then visit `http://<pi-ip>:5000` 
from any device on the same network.

## Status

Version 1 is complete and functional: all three subsystems (light, move, 
scan) work end-to-end, browser to hardware and back. V1 ran against a 
hardware constraint — attaching the servo library consumes a timer the 
Uno needs for PWM on two pins, leaving too few PWM-capable pins for full 
RGB and full variable-speed control in both directions. See 
`docs/architecture.md` for the tradeoff and what V1 scoped out as a result.
