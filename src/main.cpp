#include "Adafruit_VL6180X.h"
#include "virtual_tof_coeffs.h"
#include <Arduino.h>
#include <Wire.h>
#include <cmath>
#include <cstdlib>

const float T1_TARGET = 48.6;
const float T2_TARGET = 49.0;

/* --- Pin Definitions (Teensy 4.1) --- */
struct MagnetPins {
  int rpwm;
  int lpwm;
};

struct TOFPins {
  int scl;
  int sda;
  int xshut;
};

// User provided pins
MagnetPins front = {37, 36};
MagnetPins back = {23, 22};
MagnetPins left = {30, 29};
MagnetPins right = {15, 14};

// Sensor on Analog A10/A11
const int SENSOR_1 = 24;
TOFPins tof1 = {16, 17, 40};

const int SENSOR_2 = 25;
TOFPins tof2 = {19, 18, 41};

Adafruit_VL6180X lox1 = Adafruit_VL6180X();
Adafruit_VL6180X lox2 = Adafruit_VL6180X();

/* --- Control Variables --- */
float Kp_x = 0.2, Kp_y = 20;
float Ki_x = 0.001, Ki_y = 0; // Significantly slower integral
float Kd_x = 0.7, Kd_y = 700;

float last_derivative_x = 0;
float last_derivative_y = 0;

float lastErrorX = 0;
float lastErrorY = 0;
float integralX = 0;
float integralY = 0;

const float updateRate = 0.01;
const float maxIntegral = 500.0;

// Set to true to output raw sensor data (s1,s2,t1,t2) for python/calibrate.py
// Set to false for normal PID debug output
bool CALIBRATION_MODE = false;

// ToF & IR Smoothing (for characterization)
float smoothS1 = 0, smoothS2 = 0;
float smoothT1 = 0, smoothT2 = 0;
const float tofAlpha = 0.05; // Lower for better plotting (more filtered)

struct Vector2D {
  float x;
  float y;
};

// RPWM attracts, LPWM repels, consider bool flipped
void setPower(MagnetPins m, float power) {
  int val = constrain((int)round(abs(power)), 0, 255);
  bool attract = (power >= 0);

  if (attract) {
    analogWrite(m.rpwm, val);
    analogWrite(m.lpwm, 0);
  } else {
    analogWrite(m.rpwm, 0);
    analogWrite(m.lpwm, val);
  }
}

// rotate coordinate plane by 45 degrees counterclockwise
// Axis definitions: X+(E), Y+(N), U+(NW), V+(SW)
Vector2D uvToXY(float u, float v) {
  float coeff = 0.7071f;
  float x = -u * coeff - v * coeff;
  float y = u * coeff - v * coeff;
  return {x, y};
}

void setup() {
  Serial.begin(115200);

  pinMode(front.rpwm, OUTPUT);
  pinMode(front.lpwm, OUTPUT);
  pinMode(back.rpwm, OUTPUT);
  pinMode(back.lpwm, OUTPUT);
  pinMode(left.rpwm, OUTPUT);
  pinMode(left.lpwm, OUTPUT);
  pinMode(right.rpwm, OUTPUT);
  pinMode(right.lpwm, OUTPUT);

  analogReadResolution(12);

  // High frequency PWM for smooth control
  analogWriteFrequency(front.rpwm, 18310);
  analogWriteFrequency(front.lpwm, 18310);
  analogWriteFrequency(back.rpwm, 18310);
  analogWriteFrequency(back.lpwm, 18310);
  analogWriteFrequency(left.rpwm, 18310);
  analogWriteFrequency(left.lpwm, 18310);
  analogWriteFrequency(right.rpwm, 18310);
  analogWriteFrequency(right.lpwm, 18310);

  Serial.println("Teensy 4.1 PD Stabilization Active");
  Serial.println("Monitoring S1 on Pin 14");

  // TOF Initialization
  pinMode(tof1.xshut, OUTPUT);
  pinMode(tof2.xshut, OUTPUT);

  // Keep them both in reset
  digitalWrite(tof1.xshut, LOW);
  digitalWrite(tof2.xshut, LOW);
  delay(100);

  // Initialize Wire (TOF2)
  Wire.setSDA(tof2.sda);
  Wire.setSCL(tof2.scl);
  Wire.begin();
  Wire.setClock(400000);
  delay(50);

  // Initialize Wire1 (TOF1)
  Wire1.setSDA(tof1.sda);
  Wire1.setSCL(tof1.scl);
  Wire1.begin();
  Wire1.setClock(400000);
  delay(50);

  // Wake up both
  digitalWrite(tof1.xshut, HIGH);
  digitalWrite(tof2.xshut, HIGH);
  delay(100);

  // Boot lox1 (TOF1 on Wire1)
  if (lox1.begin(&Wire1)) {
    Serial.println(F("SUCCESS: VL6180X #1 (Wire1) Booted"));
  } else {
    Serial.println(F("CRITICAL: VL6180X #1 (Wire1) Failed"));
  }

  // Boot lox2 (TOF2 on Wire)
  if (lox2.begin(&Wire)) {
    Serial.println(F("SUCCESS: VL6180X #2 (Wire) Booted"));
  } else {
    Serial.println(F("CRITICAL: VL6180X #2 (Wire) Failed"));
  }
}

int i = 0;

void loop() {
  // 0. Handle Serial Commands
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "CAL_ON") {
      CALIBRATION_MODE = true;
      Serial.println("MODE: CALIBRATION ACTIVE");
    } else if (cmd == "CAL_OFF") {
      CALIBRATION_MODE = false;
      Serial.println("MODE: STABILIZATION ACTIVE");
    }
  }

  int s1 = analogRead(SENSOR_1);
  int s2 = analogRead(SENSOR_2);

  // // Safety: If sensor is out of range, kill power
  // if (s1 < 10 || s1 > 3000 || s2 < 10 || s2 > 3000) {
  //   setPower(front, 0);
  //   setPower(back, 0);
  //   setPower(left, 0);
  //   setPower(right, 0);
  //   return;
  // }

  // 1. Calculate Virtual ToF heights from IR sensors with tilt correction
  float s1f = (float)s1;
  float s2f = (float)s2;

  // Derived formulas from virtual_tof_coeffs.h
  float t1_est = get_t1_est(s1f, s2f);
  float t2_est = get_t2_est(s1f, s2f);

  // 2. Calculate error relative to target centers.
  // In our coord system, positive error creates restoring force in that axis
  // direction. Distance > Target means too far -> move towards sensor (positive
  // U/V direction)
  float error_u = t1_est - T1_TARGET;
  float error_v = t2_est - T2_TARGET;

  // 3. Transform error to X/Y space before PID
  Vector2D errorXY = uvToXY(error_u, error_v);

  // 3. X-Axis PID
  integralX = constrain(integralX + (errorXY.x * updateRate), -maxIntegral,
                        maxIntegral);

  float derivX = errorXY.x - lastErrorX;
  float alpha_x = 0.1;
  float filtered_derivative_x =
      (alpha_x * derivX) + ((1.0 - alpha_x) * last_derivative_x);
  last_derivative_x = filtered_derivative_x;

  float signalX =
      errorXY.x * Kp_x + integralX * Ki_x + filtered_derivative_x * Kd_x;
  lastErrorX = errorXY.x;

  // 4. Y-Axis PID
  integralY = constrain(integralY + (errorXY.y * updateRate), -maxIntegral,
                        maxIntegral);

  float derivY = errorXY.y - lastErrorY;
  float alpha_y = 0.05;
  float filtered_derivative_y =
      (alpha_y * derivY) + ((1.0 - alpha_y) * last_derivative_y);
  last_derivative_y = filtered_derivative_y;

  float signalY =
      errorXY.y * Kp_y + integralY * Ki_y + filtered_derivative_y * Kd_y;
  lastErrorY = errorXY.y;

  // 5. Apply forces to cardinal magnets
  // setPower(right, -signalX);
  // setPower(left, signalX);
  setPower(front, -signalY);
  setPower(back, signalY);

  // Debug Output
  if (!CALIBRATION_MODE) {
    Serial.print("Ex:");
    Serial.print(errorXY.x);
    Serial.print(" Ey:");
    Serial.print(errorXY.y);
    Serial.print(" | Sx:");
    Serial.print(signalX);
    Serial.print(" Sy:");
    Serial.print(signalY);
    Serial.print(" | Ix:");
    Serial.print(integralX);
    Serial.print(" Iy:");
    Serial.println(integralY);
  }

  // Read TOF Sensors
  const float TOF1_OFFSET = -12;
  const float TOF2_OFFSET = 38.0;

  // TOF1 (Wire1)
  uint8_t range1 = lox1.readRange();
  uint8_t status1 = lox1.readRangeStatus();
  float rawT1 =
      (status1 == VL6180X_ERROR_NONE) ? (float)range1 + TOF1_OFFSET : smoothT1;
  smoothT1 = (smoothT1 * (1.0f - tofAlpha)) + (rawT1 * tofAlpha);

  // TOF2 (Wire)
  uint8_t range2 = lox2.readRange();
  uint8_t status2 = lox2.readRangeStatus();
  float rawT2 =
      (status2 == VL6180X_ERROR_NONE) ? (float)range2 + TOF2_OFFSET : smoothT2;
  smoothT2 = (smoothT2 * (1.0f - tofAlpha)) + (rawT2 * tofAlpha);

  // Sync IR smoothing with ToF lag for characterization
  smoothS1 = (smoothS1 * (1.0f - tofAlpha)) + ((float)s1 * tofAlpha);
  smoothS2 = (smoothS2 * (1.0f - tofAlpha)) + ((float)s2 * tofAlpha);

  // Stream data for calibration script (s1, s2, TrueT1, TrueT2)
  if (CALIBRATION_MODE) {
    Serial.print(s1);
    Serial.print(",");
    Serial.print(s2);
    Serial.print(",");
    Serial.print(smoothT1);
    Serial.print(",");
    Serial.println(smoothT2);
  }

  delayMicroseconds(500); // 2kHz loop
}
