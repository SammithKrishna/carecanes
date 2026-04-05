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

const float impact_threshold = 130.0; // subject to change
const float stillness_threshold = 5; // also subject to change low movement after impact
const unsigned long confirm_time = 10000; 

const float x0 = 300;
const float y0 = 350;
const float z0 = 350;

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

    if(!possibleFall && magnitude > impact_threshold){
        possibleFall = true;
        fallStartTime = millis();
        Serial.println("Possible fall detected...");
    }
    if(possibleFall){
        digitalWrite(potential, HIGH);
        if(millis() - fallStartTime > confirm_time){
            accelData newReading = readings();
            float stillDx = fabs(newReading.x - reading.x);
            float stillDy = fabs(newReading.y - reading.y);
            float stillDz = fabs(newReading.z - reading.z);

            float stillness = sqrt((stillDx * stillDx) + (stillDy * stillDy) + (stillDz * stillDz)); 
            if(stillness < stillness_threshold){
                fallDetected = true;
                Serial.println("Fall Confirmed!");
            } else {
                Serial.println("Not a fall. Resetting.");
                digitalWrite(potential, LOW);
            }

            possibleFall = false;
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