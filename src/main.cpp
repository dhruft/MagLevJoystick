#include <Arduino.h>

/* --- Calibration & Geometry --- */
// Target "Center" values for each sensor (approx 750 based on previous tests)
const int S_CENTER[] = {750, 750, 750, 750};
const int S_RANGE = 700; // Expected swing from center to limit

/* --- Pin Definitions --- */
struct MagnetPins {
  int rpwm;
  int lpwm;
};

// 4 Magnets at 0, 90, 180, 270 degrees
MagnetPins front = {19, 21}; // Y+
MagnetPins right = {4, 2};   // X+
MagnetPins back = {5, 18};   // Y-
MagnetPins left = {17, 16};  // X-

// 4 Sensors at 45, 135, 225, 315 degrees
const int IR_SENSORS[] = {36, 39, 34, 35};
const int NUM_SENSORS = 4;

/* --- Haptic Joystick API --- */

/**
 * Reads sensors and calculates XY position.
 * Returns values from -100 to 100.
 */
void getPosition(int &x, int &y) {
  int s1 = analogRead(IR_SENSORS[0]); // 45°  (Front-Left)
  int s2 = analogRead(IR_SENSORS[1]); // 135° (Back-Left)
  int s3 = analogRead(IR_SENSORS[2]); // 225° (Back-Right)
  int s4 = analogRead(IR_SENSORS[3]); // 315° (Front-Right)

  // Y-axis (Front - Back)
  y = ((s1 + s4) - (s2 + s3)) / 2;
  // X-axis (Right - Left)
  x = ((s3 + s4) - (s1 + s2)) / 2;

  // Map to -100 to 100 range (approximate mapping)
  x = constrain(map(x, -S_RANGE, S_RANGE, -100, 100), -100, 100);
  y = constrain(map(y, -S_RANGE, S_RANGE, -100, 100), -100, 100);
}

/**
 * Sets raw power to a magnet based on polarity rules.
 */
void setMagnetPower(MagnetPins m, int pwr) {
  int val = constrain(abs(pwr), 0, 255);
  if (pwr >= 0) {
    analogWrite(m.rpwm, val);
    analogWrite(m.lpwm, 0);
  } else {
    analogWrite(m.rpwm, 0);
    analogWrite(m.lpwm, val);
  }
}

/**
 * Applies XY force vectors.
 * fx, fy: -255 to 255
 */
void applyForce(int fx, int fy) {
  // Y-AXIS FORCE
  // To move Front (fy > 0): Pull Front (Atr) + Push Back (Rep)
  setMagnetPower(front, fy); // Assuming positive is attractive for Front
  setMagnetPower(back, fy);  // Assuming positive is repulsive for Back

  // X-AXIS FORCE
  // To move Right (fx > 0): Pull Right (Atr) + Push Left (Rep)
  setMagnetPower(right, fx); // Assuming positive is attractive for Right
  setMagnetPower(left, fx);  // Assuming positive is repulsive for Left
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);

  pinMode(front.rpwm, OUTPUT);
  pinMode(front.lpwm, OUTPUT);
  pinMode(back.rpwm, OUTPUT);
  pinMode(back.lpwm, OUTPUT);
  pinMode(left.rpwm, OUTPUT);
  pinMode(left.lpwm, OUTPUT);
  pinMode(right.rpwm, OUTPUT);
  pinMode(right.lpwm, OUTPUT);
}

void loop() {
  int x, y;
  getPosition(x, y);

  // 1. Broadcast Position
  Serial.print("POS:");
  Serial.print(x);
  Serial.print(",");
  Serial.println(y);

  // 2. Read Force Commands: SETF:fx,fy
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    if (input.startsWith("SETF:")) {
      int commaIndex = input.indexOf(',');
      if (commaIndex != -1) {
        int fx = input.substring(5, commaIndex).toInt();
        int fy = input.substring(commaIndex + 1).toInt();
        applyForce(fx, fy);
      }
    }
  }

  delay(10);
}
