#include <Arduino.h>
#include <math.h>
#include "fall_detection.h"

#define xPin A0
#define yPin A1
#define zPin A2
#define LED 8
#define MOTOR 9 


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

const float impact_threshold = 130.0; // subject to change
const float stillness_threshold = 7.0; // tolerates sensor noise and minor movement
const float cancel_threshold = 80.0;  // only considerable movement cancels the fall
const unsigned long confirm_time = 10000;
const int impact_samples_required = 3;               // debounce impact trigger
const unsigned long possible_fall_cooldown = 6000;   // ignore retriggers for 6s after reset
const unsigned long rearm_calm_time = 3000;          // calm time before allowing next potential trigger
const unsigned long led_blink_interval = 1000;        // slower LED blinking during possible fall

float x0 = 0;
float y0 = 0;
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
    y0 = sumY / 100.0;
    z0 = sumZ / 100.0;
    Serial.print("Calibrated baseline - X:"); Serial.print(x0);
    Serial.print(" Y:"); Serial.print(y0);
    Serial.print(" Z:"); Serial.println(z0);
}

accelData readings(){
    accelData data;
    data.x = analogRead(xPin);
    data.y = analogRead(yPin);
    data.z = analogRead(zPin);
    return data;
}

void spacialReadings(){
    accelData reading = readings();
    //2 holes facing up

    Serial.print("X value: ");
    Serial.println(reading.x);
    // x < 300

    Serial.print("Y value: ");
    Serial.println(reading.y);
    // 300 < y < 400

    Serial.print("Z value: ");
    Serial.println(reading.z);
    // 300 < z < 400

    Serial.println("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
    delay(250);
}


void buzzOnce(int duration = 1000) {
    digitalWrite(MOTOR, HIGH);
    delay(duration);
    digitalWrite(MOTOR, LOW);
}

void buzzMultiple(int times, int duration = 1000, int gap = 1000) {
    for (int i = 0; i < times; i++) {
        digitalWrite(MOTOR, HIGH);
        delay(duration);
        digitalWrite(MOTOR, LOW);
        delay(gap);
    }
}

void fall(){
    accelData reading = readings();

    float dx = fabs(reading.x - x0);
    float dy = fabs(reading.y - y0);
    float dz = fabs(reading.z - z0);

    float magnitude = sqrt((dx*dx) + (dy*dy) + (dz*dz));
    //Serial.println(magnitude);

    if(!possibleFall){
        digitalWrite(LED, LOW);  // Ensure LED is off when not in a fall state
        // Re-arm once cooldown has fully expired after a reset.
        if (!potentialTriggerArmed && millis() - lastPossibleFallEnd >= possible_fall_cooldown) {
            potentialTriggerArmed = true;
            calmStartTime = 0;
            impactConsecutiveCount = 0;
            Serial.println("--- System re-armed. Ready to detect falls. ---");
        }

        bool impactNow = (magnitude > impact_threshold && reading.z < 400);

        // Apply cooldown after resets to avoid immediate retrigger noise.
        if (millis() - lastPossibleFallEnd < possible_fall_cooldown) {
            impactConsecutiveCount = 0;
        } else if (impactNow && potentialTriggerArmed) {
            impactConsecutiveCount++;
        } else {
            impactConsecutiveCount = 0;
        }

        // Start possible fall only after consecutive impact samples.
        if (impactConsecutiveCount >= impact_samples_required) {
            possibleFall = true;
            fallStartTime = millis();
            stillnessStart = 0;
            magnitudeSum = 0;
            magnitudeCount = 0;
            avgWindowStart = 0;  // will be set after the 3s settling period
            prevMagnitude = magnitude;
            lastBlinkTime = millis();
            ledBlinkState = false;
            impactConsecutiveCount = 0;
            potentialTriggerArmed = false;
            calmStartTime = 0;
            Serial.println("Possible fall detected... waiting 3 seconds before checking for stillness.");
            buzzOnce();
        }
    }
    if(possibleFall){
          // Non-blocking LED blink so fall logic can continue running.
        if (millis() - lastBlinkTime >= led_blink_interval) {
            ledBlinkState = !ledBlinkState;
            digitalWrite(LED, ledBlinkState ? HIGH : LOW);
            lastBlinkTime = millis();
        }
        // Wait 3 seconds after impact before checking for stillness
        // (allows initial tumbling/sliding to settle)
        if (millis() - fallStartTime < 3000) {
            return;
        }

        // Start the avg window once the settling period has passed
        if (avgWindowStart == 0) {
            avgWindowStart = millis();
            prevMagnitude = magnitude;
            Serial.println("Settling period over. Now checking for stillness...");
        }

        // Accumulate change in magnitude for 5-second average
        float deltaMagnitude = fabs(magnitude - prevMagnitude);
        magnitudeSum += deltaMagnitude;
        magnitudeCount++;
        prevMagnitude = magnitude;

        // Every 5 seconds, evaluate average change in magnitude
        if (millis() - avgWindowStart >= 5000) {
            float avgDelta = magnitudeSum / magnitudeCount;
            Serial.print("5s avg magnitude change: ");
            Serial.println(avgDelta);
            magnitudeSum = 0;
            magnitudeCount = 0;
            avgWindowStart = millis();

            if (avgDelta >= cancel_threshold) {
                // Considerable sustained movement - false alarm
                Serial.println("Considerable movement detected - false alarm. Resetting.");
                resetPossibleFallState(false);
                Serial.println("Reset complete. Monitoring resumed.");
                return;
            } else if (avgDelta < stillness_threshold) {
                // Avg change is small enough - sensor is still
                if (stillnessStart == 0) {
                    stillnessStart = millis();
                    Serial.println("Stillness detected, waiting to confirm...");
                } else if (millis() - stillnessStart >= confirm_time) {
                    // Sustained stillness confirmed - it's a fall
                    fallDetected = true;
                    resetPossibleFallState(true);
                    Serial.println("Fall Confirmed!");
                    buzzMultiple(3, 300, 1000); 
                    return;
                }
            } else {
                // Minor movement - reset stillness timer
                if (stillnessStart != 0) {
                    stillnessStart = 0;
                    Serial.println("Minor movement, resetting stillness timer.");
                }
            }
        }

        // Safety timeout: if still possible fall after 30s with no confirmation, reset
        if(possibleFall && millis() - fallStartTime > 30000){
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
    if (fallDetected){
        digitalWrite(LED, HIGH);
    }
}