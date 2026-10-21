# Robotics Operations and Control Hub (ROCH)

ROCH (Robotics Operations and Control Hub) is a personal engineering project that explores how multiple robotic devices can be monitored and controlled through a unified browser-based interface and a wearable control glove.

The project is developed in phases. Each phase delivers a milestone across four systems: the **car**, the **HUD** (browser interface), the **glove**, and the **arm**.

## Project Status

| Phase | Systems | Status |
|---|---|---|
| Phase 1 | Car V1, HUD V1 | Complete |
| Phase 2 | Glove V1, Car V1.5, HUD V1.5 | Complete |
| Phase 3 | Glove V2, Arm V1, HUD V2 | In progress: arm software drafted and protocol defined; hardware sourced, not yet assembled or tested |
| Phase 4+ | To be determined | Planned |

Items described as "planned" are design decisions, not working features.

## Phase 1: Car V1 and HUD V1

Phase 1 demonstrates end-to-end remote control and two-way communication between a browser interface and physical robotic hardware.

Car V1 functions:

1. Controllable multicolor lighting.
2. Forward, reverse, left, and right movement.
3. Distance scanning across a 180-degree field of view.

HUD V1 is a browser interface on the local network that sends commands to the car and shows the results, including a distance display with a radar-style scan graph.

### System Architecture

- **Browser interface:** lighting, movement, and scan controls, plus the scan graph.
- **Raspberry Pi backend:** Python/Flask service that handles HTTP requests, manages state, and communicates over USB serial.
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

See [`Robot-Car-V1/`](Robot-Car-V1/) for build documentation, firmware, backend code, and media.

## Phase 2: Glove V1, Car V1.5, HUD V1.5

Phase 2 adds a wearable control glove and updates the car and HUD to support glove driving, through both a joystick channel and a wrist-gesture channel.

### Glove V1

The glove is built around an ESP32-C3 with an I2C IMU, a joystick, and a vibration motor for haptic feedback. It sends drive commands and receives car telemetry every 50 ms.

**Car control scheme:**
- **Joystick (primary):** throttle and steering using a throttle-gated mix and a squared turn curve.
- **Wrist gesture (secondary):** holding the joystick button captures the current wrist angle as zero (press-to-zero); rotating about 45 degrees either way maps to pivot speed.
- **Haptic feedback:** the vibration motor responds to the car's proximity data.

A first-generation scheme mapped wrist pitch and roll directly to wheel speed. Logged trials led to the current joystick-primary design, and the earlier data and code are archived in this repository.

### Car V1.5 and HUD V1.5

Car V1.5 is a firmware update from the V1 move/light/scan protocol to a wheel-based protocol that supports glove driving, with forward distance sensing, a proximity warning and stop, and a command timeout. HUD V1.5 updates the browser interface to mirror glove input on the movement pad and to lock browser driving while the glove is in use.

### Protocol (glove, Raspberry Pi, car)

```text
WHEELS:<left>,<right>                 each -100..100      glove -> car
PROX:<dist>,<blocked>,<red>,<green>                       car -> glove
```

The car stops its motors if no command arrives within 500 ms.

## Phase 3: Glove V2, Arm V1, HUD V2

Phase 3 extends the glove to control a desk-mounted robotic arm driven directly by the Raspberry Pi through a PCA9685 servo driver, and expands the HUD to cover the arm and system health.

**Arm V1 payload target:** 1.5 kg.

### Arm control scheme

For the arm, wrist and forearm gestures are the primary control and the joystick is used for precision adjustments, the reverse of the car scheme. The same press-to-zero technique sets the reference position for gesture control.

### Arm protocol

```text
BASE:<pitch>,<yaw>             0-180 each
ELBOW:<pitch>                  0-180
WRIST:<pitch>,<yaw>,<roll>     0-180 each
FINGER:<grip>                  0-90
```

Values are target servo angles in degrees. The glove converts its own range to servo range before sending.

### Software

`Arm_V1/arm_controller.py` contains the `ArmController` class. It parses protocol strings, validates the joint count and angle range, and sets servo angles through the PCA9685. A single `execute()` call keeps the servo driver swappable, so it can later move to a microcontroller without changing the protocol. The parsing and clamping logic has been tested against simulated servos; hardware testing is pending.

### HUD V2

HUD V2 adds arm status and brief glove information, plus a communication status panel: connection state for each link, command rate, latency, and protocol and connectivity details.

### Planned

- Glove mode indicator showing whether the glove is in car or arm mode (not yet implemented in code).
- Press-and-hold mode switching between car and arm control.
- Flex-sensor gripper input.
- Claw-mounted IMU for position feedback and a distance sensor at the claw.
- Camera-based object detection on the Raspberry Pi.

## Roadmap

1. Assemble the arm and test `ArmController` with real servos.
2. Build Glove V2 and HUD V2.
3. Add automated tests, structured data logging, and CI.
4. Secure the communication links and extend operation beyond local Wi-Fi.
