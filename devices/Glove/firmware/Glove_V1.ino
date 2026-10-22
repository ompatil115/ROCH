//  Glove V1 Code (Archive — Gen 1 Wrist-Tilt Control)  //
//  Om Patil                                            //
//  2026-10-16                                          //

#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>

#define MPU_ADDR 0x68
#define DIRE 10
#define SSID "<secret>"
#define PASSWORD "<secret>"
#define HUD "http://192.168.1.50:5000"

float ax, ay, az;
int pitch, roll, speed, turnSpeed, leftSpeed, rightSpeed, leftInten, rightInten;
float fracTurnSpeed;
bool minPitch, minRoll;
const int WARN = 80;
const int STOP = 25;
int dist = 400;
int vibeInten;
bool blocked;
unsigned long lastPrint = 0;

int16_t read16() {
  int16_t hi = Wire.read();
  int16_t lo = Wire.read();
  return (hi << 8) | lo;
}

void mpuInit() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);            // wake up
  Wire.write(0x00);
  Wire.endTransmission();
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1C);            // +/-4 g
  Wire.write(0x08);
  Wire.endTransmission();
}

void readAngles() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 6);
  if (Wire.available() < 6) return;   // keep last good values on a bad read
  ax = read16() / 8192.0;
  ay = read16() / 8192.0;
  az = read16() / 8192.0;

  float offsetRad = 45.0 * PI / 180.0;
  float axC = ax * cos(offsetRad) - az * sin(offsetRad);
  float azC = ax * sin(offsetRad) + az * cos(offsetRad);
  int rawPitch = atan2(-axC, sqrt(ay * ay + azC * azC)) * 180 / PI;
  int rawRoll  = atan2(ay, azC) * 180 / PI;
  pitch = -rawRoll;
  roll  = -rawPitch;
}

// ---------- Wheels ----------

void pitchToSpeed() {
  minPitch = !(pitch <= 25 && pitch >= -25);
  if (minPitch) {
    speed = map(constrain(abs(pitch), 25, 75), 25, 75, 1, 100);
    leftInten = rightInten = (pitch < 0) ? -speed : speed;
  } else {
    leftInten = rightInten = 0;
  }
}

void rollToSpeed() {
  minRoll = !(roll <= 15 && roll >= -15);
  if (!minRoll) return;

  turnSpeed = map(constrain(abs(roll), 15, 75), 15, 75, 1, 100);
  if (!minPitch) {
    if (roll < 0) {
      leftInten = -turnSpeed;
      rightInten = turnSpeed;
    } else {
      leftInten = turnSpeed;
      rightInten = -turnSpeed;
    }
  } else {
    fracTurnSpeed = map(constrain(abs(roll), 15, 75), 15, 75, 100, 0) / 100.0;
    turnSpeed = speed * (fracTurnSpeed * fracTurnSpeed);
    if (pitch < 0) turnSpeed = -turnSpeed;
    if (roll < 0) leftInten = turnSpeed;
    else rightInten = turnSpeed;
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
  if (blocked) {
    analogWrite(DIRE, 255);
  } else if (dist > WARN) {
    analogWrite(DIRE, 0);
  } else {
    vibeInten = map(constrain(dist, STOP, WARN), WARN, STOP, 1, 255);
    analogWrite(DIRE, vibeInten);
  }
}

void receiveProx() {
  if (Serial.available() > 0) {
    Serial.println("got something");
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    Serial.print("received: ");
    Serial.println(cmd);

    if (cmd.startsWith("PROX:")) {
      int firstComma = cmd.indexOf(',');
      int secondComma = cmd.indexOf(',', firstComma + 1);
      if (firstComma > 5 && secondComma > firstComma) {
        dist = cmd.substring(5, firstComma).toInt();
        blocked = cmd.substring(firstComma + 1, secondComma).toInt();
      }
    }
  }
  proxToVibe();
}

void wheelsTick() {
  readAngles();
  pitchToSpeed();
  rollToSpeed();
  if (millis() - lastPrint >= 250) {
    sendWheels();
    lastPrint = millis();
  }
  receiveProx();
}

// ---------- Main ----------

void setup() {
  Serial.begin(115200);
  pinMode(DIRE, OUTPUT);
  Wire.begin(9, 8);
  delay(100);
  mpuInit();

  /*
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
  }
  Serial.println(WiFi.localIP());
  */
}

void loop() {
  wheelsTick();
  delay(50);
}