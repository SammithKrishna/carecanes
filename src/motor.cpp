#include <Arduino.h>
#include "motor.h"

#define MOTOR_PIN 8

void turnOnMotorOn() {
    digitalWrite(MOTOR_PIN, HIGH);
}

void turnOffMotor() {
    digitalWrite(MOTOR_PIN, LOW);
}

