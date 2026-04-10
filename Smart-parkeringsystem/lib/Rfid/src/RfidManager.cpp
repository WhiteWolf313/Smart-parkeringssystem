#include "RfidManager.h"

RfidManager::RfidManager(uint8_t inSs, uint8_t inRst, uint8_t outSs, uint8_t outRst)
    : _in("IN", inSs, inRst), _out("OUT", outSs, outRst) {}

void RfidManager::begin() {
    SPI.begin();
    _in.begin();
    _out.begin();
    Serial.println("[RFID] manager ready");
}

void RfidManager::loop() {
    String uid;
    if (_in.poll(uid)) {
        Serial.printf("[RFID IN]  UID=%s\n", uid.c_str());
        if (_cb) _cb(Direction::IN, uid);
    }
    if (_out.poll(uid)) {
        Serial.printf("[RFID OUT] UID=%s\n", uid.c_str());
        if (_cb) _cb(Direction::OUT, uid);
    }
}

void RfidManager::setDebounceMs(uint32_t ms) {
    _in.setDebounceMs(ms);
    _out.setDebounceMs(ms);
}