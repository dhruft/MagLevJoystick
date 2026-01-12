#include <Arduino.h>

/*
 * ELECTROMAGNET SEQUENTIAL TEST
 * Cycles through each magnet one by one to verify wiring and polarity.
 */

struct MagnetPins {
  const char *name;
  int rpwm;
  int lpwm;
};

// Define all 4 magnets
MagnetPins magnets[] = {
    {"FRONT", 19, 21}, {"BACK", 5, 18}, {"LEFT", 17, 16}, {"RIGHT", 4, 2}};
const int NUM_MAGNETS = 4;

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

  Serial.println("\n=== Electromagnet Sequential Test ===");
  Serial.println(
      "Each magnet will pulse: RPWM (Positive) then LPWM (Negative)");

  for (int i = 0; i < NUM_MAGNETS; i++) {
    pinMode(magnets[i].rpwm, OUTPUT);
    pinMode(magnets[i].lpwm, OUTPUT);
    // Ensure everything is OFF initially
    setPower(magnets[i], 0);
  }
}

void loop() {
  for (int i = 0; i < NUM_MAGNETS; i++) {
    Serial.print("Testing Magnet: ");
    Serial.println(magnets[i].name);

    // Pulse 1: RPWM
    Serial.println("  -> RPWM (Positive Polarity) ON");
    setPower(magnets[i], 150);
    delay(1000);

    // Stop
    setPower(magnets[i], 0);
    delay(500);

    // Pulse 2: LPWM
    Serial.println("  -> LPWM (Negative Polarity) ON");
    setPower(magnets[i], -150);
    delay(1000);

    // Stop
    setPower(magnets[i], 0);
    delay(2000); // Wait before next magnet
  }

  Serial.println("--- Sequence Complete. Restarting... ---\n");
}
