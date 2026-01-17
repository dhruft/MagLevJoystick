#include "Adafruit_VL6180X.h"
#include <Arduino.h>
#include <Wire.h>
#include <cstdlib>

/*
 * --- STABILIZED SWAY DEMO ---
 * This file levitates the magnet at the calibrated center
 * and gently sways it back and forth using a sine wave.
 *
 * TO USE:
 * 1. Backup your main.cpp
 * 2. Paste this content into main.cpp OR rename this file to main.cpp
 */

// Calibrated Target Heights (mm)
const float T1_BASE = 48.6;
const float T2_BASE = 49.0;

// Sway Parameters
const float SWAY_AMP = 40.0; // +/- 4mm movement
const float SWAY_FREQ = 0.4; // 0.4 Hz cycle (about 2.5 seconds per sway)

/* --- Pin Definitions --- */
struct MagnetPins {
  int rpwm;
  int lpwm;
};
struct TOFPins {
  int scl;
  int sda;
  int xshut;
};

MagnetPins front = {37, 36};
MagnetPins back = {23, 22};
MagnetPins left = {30, 29};
MagnetPins right = {15, 14};

const int SENSOR_1 = 24;
TOFPins tof1_p = {16, 17, 40};
const int SENSOR_2 = 25;
TOFPins tof2_p = {19, 18, 41};

Adafruit_VL6180X lox1 = Adafruit_VL6180X();
Adafruit_VL6180X lox2 = Adafruit_VL6180X();

/* --- PID Gains --- */
float Kp_x = 1.0, Kp_y = 50.0; // Slightly increased X for the demo
float Ki_x = 0.001, Ki_y = 0.5;
float Kd_x = 1.5, Kd_y = 20.0;

float lastErrorX = 0, lastErrorY = 0;
float integralX = 0, integralY = 0;
const float updateRate = 0.0001;
const float maxIntegral = 500.0;

struct Vector2D {
  float x;
  float y;
};

void setPower(MagnetPins m, float power) {
  int val = constrain((int)round(abs(power)), 0, 255);
  if (power >= 0) {
    analogWrite(m.rpwm, val);
    analogWrite(m.lpwm, 0);
  } else {
    analogWrite(m.rpwm, 0);
    analogWrite(m.lpwm, val);
  }
}

Vector2D uvToXY(float u, float v) {
  float coeff = 0.7071f;
  return {-u * coeff - v * coeff, u * coeff - v * coeff};
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
  analogWriteFrequency(front.rpwm, 18310);
  analogWriteFrequency(front.lpwm, 18310);
  analogWriteFrequency(back.rpwm, 18310);
  analogWriteFrequency(back.lpwm, 18310);
  analogWriteFrequency(left.rpwm, 18310);
  analogWriteFrequency(left.lpwm, 18310);
  analogWriteFrequency(right.rpwm, 18310);
  analogWriteFrequency(right.lpwm, 18310);

  pinMode(tof1_p.xshut, OUTPUT);
  pinMode(tof2_p.xshut, OUTPUT);
  digitalWrite(tof1_p.xshut, LOW);
  digitalWrite(tof2_p.xshut, LOW);
  delay(100);
  Wire.setSDA(tof2_p.sda);
  Wire.setSCL(tof2_p.scl);
  Wire.begin();
  Wire1.setSDA(tof1_p.sda);
  Wire1.setSCL(tof1_p.scl);
  Wire1.begin();
  digitalWrite(tof1_p.xshut, HIGH);
  digitalWrite(tof2_p.xshut, HIGH);
  delay(100);
  lox1.begin(&Wire1);
  lox2.begin(&Wire);
  Serial.println("Sway Demo Active");
}

void loop() {
  int s1 = analogRead(SENSOR_1);
  int s2 = analogRead(SENSOR_2);

  // Virtual ToF Height Mapping
  float s1f = (float)s1;
  float s2f = (float)s2;
  float s1_sq = s1f * s1f;
  float s2_sq = s2f * s2f;
  float t1_est = -4.705125f + (0.0058928339f * s1f) + (0.0403866126f * s2f) +
                 (0.0000026319f * s1_sq) + (-0.0000076040f * s1f * s2f) +
                 (-0.0000050890f * s2_sq);
  float t2_est = 327.215778f + (-0.1293694981f * s1f) + (-0.1502912265f * s2f) +
                 (0.0000136065f * s1_sq) + (0.0000376023f * s1f * s2f) +
                 (0.0000182367f * s2_sq);

  // Generate Sway Sine Wave
  float sway = sin(millis() * 0.001f * 2.0f * PI * SWAY_FREQ) * SWAY_AMP;

  // Apply sway to targets
  float error_u = t1_est - (T1_BASE + sway);
  float error_v =
      t2_est -
      (T2_BASE -
       sway); // Subtracting from one and adding to other sways in a line

  Vector2D errXY = uvToXY(error_u, error_v);

  // X PID
  integralX =
      constrain(integralX + (errXY.x * updateRate), -maxIntegral, maxIntegral);
  float sigX =
      errXY.x * Kp_x + integralX * Ki_x + (errXY.x - lastErrorX) * Kd_x;
  lastErrorX = errXY.x;

  // Y PID
  integralY =
      constrain(integralY + (errXY.y * updateRate), -maxIntegral, maxIntegral);
  float sigY =
      errXY.y * Kp_y + integralY * Ki_y + (errXY.y - lastErrorY) * Kd_y;
  lastErrorY = errXY.y;

  // Drive Magnets
  setPower(front, -sigY);
  setPower(back, sigY);
  setPower(left, sigX);
  setPower(right, -sigX);

  // Debug
  if (millis() % 20 == 0) {
    Serial.print("Sway:");
    Serial.print(sway);
    Serial.print(" T1:");
    Serial.print(t1_est);
    Serial.print(" T2:");
    Serial.println(t2_est);
  }

  delayMicroseconds(500);
}
