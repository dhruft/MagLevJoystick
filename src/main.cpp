#include <Arduino.h>

/* --- Hardware Calibration --- */
const int S1_FRONT = 50;
const int S1_BACK = 1450;
const int S_RANGE = 700;

/* --- Pin Definitions --- */
struct MagnetPins {
  int rpwm;
  int lpwm;
};
MagnetPins front = {19, 21};
MagnetPins right = {4, 2};
MagnetPins back = {5, 18};
MagnetPins left = {17, 16};
const int IR_SENSORS[] = {36, 39, 34, 35};

/* --- Global State (Shared between cores) --- */
volatile int currentX = 0;
volatile int currentY = 0;
volatile int targetFx = 0;
volatile int targetFy = 0;

/* --- Helper: Set Magnet Power --- */
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

/* --- Core 1: High-Speed Control Loop --- */
void ControlTask(void *pvParameters) {
  analogReadResolution(12);
  // Set PWM frequency high for silent/smooth operation
  // Note: analogWrite frequency on ESP32 is fixed but enough for this

  while (true) {
    // 1. Read Sensors
    int s1 = analogRead(IR_SENSORS[0]);
    int s2 = analogRead(IR_SENSORS[1]);
    int s3 = analogRead(IR_SENSORS[2]);
    int s4 = analogRead(IR_SENSORS[3]);

    // 2. Calculate Position Vectors
    int rawY = ((s1 + s4) - (s2 + s3)) / 2;
    int rawX = ((s3 + s4) - (s1 + s2)) / 2;

    currentX = constrain(map(rawX, -S_RANGE, S_RANGE, -100, 100), -100, 100);
    currentY = constrain(map(rawY, -S_RANGE, S_RANGE, -100, 100), -100, 100);

    // 3. Safety Check: If out of range, zero the force
    if (s1 < 10 || s1 > 2500) {
      setMagnetPower(front, 0);
      setMagnetPower(back, 0);
      setMagnetPower(right, 0);
      setMagnetPower(left, 0);
    } else {
      // 4. Apply Force Vectors (Directly from targetFx/targetFy)
      setMagnetPower(front, targetFy);
      setMagnetPower(back, targetFy);
      setMagnetPower(right, targetFx);
      setMagnetPower(left, targetFx);
    }

    // Run at ~2kHz
    vTaskDelay(pdMS_TO_TICKS(0)); // Rapid as possible, yielding minimally
  }
}

/* --- Core 0: High-Speed Serial Communication --- */
void CommTask(void *pvParameters) {
  Serial.begin(460800); // High baud rate
  while (true) {
    // 1. Broadcast Position
    Serial.print("P:");
    Serial.print(currentX);
    Serial.print(",");
    Serial.println(currentY);

    // 2. Parse Incoming Forces: F:fx,fy
    if (Serial.available()) {
      String input = Serial.readStringUntil('\n');
      if (input.startsWith("F:")) {
        int commaIndex = input.indexOf(',');
        if (commaIndex != -1) {
          targetFx = input.substring(2, commaIndex).toInt();
          targetFy = input.substring(commaIndex + 1).toInt();
        }
      }
    }
    // Comm rate ~200Hz is plenty for games, saves CPU for Control Task
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

void setup() {
  pinMode(front.rpwm, OUTPUT);
  pinMode(front.lpwm, OUTPUT);
  pinMode(back.rpwm, OUTPUT);
  pinMode(back.lpwm, OUTPUT);
  pinMode(left.rpwm, OUTPUT);
  pinMode(left.lpwm, OUTPUT);
  pinMode(right.rpwm, OUTPUT);
  pinMode(right.lpwm, OUTPUT);

  // Create Tasks
  xTaskCreatePinnedToCore(ControlTask, "Control", 4096, NULL, 5, NULL, 1);
  xTaskCreatePinnedToCore(CommTask, "Comm", 4096, NULL, 1, NULL, 0);
}

void loop() {
  // Main loop remains empty as we use FreeRTOS Tasks
  vTaskDelete(NULL);
}
