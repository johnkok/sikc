#pragma once
#include <Arduino.h>

enum class Fault : uint16_t
{
  None = 0,
  TankSensors = 1,
  RefillTimeout = 2,
  WellOverload = 4,
  WellDry = 8,
  WellNoFlow = 16,
  IrrigationNoFlow = 32,
  Leak = 64,
  SensorFailure = 128
};
inline Fault operator|(Fault a, Fault b) { return Fault(uint16_t(a) | uint16_t(b)); }

struct SensorReading
{
  float moisture = NAN, temperature = NAN, ec = NAN, ph = NAN;
  float nitrogen = NAN, phosphorus = NAN, potassium = NAN;
  bool valid = false;
  uint32_t lastOkMs = 0;
};

struct Schedule
{
  bool enabled = false;
  uint8_t hour = 6, minute = 0;
  uint16_t durationMin = 10;
  uint8_t days = 0x7F; // bit 0=Sunday
};

struct ZoneConfig
{
  bool automatic = true;
  int8_t sensorIndex = -1;
  float startMoisture = 30.0f;
  float stopMoisture = 45.0f;
  uint16_t maxRuntimeMin = 30;
  Schedule schedule;
};

struct Settings
{
  uint32_t version = 1;
  ZoneConfig zones[4];
  uint32_t refillDelaySec = 300;
  uint32_t minPumpOffSec = 300;
  uint32_t maxRefillSec = 3600;
  uint32_t noFlowTimeoutSec = 20;
  float irrigationPulsesPerLiter = 450.0f;
  float wellPulsesPerLiter = 450.0f;
  float minIrrigationFlowLpm = 0.25f;
  float minWellFlowLpm = 0.25f;
  float leakFlowLpm = 0.20f;
  bool requireFullBeforeResume = true;
};
