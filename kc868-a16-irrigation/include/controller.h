#pragma once
#include "board.h"
#include "types.h"

class Controller
{
public:
  Controller(Board &b, Settings &s) : board_(b), settings_(s) {}
  void begin();
  void update();
  bool requestZone(uint8_t zone, bool on, bool manual = false);
  void setWeatherBlock(bool b)
  {
    weatherBlock_ = b;
    if (b)
      stopIrrigation();
  }
  void clearFaults();
  void onFlowPulse(bool well);
  void setSensors(const SensorReading r[2]);
  uint16_t faultMask() const { return faults_; }
  bool zoneOn(uint8_t z) const { return z < 4 && zoneOn_[z]; }
  bool wellOn() const { return wellOn_; }
  bool masterOn() const { return masterOn_; }
  bool filling() const { return refillState_ != 0; }
  float irrigationLpm() const { return irrigationLpm_; }
  bool weatherBlocked() const { return weatherBlock_; }
  float wellLpm() const { return wellLpm_; }
  double irrigationLiters() const { return irrigationLiters_; }
  double wellLiters() const { return wellLiters_; }
  const char *tankState() const;

private:
  void updateTank(uint32_t now);
  void updateIrrigation(uint32_t now);
  void updateFlow(uint32_t now);
  void setWell(bool on);
  void stopIrrigation();
  void latch(Fault f);
  bool criticalFault() const;
  Board &board_;
  Settings &settings_;
  SensorReading sensors_[2];
  bool zoneOn_[4] = {};
  uint32_t zoneStarted_[4] = {};
  bool masterOn_ = false, wellOn_ = false, weatherBlock_ = false;
  uint32_t irrigationCycleStarted_ = 0;
  int32_t lastScheduleDay_[4] = {-1, -1, -1, -1};
  uint32_t scheduledStopAt_[4] = {};
  uint8_t refillState_ = 0;
  uint32_t refillStateSince_ = 0, lastWellOff_ = 0, wellStarted_ = 0;
  volatile uint32_t irrigationPulses_ = 0, wellPulses_ = 0;
  uint32_t lastFlowCalc_ = 0;
  float irrigationLpm_ = 0, wellLpm_ = 0;
  double irrigationLiters_ = 0, wellLiters_ = 0;
  uint16_t faults_ = 0;
  uint32_t allOffSince_ = 0;
};
