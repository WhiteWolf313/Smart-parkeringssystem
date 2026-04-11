/*
 * SMART PARKING SYSTEM - PROFESSIONELL VERSION (NON-BLOCKING)
 * Hårdvara: ESP32 DevKitC V4 (38-pin), 2x RFID, 2x Servo, 2x LCD, 4x HC-SR04, 1x Buzzer
 * Databas: Redis via Wi-Fi (med LittleFS Offline-stöd)
 */

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <Redis.h>
#include <UltrasoundSensor.h>

// =======================================================
// 2. PIN-DEFINITIONER
// =======================================================
#define BUZZER_PIN 2

#define RST_PIN 27
#define SS_IN_PIN 5
#define SS_OUT_PIN 4

#define SERVO_IN_PIN 13
#define SERVO_OUT_PIN 14

#define TRIG_IN_BEFORE 32
#define ECHO_IN_BEFORE 34
#define TRIG_IN_AFTER 33
#define ECHO_IN_AFTER 35

#define TRIG_OUT_BEFORE 25
#define ECHO_OUT_BEFORE 36
#define TRIG_OUT_AFTER 26
#define ECHO_OUT_AFTER 39

namespace {
constexpr unsigned long kSerialBaudRate = 115200UL;
constexpr unsigned long kPrintIntervalMs = 300UL;
constexpr float kCarPresentThresholdCm = 15.0f;
constexpr uint8_t kSamplesPerReading = 3;

// One sensor object for each position around the two gates.
UltrasoundSensor inBeforeSensor(TRIG_IN_BEFORE, ECHO_IN_BEFORE);
UltrasoundSensor inAfterSensor(TRIG_IN_AFTER, ECHO_IN_AFTER);
UltrasoundSensor outBeforeSensor(TRIG_OUT_BEFORE, ECHO_OUT_BEFORE);
UltrasoundSensor outAfterSensor(TRIG_OUT_AFTER, ECHO_OUT_AFTER);

unsigned long lastPrintAtMs = 0;

void printSensorState(const char* name, const UltrasoundSensor& sensor) {
    const float distance = sensor.readDistanceCm(kSamplesPerReading);
    const bool present =
        distance != UltrasoundSensor::kInvalidDistanceCm && distance <= kCarPresentThresholdCm;

    // Print the latest distance and whether a car is currently detected.
    Serial.print(name);
    Serial.print(": ");

    if (distance == UltrasoundSensor::kInvalidDistanceCm) {
        Serial.print("no reading");
    } else {
        Serial.print(distance, 1);
        Serial.print(" cm");
    }

    Serial.print(" | car=");
    Serial.println(present ? "yes" : "no");
}
}  // namespace

void setup() {
    Serial.begin(kSerialBaudRate);

    // Initialize all ultrasound sensors used by the prototype.
    inBeforeSensor.begin();
    inAfterSensor.begin();
    outBeforeSensor.begin();
    outAfterSensor.begin();

    Serial.println();
    Serial.println("Ultrasound module started");
}

void loop() {
    const unsigned long now = millis();
    if (now - lastPrintAtMs < kPrintIntervalMs) {
        return;
    }

    lastPrintAtMs = now;

    // Print a quick live status view for the four sensors.
    printSensorState("IN_BEFORE", inBeforeSensor);
    printSensorState("IN_AFTER", inAfterSensor);
    printSensorState("OUT_BEFORE", outBeforeSensor);
    printSensorState("OUT_AFTER", outAfterSensor);
    Serial.println("---");
}

