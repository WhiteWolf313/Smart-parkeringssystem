#include <Arduino.h>
#include <ESP32Servo.h>

Servo gateIn;
Servo gateOut;

#define SERVO_IN_PIN 13
#define SERVO_OUT_PIN 14

void servoInit() {
    gateIn.attach(SERVO_IN_PIN);
    gateOut.attach(SERVO_OUT_PIN);

    gateIn.write(0);
    gateOut.write(0);
}

void openEntryGate() {
    gateIn.write(90);
    Serial.println("Entry gate opened");
}

void closeEntryGate() {
    gateIn.write(0);
    Serial.println("Entry gate closed");
}

void openExitGate() {
    gateOut.write(90);
    Serial.println("Exit gate opened");
}

void closeExitGate() {
    gateOut.write(0);
    Serial.println("Exit gate closed");
}
