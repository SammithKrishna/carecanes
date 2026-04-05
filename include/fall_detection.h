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

#endif