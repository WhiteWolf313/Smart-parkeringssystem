#ifndef RFID_MANAGER_H
#define RFID_MANAGER_H

#include "RfidReader.h"

enum class Direction : uint8_t { IN = 0, OUT = 1 };

inline const char* directionToStr(Direction d) {
    return d == Direction::IN ? "IN" : "OUT";
}

typedef void (*RfidCardCallback)(Direction dir, const String& uid);

class RfidManager {
public:
    RfidManager(uint8_t inSs, uint8_t inRst, uint8_t outSs, uint8_t outRst);
    void begin();
    void loop();
    void onCard(RfidCardCallback cb) { _cb = cb; }
    void setDebounceMs(uint32_t ms);

private:
    RfidReader        _in;
    RfidReader        _out;
    RfidCardCallback  _cb = nullptr;
};

#endif