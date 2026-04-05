#include <Arduino.h>
#include <math.h>
#include "fall_detection.h"

#define xPin A0
#define yPin A1
#define zPin A2
#define potential 9
#define confirmed 8


bool possibleFall = false;
bool fallDetected = false;
unsigned long fallStartTime = 0;
unsigned long stillnessStart = 0;
float magnitudeSum = 0;
int magnitudeCount = 0;
unsigned long avgWindowStart = 0;
float prevMagnitude = 0;

const float impact_threshold = 130.0; // subject to change
const float stillness_threshold = 5.0; // tolerates sensor noise and minor movement
const float cancel_threshold = 80.0;  // only considerable movement cancels the fall
const unsigned long confirm_time = 10000; 

float x0 = 0;
float y0 = 0;
float z0 = 0;

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

void fall(){
    accelData reading = readings();

    float dx = fabs(reading.x - x0);
    float dy = fabs(reading.y - y0);
    float dz = fabs(reading.z - z0);

    float magnitude = sqrt((dx*dx) + (dy*dy) + (dz*dz));
    Serial.println(magnitude);

    if(!possibleFall && magnitude > impact_threshold && reading.z < 400){
        possibleFall = true;
        fallStartTime = millis();
        stillnessStart = 0;
        magnitudeSum = 0;
        magnitudeCount = 0;
        avgWindowStart = 0;  // will be set after the 3s settling period
        prevMagnitude = magnitude;
        Serial.println("Possible fall detected... waiting 3 seconds before checking for stillness.");
    }
    if(possibleFall){
        digitalWrite(potential, HIGH);

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
                possibleFall = false;
                stillnessStart = 0;
                digitalWrite(potential, LOW);
            } else if (avgDelta < stillness_threshold) {
                // Avg change is small enough - sensor is still
                if (stillnessStart == 0) {
                    stillnessStart = millis();
                    Serial.println("Stillness detected, waiting to confirm...");
                } else if (millis() - stillnessStart >= confirm_time) {
                    // Sustained stillness confirmed - it's a fall
                    fallDetected = true;
                    possibleFall = false;
                    Serial.println("Fall Confirmed!");
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
            possibleFall = false;
            stillnessStart = 0;
            Serial.println("Timeout - not a fall. Resetting.");
            digitalWrite(potential, LOW);
        }
    }
    if (fallDetected){
        digitalWrite(confirmed, HIGH);
        Serial.println("ALERT: Send the emergency signal!");
        fallDetected = false;
        delay(10000);
        digitalWrite(confirmed, LOW);
        digitalWrite(potential, LOW);
    }
}