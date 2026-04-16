#include <Arduino.h>
#include <ESP32Servo.h>

#define SERVO_IN_PIN 13
#define SERVO_OUT_PIN 14

Servo gateIn;
Servo gateOut;

bool gateInOpen = false;
bool gateOutOpen = false;

void openEntryGate() {
    gateIn.write(90);
    gateInOpen = true;
    Serial.println("Entry gate opened");
}

void closeEntryGate() {
    gateIn.write(0);
    gateInOpen = false;
    Serial.println("Entry gate closed");
}

void openExitGate() {
    gateOut.write(90);
    gateOutOpen = true;
    Serial.println("Exit gate opened");
}

void closeExitGate() {
    gateOut.write(0);
    gateOutOpen = false;
    Serial.println("Exit gate closed");
}

void setup() {
    Serial.begin(115200);

    gateIn.attach(SERVO_IN_PIN);
    gateOut.attach(SERVO_OUT_PIN);

    closeEntryGate();
    closeExitGate();

    Serial.println("Parking servo system started");
}

void loop() {

    openEntryGate();
    delay(3000);
    closeEntryGate();
    delay(2000);

    openExitGate();
    delay(3000);
    closeExitGate();
    delay(2000);
}
