// Glove V1 Code  //
// Om Patil       //
// 2026-11-05     //

#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "secrets.h"

#define MPU 0x68
#define SDA D5
#define SCL D6
#define DIRE D8
#define BTN_ROLL D9
#define BTN_PITCH D10

float ax, ay, az;
float rawpitch, rawroll, corrpitch, corrroll, neutralPitch, neutralRoll;
int pitch, roll, speed, turnSpeed, leftSpeed, rightSpeed, leftInten, rightInten, lastLeft, lastRight;
bool minPitch, minRoll, rollPressed, pitchPressed, armed, forceSend;
unsigned long releasedAt = 0, lastRollChange = 0, lastPitchChange = 0, lastWifiTry = 0, lastPoll = 0;
WiFiClient client;

const int WARN = 80;
const int STOP = 25;
const int PITCH_LIMIT = 45;
const int ROLL_LIMIT = 70;
const int NEUTRAL_PITCH_LIMIT = 45;
const int NEUTRAL_ROLL_LIMIT = 70;
const unsigned long SAFETY_TIMEOUT = 1000;
const unsigned long DEBOUNCE_MS = 20;
const char* PI_HOST = "192.168.1.100";
const int PI_PORT = 5000;
const unsigned long HTTP_TIMEOUT_MS = 300;
const unsigned long WIFI_RETRY_MS = 5000;
const unsigned long PROX_POLL_MS = 200;
const unsigned long HEARTBEAT_MS = 2000;
const float DIVISOR_MIN = 0.5;
const bool ENABLE_PIVOT = false;
int dist = 400;
int vibeInten;
bool blocked;

void setup() {
  Serial.begin(115200);
  pinMode(DIRE, OUTPUT);
  pinMode(BTN_ROLL, INPUT_PULLUP);
  pinMode(BTN_PITCH, INPUT_PULLUP);
  Wire.begin(SDA, SCL);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  client.setNoDelay(true);
  lastWifiTry = millis();
  delay(100);
  while (true) {
    Wire.beginTransmission(MPU);
    if (Wire.endTransmission() == 0) break;
    Serial.println("MPU6050 not found.");
    delay(5000);
  }
  writeMPU(0x6B, 0x00);
  writeMPU(0x1C, 0x08);
  writeMPU(0x1A, 0x04);
}

void writeMPU(byte reg, byte val) {
  Wire.beginTransmission(MPU);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

void readAngles() {
  Wire.beginTransmission(MPU);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 6);
  ax = (int16_t)(Wire.read() << 8 | Wire.read()) / 8192.0;
  ay = (int16_t)(Wire.read() << 8 | Wire.read()) / 8192.0;
  az = (int16_t)(Wire.read() << 8 | Wire.read()) / 8192.0;
  //-x pitch down (forward), +y roll right, +z up
  //g points +z
  rawpitch = atan2(-ax, az) * 180 / PI;
  rawroll = atan2(ay, sqrt(ax * ax + az * az)) * 180 / PI;
  corrpitch = rawpitch;
  corrroll = rawroll;
}

// ---------- Network ----------
bool wifiUp() {return WiFi.status() == WL_CONNECTED;}

void maintainWifi() {
  if (wifiUp() || millis() - lastWifiTry < WIFI_RETRY_MS) return;
  lastWifiTry = millis();
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

void parseProx(String cmd) {
  cmd.trim();
  if (!cmd.startsWith("PROX:")) return;
  int firstComma = cmd.indexOf(',');
  int secondComma = cmd.indexOf(',', firstComma + 1);
  if (firstComma > 5 && secondComma > firstComma) {
    dist = cmd.substring(5, firstComma).toInt();
    blocked = cmd.substring(firstComma + 1, secondComma).toInt();
  }
}

void postLine(const String& body) {
  lastPoll = millis();
  HTTPClient http;
  http.setReuse(true);
  http.setConnectTimeout(HTTP_TIMEOUT_MS);
  http.setTimeout(HTTP_TIMEOUT_MS);
  if (!http.begin(client, PI_HOST, PI_PORT, "/glove")) return;
  http.addHeader("Content-Type", "text/plain");
  if (http.POST(body) == 200) {parseProx(http.getString());}
  http.end();
}

// ---------- Neutral ----------
void applyNeutral() {
  float divisor = cos(neutralRoll * PI / 180);
  if (divisor < DIVISOR_MIN) {divisor = DIVISOR_MIN;}
  float relPitch = (corrpitch - neutralPitch) / divisor;
  float relRoll = corrroll - neutralRoll;
  pitch = constrain(relPitch, -PITCH_LIMIT, PITCH_LIMIT);
  roll = constrain(relRoll, -ROLL_LIMIT, ROLL_LIMIT);
}

void zeroRoll() {
  float sumRoll = 0;
  for (int i = 0; i < 10; i++) {
    readAngles();
    sumRoll += corrroll;
    delay(5);
  }
  float avgRoll = sumRoll / 10;
  neutralRoll = constrain(avgRoll, -NEUTRAL_ROLL_LIMIT, NEUTRAL_ROLL_LIMIT);
  readAngles();
  applyNeutral();
}

void zeroPitch() {
  float sumPitch = 0;
  for (int i = 0; i < 10; i++) {
    readAngles();
    sumPitch += corrpitch;
    delay(5);
  }
  float avgPitch = sumPitch / 10;
  neutralPitch = constrain(avgPitch, -NEUTRAL_PITCH_LIMIT, NEUTRAL_PITCH_LIMIT);
  readAngles();
  applyNeutral();
}

void checkButtons() {
  unsigned long now = millis();
  bool rollRaw = digitalRead(BTN_ROLL) == LOW;
  bool pitchRaw = digitalRead(BTN_PITCH) == LOW;
  if (rollRaw != rollPressed && now - lastRollChange >= DEBOUNCE_MS) {
    rollPressed = rollRaw;
    if (rollPressed) {zeroRoll(); forceSend = true;}
    else {releasedAt = millis();}
    lastRollChange = millis();
  }
  if (pitchRaw != pitchPressed && now - lastPitchChange >= DEBOUNCE_MS) {
    pitchPressed = pitchRaw;
    if (pitchPressed) {zeroPitch(); releasedAt = 0; forceSend = true;}
    lastPitchChange = millis();
  }
  armed = rollPressed || (releasedAt != 0 && millis() - releasedAt < SAFETY_TIMEOUT);
}

// ---------- Wheels ----------
void pitchToSpeed() {
  if (pitch <= 15 && pitch >= -15) {minPitch = false;}
  else {minPitch = true;}
  if (minPitch) {
    speed = map(constrain(abs(pitch), 15, PITCH_LIMIT), 15, PITCH_LIMIT, 1, 100);
    if (pitch < 0) {leftInten = rightInten = -speed;}
    else {leftInten = rightInten = speed;}
  }
  else {leftInten = rightInten = 0;}
}

void rolltoSpeed() {
  if (roll <= 15 && roll >= -15) {minRoll = false;}
  else {minRoll = true;}
  if (minRoll) {
    if (!minPitch) {
      if (ENABLE_PIVOT) {
        turnSpeed = map(constrain(abs(roll), 15, ROLL_LIMIT), 15, ROLL_LIMIT, 1, 100);
        if (roll < 0) {
          leftInten = -turnSpeed;
          rightInten = turnSpeed;
        }
        else {
          leftInten = turnSpeed;
          rightInten = -turnSpeed;
        }
      }
    }
    else {
      turnSpeed = map(constrain(abs(roll), 15, ROLL_LIMIT), 15, ROLL_LIMIT, speed, 0);
      if (pitch < 0) {turnSpeed = -turnSpeed;}
      if (roll < 0) {leftInten = turnSpeed;}
      else {rightInten = turnSpeed;}
    }
  }
}

void sendWheels() {
  if (!forceSend && leftInten == 0 && rightInten == 0 && lastLeft == 0 && lastRight == 0) return;
  String line = "WHEELS:" + String(leftInten) + "," + String(rightInten) + "|";
  line += String(pitchPressed ? 0 : pitch) + "," + String(roll) + "|";
  line += String(neutralPitch, 0) + "," + String(neutralRoll, 0);
  if (wifiUp()) {postLine(line);}
  else {Serial.println(line);}
  lastLeft = leftInten;
  lastRight = rightInten;
  forceSend = false;
}

// ---------- Prox ----------
void proxToVibe() {
  if (!armed) {analogWrite(DIRE, 0);}
  else if (blocked) {analogWrite(DIRE, 255);}
  else if (dist > WARN) {analogWrite(DIRE,0);}
  else {
    vibeInten = map(constrain(dist, STOP, WARN), WARN, STOP, 1, 255);
    analogWrite(DIRE, vibeInten);
  }
}

void receiveProx() {
  if (Serial.available() > 0) {parseProx(Serial.readStringUntil('\n'));}
  if (wifiUp() && millis() - lastPoll >= (armed ? PROX_POLL_MS : HEARTBEAT_MS)) {postLine("PING");}
  proxToVibe();
}

void wheelsTick() {
  maintainWifi();
  checkButtons();
  readAngles();
  applyNeutral();
  if (pitchPressed) {
    leftInten = rightInten = 0;
    forceSend = true;
  }
  else if (rollPressed) {
    pitchToSpeed();
    rolltoSpeed();
  }
  else if (!armed) {leftInten = rightInten = 0;}
  sendWheels();
  receiveProx();
}

// ---------- Main ----------
void loop() {wheelsTick(); delay(50);}