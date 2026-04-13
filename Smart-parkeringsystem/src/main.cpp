#include <Arduino.h>
#include <SPI.h>
#include <RfidReader.h>

// ── Pin definitions (NodeMCU-32S / ESP32) ────────────────────────────────────
#define SPI_SCK   18   // CLK
#define SPI_MISO  19   // MISO
#define SPI_MOSI  23   // MOSI
#define SS_IN_PIN  5   // SDA/SS
#define RST_IN_PIN 27   // RST

// OUT reader (exit gate)
#define SS_OUT_PIN  4
#define RST_OUT_PIN 22

RfidReader readerIn ("IN",  SS_IN_PIN,  RST_IN_PIN);
RfidReader readerOut("OUT", SS_OUT_PIN, RST_OUT_PIN);

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n[BOOT] Smart Parking System");

    // SPI bus — SS pin in begin() is just a default, each reader drives its own
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SS_IN_PIN);

    readerIn.begin();
    readerOut.begin();
}

void loop() {
    String uid;

    if (readerIn.poll(uid)) {
        Serial.printf("[CARD IN]  UID=%s\n", uid.c_str());
        // TODO: Validator::check(uid) -> open entry gate
    }

    if (readerOut.poll(uid)) {
        Serial.printf("[CARD OUT] UID=%s\n", uid.c_str());
        // TODO: Validator::check(uid) -> open exit gate
    }
}