#include "RfidReader.h"

RfidReader::RfidReader(const char* name, uint8_t ssPin, uint8_t rstPin)
    : _name(name), _ssPin(ssPin), _rstPin(rstPin), _mfrc(ssPin, rstPin) {}

void RfidReader::begin() {
    pinMode(_ssPin, OUTPUT);
    digitalWrite(_ssPin, HIGH);
    _mfrc.PCD_Init();
    _mfrc.PCD_SetAntennaGain(MFRC522::RxGain_max);
    Serial.printf("[RFID %s] init on SS=%u RST=%u  ver=0x%02X\n",
                  _name, _ssPin, _rstPin,
                  _mfrc.PCD_ReadRegister(MFRC522::VersionReg));
}

bool RfidReader::poll(String& outUid) {
    if (!_mfrc.PICC_IsNewCardPresent()) return false;
    if (!_mfrc.PICC_ReadCardSerial())   return false;

    String uid = uidToHex(_mfrc.uid);
    _mfrc.PICC_HaltA();
    _mfrc.PCD_StopCrypto1();

    const uint32_t now = millis();
    if (uid == _lastUid && (now - _lastSeenMs) < _debounceMs) {
        _lastSeenMs = now;
        return false;
    }
    _lastUid    = uid;
    _lastSeenMs = now;
    outUid      = uid;
    return true;
}

String RfidReader::uidToHex(const MFRC522::Uid& uid) {
    String s;
    s.reserve(uid.size * 2);
    for (byte i = 0; i < uid.size; i++) {
        if (uid.uidByte[i] < 0x10) s += '0';
        s += String(uid.uidByte[i], HEX);
    }
    s.toUpperCase();
    return s;
}