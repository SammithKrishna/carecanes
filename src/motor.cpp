#include <Arduino.h>
#include "motor.h"

#define MOTOR_PIN 9

void turnOnMotor() {
    digitalWrite(MOTOR_PIN, HIGH);
}

void turnOffMotor() {
    digitalWrite(MOTOR_PIN, LOW);
}

