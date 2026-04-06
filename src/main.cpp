#include "heart_sensor.h"
#include "fall_detection.h"
#include <Arduino.h>
#define LED 8
#define MOTOR 9 

// HeartRateSensor heart;
// bool fingerDetected = false;
// unsigned long lastFingerCheck = 0;

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  pinMode(MOTOR, OUTPUT);
  digitalWrite(MOTOR, LOW);

  calibrateAccelerometer();
  Serial.println("Fall detection test ready.");
 // Serial.println("Initializing...");

  // if (!heart.begin()) {
  //   Serial.println("MAX30105 was not found. Please check wiring/power.");
  //   while (1);
  // }

  // Serial.println("Place your index finger on the sensor with steady pressure.");
}

void loop() {
  // long irValue = heart.getIR();

  // // Check for finger every 500ms
  // if (millis() - lastFingerCheck >= 500) {
  //   if (irValue < 50000) {
  //     if (fingerDetected) {
  //       // Finger was removed during measurement
  //       Serial.println("BPM CHECKER CANCELED - PLEASE TRY AGAIN");
  //       fingerDetected = false;
  //     }
  //   } else {
  //     if (!fingerDetected) {
  //       Serial.println("Finger detected. Starting BPM measurement...");
  //       fingerDetected = true;
  //       heart.reset60sAvg();  // Start fresh 60s measurement
  //     }
  //   }
  //   lastFingerCheck = millis();
  // }

  // // Only update heart sensor if finger is detected
  // if (fingerDetected) {
  //   heart.update();
  // }
  // Fall detection test
  // spacialReadings();
  fall();
  delay(25);
}
