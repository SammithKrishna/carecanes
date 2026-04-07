//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Arduino Version Below~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// #include "heart_sensor.h"
// #include "fall_detection.h"
// #include <Arduino.h>
// #define LED 8
// #define MOTOR 9 

// // HeartRateSensor heart;
// // bool fingerDetected = false;
// // unsigned long lastFingerCheck = 0;
// HeartRateSensor heart;
// bool fingerDetected = false;
// unsigned long lastFingerCheck = 0;
// bool fallResponseActive = false;
// bool heartCheckActive = false;
// bool emergencyAlertSent = false;
// bool fallLedLatched = false;
// unsigned long fallResponseStartTime = 0;
// unsigned long heartCheckStartTime = 0;
// unsigned long emergencyAlertTime = 0;
// int lastResponseCountdown = -1;
// const unsigned long FALL_RESPONSE_WINDOW_MS = 15000;
// const unsigned long HEART_CHECK_DURATION_MS = 60000;

// void setup() {
//   Serial.begin(115200);
//   pinMode(LED, OUTPUT);
//   pinMode(MOTOR, OUTPUT);
//   digitalWrite(MOTOR, LOW);
//   calibrateAccelerometer();
//   Serial.println("Fall detection test ready.");
//   Serial.println("Initializing...");

//   if (!heart.begin()) {
//     Serial.println("MAX30105 was not found. Please check wiring/power.");
//     while (1);
//   }

//   Serial.println("Heart sensor standby. It will be used after a confirmed fall.");
// }

// void loop() {
//   fall();

//   if (fallDetected && !fallResponseActive) {
//     fallDetected = false;
//     fallResponseActive = true;
//     heartCheckActive = false;
//     emergencyAlertSent = false;
//     fallLedLatched = true;
//     fallResponseStartTime = millis();
//     lastResponseCountdown = -1;
//     Serial.println("Fall confirmed. Place your finger on the heart sensor within 15 seconds.");
//   }

//   if (fallResponseActive) {
//     if (millis() - lastFingerCheck >= 500) {
//       long irValue = heart.getIR();
//       bool fingerNow = (irValue >= 50000);

//       if (fingerNow && !fingerDetected) {
//         Serial.println("Finger detected on heart sensor.");
//       } else if (!fingerNow && fingerDetected) {
//         Serial.println("Finger removed from heart sensor.");
//       }

//       fingerDetected = fingerNow;
//       lastFingerCheck = millis();
//     }

//     if (!heartCheckActive) {
//       unsigned long elapsed = millis() - fallResponseStartTime;
//       if (!emergencyAlertSent && elapsed < FALL_RESPONSE_WINDOW_MS) {
//         int remaining = (int)((FALL_RESPONSE_WINDOW_MS - elapsed + 999) / 1000);
//         if (remaining != lastResponseCountdown) {
//           lastResponseCountdown = remaining;
//           Serial.print("Heart sensor window: ");
//           Serial.print(remaining);
//           Serial.println("s remaining");
//         }
//       }

//       if (fingerDetected) {
//         heartCheckActive = true;
//         heartCheckStartTime = millis();
//         heart.reset60sAvg();
//         emergencyAlertSent = false;
//         Serial.println("Starting 60-second heart-rate check...");
//       } else if (!emergencyAlertSent && elapsed >= FALL_RESPONSE_WINDOW_MS) {
//         Serial.println("ALERT: No finger detected in time. Send the emergency signal!");
//         emergencyAlertSent = true;
//         emergencyAlertTime = millis();
//       } else if (emergencyAlertSent && millis() - emergencyAlertTime >= 10000) {
//         fallResponseActive = false;
//         heartCheckActive = false;
//         emergencyAlertSent = false;
//         fallLedLatched = false;
//         fingerDetected = false;
//         emergencyAlertTime = 0;
//         digitalWrite(LED, LOW);
//         Serial.println("Emergency alert complete. System reset. Ready to detect falls.");
//       }
//     } else {
//       if (!fingerDetected) {
//         heartCheckActive = false;
//         heart.reset60sAvg();
//         fallResponseStartTime = millis();
//         lastResponseCountdown = -1;
//         emergencyAlertSent = false;
//         Serial.println("Finger removed. 15-second response window restarted.");
//       } else {
//         heart.update();

//         if (millis() - heartCheckStartTime >= HEART_CHECK_DURATION_MS) {
//           float avgBpm = heart.get60sAvg();
//           Serial.print("Heart-rate check complete. 60s Average BPM: ");
//           Serial.println(avgBpm);

//           if (avgBpm > 0) {
//             fallResponseActive = false;
//             heartCheckActive = false;
//             emergencyAlertSent = false;
//             fallLedLatched = false;
//             fingerDetected = false;
//             digitalWrite(LED, LOW);
//             Serial.println("Valid BPM captured. System reset. Ready to detect falls.");
//           } else {
//             heartCheckActive = false;
//             heart.reset60sAvg();
//             fallResponseStartTime = millis();
//             lastResponseCountdown = -1;
//             emergencyAlertSent = false;
//             Serial.println("Invalid BPM reading. LED remains on. Restarting 15-second finger window.");
//           }
//         }
//       }
//     }
//   }

//   if (fallLedLatched) {
//     digitalWrite(LED, HIGH);
//   }

//   delay(25);
// }


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ESP 32 Version Below~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <Arduino.h>
#include <math.h>
#include "fall_detection.h"
#include "heart_sensor.h"

#define xPin 34
#define yPin 35
#define zPin 32
#define LED 2
#define MOTOR 18

bool possibleFall = false;
bool fallDetected = false;
unsigned long fallStartTime = 0;
unsigned long stillnessStart = 0;
float magnitudeSum = 0;
int magnitudeCount = 0;
unsigned long avgWindowStart = 0;
float prevMagnitude = 0;
int impactConsecutiveCount = 0;
unsigned long lastPossibleFallEnd = 0;
bool potentialTriggerArmed = true;
unsigned long calmStartTime = 0;
unsigned long lastBlinkTime = 0;
bool ledBlinkState = false;

const float impact_threshold = 130.0;
const float stillness_threshold = 7.0;
const float cancel_threshold = 80.0;
const unsigned long confirm_time = 10000;
const int impact_samples_required = 3;
const unsigned long possible_fall_cooldown = 6000;
const unsigned long rearm_calm_time = 3000;
const unsigned long led_blink_interval = 1000;

float x0 = 0;
float accelY0 = 0;
float z0 = 0;

static void resetPossibleFallState(bool rearmTrigger) {
    possibleFall = false;
    stillnessStart = 0;
    magnitudeSum = 0;
    magnitudeCount = 0;
    avgWindowStart = 0;
    prevMagnitude = 0;
    impactConsecutiveCount = 0;
    calmStartTime = 0;
    fallStartTime = 0;
    ledBlinkState = false;
    digitalWrite(LED, LOW);
    lastPossibleFallEnd = millis();
    potentialTriggerArmed = rearmTrigger;
}

void calibrateAccelerometer() {
    Serial.println("Calibrating accelerometer... keep sensor still.");
    long sumX = 0, sumY = 0, sumZ = 0;
    for (int i = 0; i < 100; i++) {
        sumX += analogRead(xPin);
        sumY += analogRead(yPin);
        sumZ += analogRead(zPin);
        delay(10);
    }
    x0 = sumX / 100.0;
    accelY0 = sumY / 100.0;
    z0 = sumZ / 100.0;
    Serial.print("Calibrated baseline - X:"); Serial.print(x0);
    Serial.print(" Y:"); Serial.print(accelY0);
    Serial.print(" Z:"); Serial.println(z0);
}

accelData readings() {
    accelData data;
    data.x = analogRead(xPin);
    data.y = analogRead(yPin);
    data.z = analogRead(zPin);
    return data;
}

void spacialReadings() {
    accelData reading = readings();

    Serial.print("X value: ");
    Serial.println(reading.x);

    Serial.print("Y value: ");
    Serial.println(reading.y);

    Serial.print("Z value: ");
    Serial.println(reading.z);

    Serial.println("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
    delay(250);
}

void buzzOnce(int duration) {
    digitalWrite(MOTOR, HIGH);
    delay(duration);
    digitalWrite(MOTOR, LOW);
}

void buzzMultiple(int times, int duration, int gap) {
    for (int i = 0; i < times; i++) {
        digitalWrite(MOTOR, HIGH);
        delay(duration);
        digitalWrite(MOTOR, LOW);
        delay(gap);
    }
}

void fall() {
    accelData reading = readings();

    float dx = fabs(reading.x - x0);
    float dy = fabs(reading.y - accelY0);
    float dz = fabs(reading.z - z0);

    float magnitude = sqrt((dx * dx) + (dy * dy) + (dz * dz));

    if (!possibleFall) {
        digitalWrite(LED, LOW);

        if (!potentialTriggerArmed && millis() - lastPossibleFallEnd >= possible_fall_cooldown) {
            potentialTriggerArmed = true;
            calmStartTime = 0;
            impactConsecutiveCount = 0;
            Serial.println("--- System re-armed. Ready to detect falls. ---");
        }

        bool impactNow = (magnitude > impact_threshold && reading.z < 400);

        if (millis() - lastPossibleFallEnd < possible_fall_cooldown) {
            impactConsecutiveCount = 0;
        } else if (impactNow && potentialTriggerArmed) {
            impactConsecutiveCount++;
        } else {
            impactConsecutiveCount = 0;
        }

        if (impactConsecutiveCount >= impact_samples_required) {
            possibleFall = true;
            fallStartTime = millis();
            stillnessStart = 0;
            magnitudeSum = 0;
            magnitudeCount = 0;
            avgWindowStart = 0;
            prevMagnitude = magnitude;
            lastBlinkTime = millis();
            ledBlinkState = false;
            impactConsecutiveCount = 0;
            potentialTriggerArmed = false;
            calmStartTime = 0;
            Serial.println("Possible fall detected... waiting 3 seconds before checking for stillness.");
            //buzzOnce(1000);
            buzzMultiple(3, 750, 500);
        }
    }

    if (possibleFall) {
        if (millis() - lastBlinkTime >= led_blink_interval) {
            ledBlinkState = !ledBlinkState;
            digitalWrite(LED, ledBlinkState ? HIGH : LOW);
            lastBlinkTime = millis();
        }

        if (millis() - fallStartTime < 3000) {
            return;
        }

        if (avgWindowStart == 0) {
            avgWindowStart = millis();
            prevMagnitude = magnitude;
            Serial.println("Settling period over. Now checking for stillness...");
        }

        float deltaMagnitude = fabs(magnitude - prevMagnitude);
        magnitudeSum += deltaMagnitude;
        magnitudeCount++;
        prevMagnitude = magnitude;

        if (millis() - avgWindowStart >= 5000) {
            float avgDelta = magnitudeSum / magnitudeCount;
            Serial.print("5s avg magnitude change: ");
            Serial.println(avgDelta);
            magnitudeSum = 0;
            magnitudeCount = 0;
            avgWindowStart = millis();

            if (avgDelta >= cancel_threshold) {
                Serial.println("Considerable movement detected - false alarm. Resetting.");
                resetPossibleFallState(false);
                Serial.println("Reset complete. Monitoring resumed.");
                return;
            } else if (avgDelta < stillness_threshold) {
                if (stillnessStart == 0) {
                    stillnessStart = millis();
                    Serial.println("Stillness detected, waiting to confirm...");
                } else if (millis() - stillnessStart >= confirm_time) {
                    fallDetected = true;
                    resetPossibleFallState(true);
                    Serial.println("Fall Confirmed!");
                    buzzMultiple(5, 500, 1000);
                    return;
                }
            } else {
                if (stillnessStart != 0) {
                    stillnessStart = 0;
                    Serial.println("Minor movement, resetting stillness timer.");
                }
            }
        }

        if (possibleFall && millis() - fallStartTime > 30000) {
            Serial.print("Timeout - not a fall (magnitude: ");
            Serial.print(magnitude);
            Serial.println("). Resetting...");
            resetPossibleFallState(false);
            Serial.print(">>> RESET COMPLETE. System will re-arm in ");
            Serial.print(possible_fall_cooldown / 1000);
            Serial.println("s. <<<");
            return;
        }
    }

    if (fallDetected) {
        digitalWrite(LED, HIGH);
    }
}

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
  analogReadResolution(10); // Match Arduino Uno 10-bit ADC (0-1023) so all thresholds stay valid
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