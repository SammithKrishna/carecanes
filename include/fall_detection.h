#ifndef FALL_DETECTION_H
#define FALL_DECTECTION_H

struct accelData{
    float x;
    float y;
    float z;
};

accelData readings();
void spacialReadings();
void fall();
void calibrateAccelerometer();
void buzzOnce(int duration);
void buzzMultiple(int times, int duration, int gap);
extern bool fallDetected;

#endif