#ifndef RFID_READER_H
#define RFID_READER_H

#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>

class RfidReader {
public:
    RfidReader(const char* name, uint8_t ssPin, uint8_t rstPin);
    void begin();
    bool poll(String& outUid);
    void setDebounceMs(uint32_t ms) { _debounceMs = ms; }
    const char* name() const { return _name; }
    MFRC522& driver() { return _mfrc; }

private:
    const char* _name;
    uint8_t     _ssPin;
    uint8_t     _rstPin;
    MFRC522     _mfrc;
    String      _lastUid;
    uint32_t    _lastSeenMs = 0;
    uint32_t    _debounceMs = 1500;

    static String uidToHex(const MFRC522::Uid& uid);
};

#endif