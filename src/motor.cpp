//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Arduino Version Below~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// #include <Arduino.h>
// #include "motor.h"

// #define MOTOR_PIN 8

// void turnOnMotor() {
//     digitalWrite(MOTOR_PIN, HIGH);
// }

// void turnOffMotor() {
//     digitalWrite(MOTOR_PIN, LOW);
// }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ESP 32 Version Below~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <Arduino.h>
#include "motor.h"

#define MOTOR_PIN 18

void turnOnMotor() {
    digitalWrite(MOTOR_PIN, HIGH);
}

void turnOffMotor() {
    digitalWrite(MOTOR_PIN, LOW);
}