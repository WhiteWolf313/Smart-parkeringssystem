#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>

// ── Pin definitions ──────────────────────────────────────────────────────────
#define SPI_SCK     18
#define SPI_MISO    19
#define SPI_MOSI    23

// IN reader (entry gate)
#define SS_IN_PIN    5
#define RST_IN_PIN  27

// OUT reader (exit gate)
#define SS_OUT_PIN   4
#define RST_OUT_PIN 22

// ── Debounce setting ─────────────────────────────────────────────────────────
#define DEBOUNCE_MS 1500

// ── Reader state ─────────────────────────────────────────────────────────────
MFRC522 readerIn(SS_IN_PIN,  RST_IN_PIN);
MFRC522 readerOut(SS_OUT_PIN, RST_OUT_PIN);

String   lastUidIn,  lastUidOut;
uint32_t lastSeenIn = 0, lastSeenOut = 0;

// ── Helper: UID bytes → hex string ──────────────────────────────────────────
String uidToHex(MFRC522::Uid &uid) {
    String s;
    s.reserve(uid.size * 2);
    for (byte i = 0; i < uid.size; i++) {
        if (uid.uidByte[i] < 0x10) s += '0';
        s += String(uid.uidByte[i], HEX);
    }
    s.toUpperCase();
    return s;
}

// ── Poll one reader with debounce ───────────────────────────────────────────
bool pollReader(MFRC522 &mfrc, String &lastUid, uint32_t &lastSeen, String &outUid) {
    if (!mfrc.PICC_IsNewCardPresent()) return false;
    if (!mfrc.PICC_ReadCardSerial())   return false;

    String uid = uidToHex(mfrc.uid);
    mfrc.PICC_HaltA();
    mfrc.PCD_StopCrypto1();

    uint32_t now = millis();
    if (uid == lastUid && (now - lastSeen) < DEBOUNCE_MS) {
        lastSeen = now;
        return false;
    }

    lastUid  = uid;
    lastSeen = now;
    outUid   = uid;
    return true;
}

// ── Setup ───────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n[BOOT] Smart Parking System");

    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SS_IN_PIN);

    pinMode(SS_IN_PIN,  OUTPUT); digitalWrite(SS_IN_PIN,  HIGH);
    pinMode(SS_OUT_PIN, OUTPUT); digitalWrite(SS_OUT_PIN, HIGH);

    readerIn.PCD_Init();
    readerIn.PCD_SetAntennaGain(MFRC522::RxGain_max);
    Serial.printf("[RFID IN]  ver=0x%02X\n",
                  readerIn.PCD_ReadRegister(MFRC522::VersionReg));

    readerOut.PCD_Init();
    readerOut.PCD_SetAntennaGain(MFRC522::RxGain_max);
    Serial.printf("[RFID OUT] ver=0x%02X\n",
                  readerOut.PCD_ReadRegister(MFRC522::VersionReg));

    Serial.println("[RFID] ready");
}

// ── Loop ────────────────────────────────────────────────────────────────────
void loop() {
    String uid;

    if (pollReader(readerIn, lastUidIn, lastSeenIn, uid)) {
        Serial.printf("[CARD IN]  UID=%s\n", uid.c_str());
        // TODO: validate → open entry gate → buzzer → LCD
    }

    if (pollReader(readerOut, lastUidOut, lastSeenOut, uid)) {
        Serial.printf("[CARD OUT] UID=%s\n", uid.c_str());
        // TODO: validate → open exit gate → buzzer → LCD
    }
}