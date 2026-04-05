#include "heart_sensor.h"
#include "heartRate.h"

HeartRateSensor::HeartRateSensor() {}

bool HeartRateSensor::begin() {
    Wire.begin();

    if (!sensor.begin(Wire, I2C_SPEED_FAST)) {
        return false;
    }

    sensor.setup();
    sensor.setPulseAmplitudeRed(0x0A);  // Turn Red LED to low to indicate sensor is running
    sensor.setPulseAmplitudeGreen(0);

    return true;
}

void HeartRateSensor::reset60sAvg() {
    bpmSum = 0;
    bpmCount = 0;
    avgStartTime = millis();
}

float HeartRateSensor::get60sAvg() {
    if (bpmCount == 0) return 0;
    return bpmSum / bpmCount;
}

void HeartRateSensor::update() {
    long irValue = sensor.getIR();

    if (avgStartTime == 0) avgStartTime = millis();

    if (checkForBeat(irValue) == true) {
        long delta = millis() - lastBeat;
        lastBeat = millis();

        beatsPerMinute = 60.0f / (delta / 1000.0f);

        // Only accept BPM in 30-220 range for all calculations
        if (beatsPerMinute <= 220 && beatsPerMinute >= 30) {
            rates[rateSpot++] = (byte)beatsPerMinute;
            rateSpot %= RATE_SIZE;

            beatAvg = 0;
            for (byte x = 0; x < RATE_SIZE; x++) {
                beatAvg += rates[x];
            }
            beatAvg /= RATE_SIZE;

            // 60s average accumulation
            if (millis() - avgStartTime <= 60000) {
                bpmSum += beatsPerMinute;
                bpmCount++;
            }
        }
    }

    // Only print 60s average every 60 seconds
    if (millis() - lastPrintTime >= 60000) {
        Serial.print("60s Average BPM: ");
        Serial.println(get60sAvg());
        lastPrintTime = millis();
    }
}

long HeartRateSensor::getIR() {
    return sensor.getIR();
}

float HeartRateSensor::getBPM() const {
    return beatsPerMinute;
}

int HeartRateSensor::getAvgBPM() const {
    return beatAvg;
}