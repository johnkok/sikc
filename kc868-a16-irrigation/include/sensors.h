#pragma once
#include "types.h"
class SoilSensors
{
public:
    void begin();
    void update();
    const SensorReading *readings() const { return readings_; }

private:
    SensorReading readings_[2];
    uint32_t lastPoll_ = 0;
    uint8_t next_ = 0;
};
