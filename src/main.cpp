#include "heart_sensor.h"
#include "fall_detection.h"
#include <Arduino.h>
#define LED 8
#define MOTOR 9 

// HeartRateSensor heart;
// bool fingerDetected = false;
// unsigned long lastFingerCheck = 0;
HeartRateSensor heart;
bool fingerDetected = false;
unsigned long lastFingerCheck = 0;
bool fallResponseActive = false;
bool heartCheckActive = false;
bool emergencyAlertSent = false;
bool fallLedLatched = false;
unsigned long fallResponseStartTime = 0;
unsigned long heartCheckStartTime = 0;
unsigned long emergencyAlertTime = 0;
int lastResponseCountdown = -1;
const unsigned long FALL_RESPONSE_WINDOW_MS = 15000;
const unsigned long HEART_CHECK_DURATION_MS = 60000;

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  pinMode(MOTOR, OUTPUT);
  digitalWrite(MOTOR, LOW);
  calibrateAccelerometer();
  Serial.println("Fall detection test ready.");
  Serial.println("Initializing...");

  if (!heart.begin()) {
    Serial.println("MAX30105 was not found. Please check wiring/power.");
    while (1);
  }

  Serial.println("Heart sensor standby. It will be used after a confirmed fall.");
}

void loop() {
  fall();

  if (fallDetected && !fallResponseActive) {
    fallDetected = false;
    fallResponseActive = true;
    heartCheckActive = false;
    emergencyAlertSent = false;
    fallLedLatched = true;
    fallResponseStartTime = millis();
    lastResponseCountdown = -1;
    Serial.println("Fall confirmed. Place your finger on the heart sensor within 15 seconds.");
  }

  if (fallResponseActive) {
    if (millis() - lastFingerCheck >= 500) {
      long irValue = heart.getIR();
      bool fingerNow = (irValue >= 50000);

      if (fingerNow && !fingerDetected) {
        Serial.println("Finger detected on heart sensor.");
      } else if (!fingerNow && fingerDetected) {
        Serial.println("Finger removed from heart sensor.");
      }

      fingerDetected = fingerNow;
      lastFingerCheck = millis();
    }

    if (!heartCheckActive) {
      unsigned long elapsed = millis() - fallResponseStartTime;
      if (!emergencyAlertSent && elapsed < FALL_RESPONSE_WINDOW_MS) {
        int remaining = (int)((FALL_RESPONSE_WINDOW_MS - elapsed + 999) / 1000);
        if (remaining != lastResponseCountdown) {
          lastResponseCountdown = remaining;
          Serial.print("Heart sensor window: ");
          Serial.print(remaining);
          Serial.println("s remaining");
        }
      }

      if (fingerDetected) {
        heartCheckActive = true;
        heartCheckStartTime = millis();
        heart.reset60sAvg();
        emergencyAlertSent = false;
        Serial.println("Starting 60-second heart-rate check...");
      } else if (!emergencyAlertSent && elapsed >= FALL_RESPONSE_WINDOW_MS) {
        Serial.println("ALERT: No finger detected in time. Send the emergency signal!");
        emergencyAlertSent = true;
        emergencyAlertTime = millis();
      } else if (emergencyAlertSent && millis() - emergencyAlertTime >= 10000) {
        fallResponseActive = false;
        heartCheckActive = false;
        emergencyAlertSent = false;
        fallLedLatched = false;
        fingerDetected = false;
        emergencyAlertTime = 0;
        digitalWrite(LED, LOW);
        Serial.println("Emergency alert complete. System reset. Ready to detect falls.");
      }
    } else {
      if (!fingerDetected) {
        heartCheckActive = false;
        heart.reset60sAvg();
        fallResponseStartTime = millis();
        lastResponseCountdown = -1;
        emergencyAlertSent = false;
        Serial.println("Finger removed. 15-second response window restarted.");
      } else {
        heart.update();

        if (millis() - heartCheckStartTime >= HEART_CHECK_DURATION_MS) {
          float avgBpm = heart.get60sAvg();
          Serial.print("Heart-rate check complete. 60s Average BPM: ");
          Serial.println(avgBpm);

          if (avgBpm > 0) {
            fallResponseActive = false;
            heartCheckActive = false;
            emergencyAlertSent = false;
            fallLedLatched = false;
            fingerDetected = false;
            digitalWrite(LED, LOW);
            Serial.println("Valid BPM captured. System reset. Ready to detect falls.");
          } else {
            heartCheckActive = false;
            heart.reset60sAvg();
            fallResponseStartTime = millis();
            lastResponseCountdown = -1;
            emergencyAlertSent = false;
            Serial.println("Invalid BPM reading. LED remains on. Restarting 15-second finger window.");
          }
        }
      }
    }
  }

  if (fallLedLatched) {
    digitalWrite(LED, HIGH);
  }

  delay(25);
}
