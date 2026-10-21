//  Car V1.5 Code  //
//  Om Patil       //
//  2026-10-21     //

#include <SR04.h>

#define ENABLE 7
#define DIRA 5
#define DIRB 9
#define DIRC 6
#define DIRD 10
#define RED 11
#define GREEN 3
#define TRIG_PIN A1
#define ECHO_PIN A0

SR04 sr04(ECHO_PIN, TRIG_PIN);

const int WARN = 80;
const int STOP = 25;
const int CLEAR = 30;
const int GREEN_MAX = 185;
const unsigned long PROX_INTERVAL = 100;
const unsigned long CMD_TIMEOUT = 500;

int inten = 0;
int leftSpeed = 0;
int rightSpeed = 0;
int lSp = 0;
int rSp = 0;

long dist = 400;
int red = 0;
int green = GREEN_MAX;
bool blocked = false;
bool movingForward = false;
bool stopped = true;
unsigned long lastProx = 0;
unsigned long lastCmd = 0;

void setup() {
  pinMode(ENABLE, OUTPUT);
  pinMode(DIRA, OUTPUT);
  pinMode(DIRB, OUTPUT);
  pinMode(DIRC, OUTPUT);
  pinMode(DIRD, OUTPUT);
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(ENABLE, HIGH);
  Serial.begin(9600);
  Serial.setTimeout(20);
}

// ---------- Wheels ----------

int convert() {
  int m = abs(inten);
  if (m == 0) return 0;
  return map(m, 1, 100, 69, 255);
}

void allStop() {
  digitalWrite(DIRA, LOW);
  digitalWrite(DIRB, LOW);
  digitalWrite(DIRC, LOW);
  digitalWrite(DIRD, LOW);
  movingForward = false;
  stopped = true;
}

void driveForward() {
  inten = leftSpeed;
  lSp = convert();
  inten = rightSpeed;
  rSp = convert();
  analogWrite(DIRA, rSp);
  digitalWrite(DIRB, LOW);
  analogWrite(DIRC, lSp);
  digitalWrite(DIRD, LOW);
  movingForward = true;
  stopped = false;
}

void driveBackward() {
  inten = leftSpeed;
  lSp = convert();
  inten = rightSpeed;
  rSp = convert();
  digitalWrite(DIRA, LOW);
  analogWrite(DIRB, rSp);
  digitalWrite(DIRC, LOW);
  analogWrite(DIRD, lSp);
  movingForward = false;
  stopped = false;
}

void turnLeftPivot() {
  inten = leftSpeed;
  lSp = convert();
  inten = rightSpeed;
  rSp = convert();
  analogWrite(DIRA, rSp);
  digitalWrite(DIRB, LOW);
  digitalWrite(DIRC, LOW);
  analogWrite(DIRD, lSp);
  movingForward = false;
  stopped = false;
}

void turnRightPivot() {
  inten = leftSpeed;
  lSp = convert();
  inten = rightSpeed;
  rSp = convert();
  digitalWrite(DIRA, LOW);
  analogWrite(DIRB, rSp);
  analogWrite(DIRC, lSp);
  digitalWrite(DIRD, LOW);
  movingForward = false;
  stopped = false;
}

void turnRightForward() {
  inten = leftSpeed;
  lSp = convert();
  inten = rightSpeed;
  rSp = convert();
  analogWrite(DIRA, rSp);
  digitalWrite(DIRB, LOW);
  analogWrite(DIRC, lSp);
  digitalWrite(DIRD, LOW);
  movingForward = true;
  stopped = false;
}

void turnLeftForward() {
  inten = leftSpeed;
  lSp = convert();
  inten = rightSpeed;
  rSp = convert();
  analogWrite(DIRA, rSp);
  digitalWrite(DIRB, LOW);
  analogWrite(DIRC, lSp);
  digitalWrite(DIRD, LOW);
  movingForward = true;
  stopped = false;
}

void turnRightBackward() {
  inten = leftSpeed;
  lSp = convert();
  inten = rightSpeed;
  rSp = convert();
  digitalWrite(DIRA, LOW);
  analogWrite(DIRB, rSp);
  digitalWrite(DIRC, LOW);
  analogWrite(DIRD, lSp);
  movingForward = false;
  stopped = false;
}

void turnLeftBackward() {
  inten = leftSpeed;
  lSp = convert();
  inten = rightSpeed;
  rSp = convert();
  digitalWrite(DIRA, LOW);
  analogWrite(DIRB, rSp);
  digitalWrite(DIRC, LOW);
  analogWrite(DIRD, lSp);
  movingForward = false;
  stopped = false;
}

void applyWheels() {
  leftSpeed  = constrain(leftSpeed,  -100, 100);
  rightSpeed = constrain(rightSpeed, -100, 100);

  if (blocked && leftSpeed >= 0 && rightSpeed >= 0 && (leftSpeed > 0 || rightSpeed > 0)) {
    leftSpeed = 0;
    rightSpeed = 0;
  }

  if (leftSpeed == 0 && rightSpeed == 0) {
    if (!stopped) allStop();
  }
  else if (leftSpeed > 0 && rightSpeed < 0) turnRightPivot();
  else if (leftSpeed < 0 && rightSpeed > 0) turnLeftPivot();
  else if (leftSpeed >= 0 && rightSpeed >= 0) {
    if (leftSpeed > rightSpeed) turnRightForward();
    else if (rightSpeed > leftSpeed) turnLeftForward();
    else driveForward();
  }
  else {
    if (leftSpeed < rightSpeed) turnRightBackward();
    else if (rightSpeed < leftSpeed) turnLeftBackward();
    else driveBackward();
  }
}

bool parseWheels(String cmd) {
  if (!cmd.startsWith("WHEELS:")) return false;
  int comma = cmd.indexOf(',');
  if (comma <= 7) return false;
  leftSpeed  = cmd.substring(7, comma).toInt();
  rightSpeed = cmd.substring(comma + 1).toInt();
  return true;
}

void receiveWheels() {
  if (blocked && movingForward) {
    if (!stopped) allStop();
  }
  if (millis() - lastCmd > CMD_TIMEOUT) {
    if (!stopped) allStop();
  }
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (parseWheels(cmd)) {
      lastCmd = millis();
      applyWheels();
    }
  }
}

// ---------- Prox ----------

void readDistance() {
  long reading = sr04.Distance();
  if (reading > 400) reading = 400;
  dist = reading;
}

void setLight() {
  analogWrite(RED, red);
  analogWrite(GREEN, green);
}

void distToColor() {
  if (blocked) {
    red = 255;
    green = 0;
  }
  else if (dist < WARN) {
    red = map(dist, STOP, WARN, 255, 0);
    green = map(dist, STOP, WARN, 0, GREEN_MAX);
  }
  else {
    red = 0;
    green = GREEN_MAX;
  }
}

void updateBlocked() {
  if (!blocked && dist <= STOP) blocked = true;
  else if (blocked && dist > CLEAR) blocked = false;
}

void sendProx() {
  Serial.print("PROX:");
  Serial.print(dist);
  Serial.print(",");
  Serial.print(blocked ? 1 : 0);
  Serial.print(",");
  Serial.print(red);
  Serial.print(",");
  Serial.println(green);
}

void proxTick() {
  if (millis() - lastProx < PROX_INTERVAL) return;
  lastProx = millis();
  readDistance();
  updateBlocked();
  distToColor();
  setLight();
  sendProx();
}

// ---------- Main ----------

void loop() {
  proxTick();
  receiveWheels();
}