#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  int pins[] = {19, 21, 5, 18, 17, 16, 4, 2};
  for (int p : pins) {
    pinMode(p, OUTPUT);
    digitalWrite(p, LOW);
  }
  Serial.println("--- ALL MAGNETS DE-ENERGIZED (STOPPED) ---");
}

void loop() { delay(1000); }
