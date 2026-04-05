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

void HeartRateSensor::update() {
    long irValue = sensor.getIR();

    if (checkForBeat(irValue) == true) {
        long delta = millis() - lastBeat;
        lastBeat = millis();

        beatsPerMinute = 60.0f / (delta / 1000.0f);

        if (beatsPerMinute < 255 && beatsPerMinute > 20) {
            rates[rateSpot++] = (byte)beatsPerMinute;
            rateSpot %= RATE_SIZE;

            beatAvg = 0;
            for (byte x = 0; x < RATE_SIZE; x++) {
                beatAvg += rates[x];
            }
            beatAvg /= RATE_SIZE;
        }
    }

    Serial.print("IR=");
    Serial.print(irValue);
    Serial.print(", BPM=");
    Serial.print(beatsPerMinute);
    Serial.print(", Avg BPM=");
    Serial.print(beatAvg);

    if (irValue < 50000) {
        Serial.print(" No finger?");
    }

    Serial.println();
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