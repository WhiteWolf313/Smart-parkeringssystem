#pragma once

#include <Arduino.h>

class UltrasoundSensor {
public:
    static constexpr float kInvalidDistanceCm = -1.0f;

    UltrasoundSensor(uint8_t trigPin, uint8_t echoPin, unsigned long timeoutUs = 30000UL);

    // Configure the trigger and echo pins.
    void begin() const;
    // Return the average distance from several valid samples.
    float readDistanceCm(uint8_t samples = 3) const;
    // Detect if a car is close enough to be considered present.
    bool isCarPresent(float thresholdCm = 15.0f, uint8_t samples = 3) const;

private:
    // Read one raw distance sample from the sensor.
    float readSingleDistanceCm() const;

    uint8_t _trigPin;
    uint8_t _echoPin;
    unsigned long _timeoutUs;
};
