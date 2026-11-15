#include "board.h"
#include "config.h"
#include <Wire.h>

bool Board::begin()
{
  Wire.begin(Config::I2C_SDA, Config::I2C_SCL);
  outputs_ = 0;
  healthy_ = setOutput(0, false);
  for (uint8_t i = 1; i < 16; i++)
    setOutput(i, false);
  return healthy_;
}
bool Board::readPort(uint8_t a, uint8_t &v)
{
  Wire.requestFrom(a, (uint8_t)1);
  if (!Wire.available())
    return false;
  v = Wire.read();
  return true;
}
bool Board::writePort(uint8_t a, uint8_t v)
{
  Wire.beginTransmission(a);
  Wire.write(v);
  return Wire.endTransmission() == 0;
}
bool Board::setOutput(uint8_t ch, bool on)
{
  if (ch >= 16)
    return false;
  if (on)
    outputs_ |= (1u << ch);
  else
    outputs_ &= ~(1u << ch);
  uint8_t lo = outputs_ & 0xff, hi = outputs_ >> 8;
  if (Config::OUTPUT_ACTIVE_LOW)
  {
    lo = ~lo;
    hi = ~hi;
  }
  healthy_ = writePort(Config::OUTPUT_EXPANDER_1, lo) && writePort(Config::OUTPUT_EXPANDER_2, hi);
  return healthy_;
}
bool Board::output(uint8_t ch) const { return ch < 16 && (outputs_ & (1u << ch)); }
bool Board::input(uint8_t ch) const
{
  if (ch >= 16)
    return false;
  bool v = stableInputs_ & (1u << ch);
  return Config::INPUT_ACTIVE_LOW ? !v : v;
}
void Board::update()
{
  uint32_t now = millis();
  if (now - lastScan_ < Config::INPUT_SCAN_MS)
    return;
  lastScan_ = now;
  uint8_t lo, hi;
  if (!readPort(Config::INPUT_EXPANDER_1, lo) || !readPort(Config::INPUT_EXPANDER_2, hi))
  {
    healthy_ = false;
    return;
  }
  healthy_ = true;
  rawInputs_ = uint16_t(lo) | (uint16_t(hi) << 8);
  if (rawInputs_ != candidate_)
  {
    candidate_ = rawInputs_;
    candidateSince_ = now;
  }
  else if (now - candidateSince_ >= Config::INPUT_DEBOUNCE_MS)
    stableInputs_ = candidate_;
}
