#ifndef HEART_SENSOR_H
#define HEART_SENSOR_H

#include <Arduino.h>
#include <Wire.h>
#include "MAX30105.h"

class HeartRateSensor {
  private:
    enum { RATE_SIZE = 4 };
    MAX30105 sensor;
    long lastBeat = 0;
    float beatsPerMinute = 0;
    int beatAvg = 0;
    byte rates[RATE_SIZE] = {0};
    byte rateSpot = 0;

  public:
    HeartRateSensor();
    bool begin();
    void update();
    long getIR();
    float getBPM() const;
    int getAvgBPM() const;
};

#endif