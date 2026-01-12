#include <Arduino.h>

/* --- Calibration Constants --- */
const int S1_FRONT = 50;
const int S1_BACK = 1450;
const int S1_CENTER = 750;

/* --- Pin Definitions --- */
struct MagnetPins {
  int rpwm;
  int lpwm;
};

// Front Magnet Polarity: Positive (RPWM) = ATTRACTIVE | Negative (LPWM) =
// REPULSIVE
MagnetPins front = {19, 21};
// Back Magnet Polarity: Negative (LPWM) = ATTRACTIVE | Positive (RPWM) =
// REPULSIVE
MagnetPins back = {5, 18};

const int IR_SENSORS[] = {36, 39};
const int NUM_SENSORS = 2;

/* --- Functions --- */

void setPower(MagnetPins m, int power) {
  int val = constrain(abs(power), 0, 255);
  if (power >= 0) {
    analogWrite(m.rpwm, val);
    analogWrite(m.lpwm, 0);
  } else {
    analogWrite(m.rpwm, 0);
    analogWrite(m.lpwm, val);
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(1);

  pinMode(front.rpwm, OUTPUT);
  pinMode(front.lpwm, OUTPUT);
  pinMode(back.rpwm, OUTPUT);
  pinMode(back.lpwm, OUTPUT);

  analogReadResolution(12);
  Serial.println("\n=== Directional Proportional Control ===");
}

void loop() {
  int s1 = analogRead(IR_SENSORS[0]);

  // 1. Calculate Positional Error (-100 to 100)
  // -100 = Front, 100 = Back, 0 = Center
  float position = map(s1, S1_FRONT, S1_BACK, -100, 100);

  // 2. Control Logic
  float Kp = 1.5; // Gain: start low to avoid oscillation
  int controlSignal = (int)(position * Kp); // Note: Simplified polarity logic
  controlSignal = constrain(controlSignal, -255, 255);

  // 3. Safety Check
  // If sensor is totally out of range (meaning magnet fell or moved too far)
  if (s1 < 10 || s1 > 2000) {
    setPower(front, 0);
    setPower(back, 0);
    Serial.println("SAFETY: Out of bounds. Magnets OFF.");
  } else {
    /*
     * Logic check:
     * If too Front (pos < 0), controlSignal is negative.
     * Front: neg = Repulsive (pushed back)
     * Back:  neg = Attractive (pulled back)
     * Result: Magnet moves towards center. ✅
     */
    setPower(front, controlSignal);
    setPower(back, controlSignal);
  }

  // 4. Output for Plotter
  Serial.print("Raw_S1:");
  Serial.print(s1);
  Serial.print(", Pos:");
  Serial.print(position);
  Serial.print(", Drive:");
  Serial.println(controlSignal);

  delay(10);
}
