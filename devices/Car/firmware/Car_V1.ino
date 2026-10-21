//  Robot V1 Code  //
//  Om Patil       //
//  2026-09-29     //

#include <Servo.h>
#include <SR04.h>

// ---------- Pin Definitions ----------
#define ENABLE 7
#define DIRA 5
#define DIRB 9
#define DIRC 6
#define DIRD 10
#define RED 11
#define GREEN 3
//#define BLUE 4
#define SERVO 12
#define TRIG_PIN A1
#define ECHO_PIN A0

Servo servo;
SR04 sr04(ECHO_PIN, TRIG_PIN);

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
  servo.attach(SERVO);
  digitalWrite(ENABLE, HIGH);
  Serial.begin(9600);
}

// ---------- Light ----------
void setLight(int r, int g) {
  analogWrite(RED, r);
  analogWrite(GREEN, g);
}

void lightOff() {
  setLight(0, 0);
}

// ---------- Move ----------
void allStop() {
  digitalWrite(DIRA, LOW);
  digitalWrite(DIRB, LOW);
  digitalWrite(DIRC, LOW);
  digitalWrite(DIRD, LOW);
}

void driveForward(int i) {
  int speed = map(i, 1, 100, 69, 255);
  analogWrite(DIRA, speed);
  digitalWrite(DIRB, LOW);
  analogWrite(DIRC, speed);
  digitalWrite(DIRD, LOW);
}

void driveBackward() {
  digitalWrite(DIRA, LOW);
  digitalWrite(DIRB, HIGH);
  digitalWrite(DIRC, LOW);
  digitalWrite(DIRD, HIGH);
}

void turnLeft() {
  digitalWrite(DIRA, LOW);
  digitalWrite(DIRB, HIGH);
  digitalWrite(DIRC, HIGH);
  digitalWrite(DIRD, LOW);
}

void turnRight() {
  digitalWrite(DIRA, HIGH);
  digitalWrite(DIRB, LOW);
  digitalWrite(DIRC, LOW);
  digitalWrite(DIRD, HIGH);
}

void drive(int i) {
  if (i > 0) driveForward(i);
  else if (i < 0) driveBackward();
  else allStop();
}

void turn(int i) {
  if (i > 0) turnRight();
  else if (i < 0) turnLeft();
  else allStop();
}

// ---------- Scan ----------
void sweep(int angle) {
  servo.write(angle);
}

long readDistance() {
  return sr04.Distance();
}

void scan() {
  for (int angle = 0; angle <= 180; angle += 10) {
    sweep(angle);
    delay(300);
    long distance = readDistance();
    Serial.print("SCAN_DATA:");
    Serial.print(angle);
    Serial.print(":");
    Serial.println(distance);
  }
  sweep(90);
  Serial.println("SCAN_DONE");
}

void loop() {
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd.startsWith("LIGHT:")) {
      String rest = cmd.substring(6);

      if (rest == "0") {
        lightOff();
      } else {
        int firstComma = rest.indexOf(',');
        int secondComma = rest.indexOf(',', firstComma + 1);

        int r = rest.substring(0, firstComma).toInt();
        int g = rest.substring(firstComma + 1, secondComma).toInt();

        setLight(r, g);
      }
    }

    else if (cmd.startsWith("DRIVE:")) {
      int i = cmd.substring(6).toInt();
      drive(i);
    }

    else if (cmd.startsWith("TURN:")) {
      int i = cmd.substring(5).toInt();
      turn(i);
    }

    else if (cmd == "SCAN") {
      scan();
    }
  }
}
