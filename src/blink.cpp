#include <Arduino.h>
#include "blink.h"

#define LED 8


void blink(int input){
  if (input == 1){
    digitalWrite(LED, HIGH);
    Serial.println("LED On");
  } else if (input == 0) {
    digitalWrite(LED, LOW);
    Serial.println("LED Off");
  }
}

bool isValid(int input){
  return (input == 0 || input == 1);
}

void turnOnLED(){
    digitalWrite(LED, HIGH);
}

void turnOffLED(){
    digitalWrite(LED, LOW);
}
