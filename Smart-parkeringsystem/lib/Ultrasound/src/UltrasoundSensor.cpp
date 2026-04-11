#include "UltrasoundSensor.h"

UltrasoundSensor::UltrasoundSensor(uint8_t trigPin, uint8_t echoPin, unsigned long timeoutUs)
    : _trigPin(trigPin), _echoPin(echoPin), _timeoutUs(timeoutUs) {}

void UltrasoundSensor::begin() const {
    pinMode(_trigPin, OUTPUT);
    pinMode(_echoPin, INPUT);
    digitalWrite(_trigPin, LOW);
}

float UltrasoundSensor::readDistanceCm(uint8_t samples) const {
    if (samples == 0) {
        samples = 1;
    }

    float sum = 0.0f;
    uint8_t validReadings = 0;

    for (uint8_t i = 0; i < samples; ++i) {
        const float distance = readSingleDistanceCm();
        // Ignore timeout or invalid readings in the average.
        if (distance != kInvalidDistanceCm) {
            sum += distance;
            ++validReadings;
        }
        delay(5);
    }

    if (validReadings == 0) {
        return kInvalidDistanceCm;
    }

    return sum / static_cast<float>(validReadings);
}

bool UltrasoundSensor::isCarPresent(float thresholdCm, uint8_t samples) const {
    const float distance = readDistanceCm(samples);
    return distance != kInvalidDistanceCm && distance <= thresholdCm;
}

float UltrasoundSensor::readSingleDistanceCm() const {
    // Send the short trigger pulse required by HC-SR04.
    digitalWrite(_trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(_trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(_trigPin, LOW);

    const unsigned long duration = pulseIn(_echoPin, HIGH, _timeoutUs);
    if (duration == 0) {
        return kInvalidDistanceCm;
    }

    // Convert echo time in microseconds to distance in centimeters.
    return (static_cast<float>(duration) * 0.0343f) / 2.0f;
}
