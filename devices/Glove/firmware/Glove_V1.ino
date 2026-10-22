//  Glove V1 Code  //
//  Om Patil       //
//  2026-10-21     //

#include <Wire.h>
#include <Arduino_Modulino.h>

#define MPU_ADDR 0x68
#define WHO_AM_I_REG 0x75
#define DIRE 10

ModulinoJoystick joystick;

int roll;
int rollRef;
int pivotSpeed;
int joyX, joyY;
bool pivotHeld;
bool wasPivotHeld;
int leftInten, rightInten;

const int PIVOT_RANGE = 45;
const int DEAD_ZONE = 10;

const int WARN_DIST = 80;
const int STOP_DIST = 25;
int dist = 400;
int vibeInten;
bool blocked;

unsigned long lastSend = 0;
const unsigned long SEND_INTERVAL = 50;

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(20);
  pinMode(DIRE, OUTPUT);
  Wire.begin(9, 8);
  delay(100);
  mpuInit();
  mpuConfirm();
  Modulino.begin();
  joystick.begin();
}

// ---------- Wheels ----------

int16_t read16() {
  int16_t hi = Wire.read();
  int16_t lo = Wire.read();
  return (hi << 8) | lo;
}

void mpuInit() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0x00);
  Wire.endTransmission();
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1C);
  Wire.write(0x08);
  Wire.endTransmission();
}

void mpuConfirm() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(WHO_AM_I_REG);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 1);
  byte id = Wire.available() ? Wire.read() : 0x00;
  Serial.print("MPU WHO_AM_I: 0x");
  Serial.println(id, HEX);
}

bool readRoll() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 6);
  if (Wire.available() < 6) return false;
  float ax = read16() / 8192.0;
  float ay = read16() / 8192.0;
  float az = read16() / 8192.0;

  int rawPitch = atan2(-ax, sqrt(ay * ay + az * az)) * 180 / PI;
  roll = -rawPitch;
  return true;
}

void readJoystick() {
  joystick.update();
  joyX = joystick.getX();
  joyY = joystick.getY();
  if (abs(joyX) < DEAD_ZONE) joyX = 0;
  if (abs(joyY) < DEAD_ZONE) joyY = 0;
  pivotHeld = joystick.isPressed();
}

void pivot() {
  int delta = roll - rollRef;
  pivotSpeed = map(constrain(abs(delta), 0, PIVOT_RANGE), 0, PIVOT_RANGE, 0, 100);
  if (delta < 0) {
    leftInten = -pivotSpeed;
    rightInten = pivotSpeed;
  } else {
    leftInten = pivotSpeed;
    rightInten = -pivotSpeed;
  }
}

void drive() {
  float turnFactor = abs(joyY) / 100.0;
  int outer = joyX;
  int inner = joyX * (1.0 - turnFactor * turnFactor);

  if (joyY < 0) {
    leftInten = inner;
    rightInten = outer;
  } else {
    leftInten = outer;
    rightInten = inner;
  }
}

void sendWheels() {
  Serial.print("WHEELS:");
  Serial.print(leftInten);
  Serial.print(",");
  Serial.println(rightInten);
}

// ---------- Prox ----------

void proxToVibe() {
  if (blocked) { analogWrite(DIRE, 255); }
  else if (dist > WARN_DIST) { analogWrite(DIRE, 0); }
  else {
    vibeInten = map(constrain(dist, STOP_DIST, WARN_DIST), WARN_DIST, STOP_DIST, 1, 255);
    analogWrite(DIRE, vibeInten);
  }
}

bool parseProx() {
  String cmd;
  if (!cmd.startsWith("PROX:")) return false;
  int firstComma = cmd.indexOf(',');
  int secondComma = cmd.indexOf(',', firstComma + 1);
  if (firstComma <= 5 || secondComma <= firstComma) return false;
  dist = cmd.substring(5, firstComma).toInt();
  blocked = cmd.substring(firstComma + 1, secondComma).toInt();
  return true;
}

void receiveProx() {
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    parseProx(cmd);
  }
  proxToVibe();
}

// ---------- Main ----------

void loop() {
  readJoystick();

  if (pivotHeld) {
    if (readRoll()) {
      if (!wasPivotHeld) rollRef = roll;
      wasPivotHeld = true;
      pivot();
    } else {
      leftInten = 0;
      rightInten = 0;
    }
  } else {
    wasPivotHeld = false;
    drive();
  }

  if (millis() - lastSend >= SEND_INTERVAL) {
    sendWheels();
    lastSend = millis();
  }

  receiveProx();
}
