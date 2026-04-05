#include "heart_sensor.h"

HeartRateSensor heart;

void setup() {
  Serial.begin(115200);
  Serial.println("Initializing...");

  if (!heart.begin()) {
    Serial.println("MAX30105 was not found. Please check wiring/power.");
    while (1);
  }

  Serial.println("Place your index finger on the sensor with steady pressure.");
}

void loop() {
  heart.update();
  delay(25);
}
