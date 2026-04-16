#include <Arduino.h>

void servoInit();
void openEntryGate();
void closeEntryGate();
void openExitGate();
void closeExitGate();

void setup() {
    Serial.begin(115200);
    servoInit();
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
