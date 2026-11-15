#pragma once
#include <Arduino.h>

class Board
{
public:
  bool begin();
  void update();
  bool input(uint8_t channel) const;
  bool setOutput(uint8_t channel, bool on);
  bool output(uint8_t channel) const;
  bool healthy() const { return healthy_; }

private:
  bool readPort(uint8_t address, uint8_t &value);
  bool writePort(uint8_t address, uint8_t value);
  uint16_t rawInputs_ = 0xFFFF, stableInputs_ = 0xFFFF, outputs_ = 0;
  uint16_t candidate_ = 0xFFFF;
  uint32_t candidateSince_ = 0, lastScan_ = 0;
  bool healthy_ = false;
};
