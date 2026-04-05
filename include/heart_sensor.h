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

    // For 60-second average
    float bpmSum = 0;
    int bpmCount = 0;
    unsigned long avgStartTime = 0;
    unsigned long lastPrintTime = 0;

  public:
    HeartRateSensor();
    bool begin();
    void update();
    long getIR();
    float getBPM() const;
    int getAvgBPM() const;
    void reset60sAvg();
    float get60sAvg();
};

#endif