#include <Arduino.h>

/* --- Calibration Constants --- */
const int S1_FRONT = 50;
const int S1_BACK = 1450;

/* --- Pin Definitions --- */
struct MagnetPins {
  int rpwm;
  int lpwm;
};

// Polarity Mappings (Determined from testing)
MagnetPins front = {19, 21}; // RPWM(+) = Attractive
MagnetPins back = {5, 18};   // LPWM(-) = Attractive

const int IR_SENSORS[] = {36, 39};

/* --- Haptic Joystick API --- */

/**
 * Returns normalized position: -100 (Front) to 100 (Back).
 */
int getPosition() {
  int s1 = analogRead(IR_SENSORS[0]);
  int pos = map(s1, S1_FRONT, S1_BACK, -100, 100);
  return constrain(pos, -100, 100);
}

/**
 * Applies a haptic force: -255 to 255.
 * Positive = Pull Back | Negative = Pull Front
 */
void applyForce(int force) {
  force = constrain(force, -255, 255);

  // Safety: If position is wildly out of range, kill power
  int s1 = analogRead(IR_SENSORS[0]);
  if (s1 < 10 || s1 > 2000) {
    force = 0;
  }

  // Set power to both magnets for balanced force
  // Front: Positive is Attractive (Pulls Back)
  // Back:  Positive is Repulsive (Pushes Front) -> Fixed: Both move magnet same
  // way

  // Let's use the polarity logic we verified:
  // controlSignal > 0: Front(Atr), Back(Rep) -> Moves BACK
  // controlSignal < 0: Front(Rep), Back(Atr) -> Moves FRONT

  int val = constrain(abs(force), 0, 255);
  if (force >= 0) {
    // Drive BACK
    analogWrite(front.rpwm, val);
    analogWrite(front.lpwm, 0);
    analogWrite(back.rpwm, val);
    analogWrite(back.lpwm, 0);
  } else {
    // Drive FRONT
    analogWrite(front.rpwm, 0);
    analogWrite(front.lpwm, val);
    analogWrite(back.rpwm, 0);
    analogWrite(back.lpwm, val);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(front.rpwm, OUTPUT);
  pinMode(front.lpwm, OUTPUT);
  pinMode(back.rpwm, OUTPUT);
  pinMode(back.lpwm, OUTPUT);
  analogReadResolution(12);
}

void loop() {
  // 1. Always broadcast current position
  Serial.print("POS:");
  Serial.println(getPosition());

  // 2. Handle serial commands from Python
  // Expected format: SETF:val\n
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    if (input.startsWith("SETF:")) {
      int force = input.substring(5).toInt();
      applyForce(force);
    }
  }

  delay(10); // 100Hz update rate
}
