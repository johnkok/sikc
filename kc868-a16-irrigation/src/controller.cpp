#include "controller.h"
#include "config.h"
#include <cstring>

void Controller::begin()
{
  for (uint8_t i = 0; i < 16; i++)
    board_.setOutput(i, false);
  lastWellOff_ = millis();
  allOffSince_ = millis();
}
void Controller::setSensors(const SensorReading r[2]) { memcpy(sensors_, r, sizeof(sensors_)); }
void Controller::prepareForFirmwareUpdate()
{
  maintenanceMode_ = true;
  stopIrrigation();
  setWell(false);
}
void Controller::onFlowPulse(bool well)
{
  if (well)
    wellPulses_++;
  else
    irrigationPulses_++;
}
void Controller::latch(Fault f)
{
  faults_ |= uint16_t(f);
  stopIrrigation();
  if (f != Fault::IrrigationNoFlow && f != Fault::Leak)
    setWell(false);
  board_.setOutput(Config::ALARM_OUT, true);
}
bool Controller::criticalFault() const { return faults_ != 0; }
void Controller::clearFaults()
{
  if (board_.input(Config::WELL_FAULT_IN) || board_.input(Config::WELL_DRY_IN))
    return;
  if (board_.input(Config::TANK_EMPTY_IN) && board_.input(Config::TANK_FULL_IN))
    return;
  faults_ = 0;
  board_.setOutput(Config::ALARM_OUT, false);
}
void Controller::setWell(bool on)
{
  if (on == wellOn_)
    return;
  if (on)
  {
    if (criticalFault())
      return;
    wellStarted_ = millis();
  }
  else
    lastWellOff_ = millis();
  wellOn_ = on;
  board_.setOutput(Config::WELL_PUMP_OUT, on);
}
void Controller::stopIrrigation()
{
  for (uint8_t i = 0; i < 4; i++)
  {
    zoneOn_[i] = false;
    board_.setOutput(Config::ZONE_OUT[i], false);
  }
  masterOn_ = false;
  board_.setOutput(Config::MASTER_OUT, false);
  board_.setOutput(Config::TANK_OUTLET_PUMP_OUT, false);
  allOffSince_ = millis();
}
bool Controller::requestZone(uint8_t z, bool on, bool manual)
{
  if (z >= 4)
    return false;
  if (on)
  {
    if (criticalFault() || maintenanceMode_ || weatherBlock_ || board_.input(Config::RAIN_IN) || board_.input(Config::TANK_EMPTY_IN) || refillState_ != 0)
      return false;
    if (!masterOn_)
    {
      masterOn_ = true;
      board_.setOutput(Config::MASTER_OUT, true);
      board_.setOutput(Config::TANK_OUTLET_PUMP_OUT, true);
    }
    zoneOn_[z] = true;
    zoneStarted_[z] = millis() + Config::MASTER_LEAD_MS; // lead time enforced below
    if (irrigationCycleStarted_ == 0)
      irrigationCycleStarted_ = millis();
  }
  else
  {
    zoneOn_[z] = false;
    scheduledStopAt_[z] = 0;
    board_.setOutput(Config::ZONE_OUT[z], false);
    allOffSince_ = millis();
  }
  return true;
}
const char *Controller::tankState() const
{
  if (faults_ & uint16_t(Fault::TankSensors))
    return "fault";
  if (refillState_)
    return wellOn_ ? "filling" : "refill_delay";
  if (board_.input(Config::TANK_FULL_IN))
    return "full";
  if (board_.input(Config::TANK_EMPTY_IN))
    return "empty";
  return "mid";
}
void Controller::updateTank(uint32_t now)
{
  bool empty = board_.input(Config::TANK_EMPTY_IN), full = board_.input(Config::TANK_FULL_IN);
  if (empty && full)
  {
    latch(Fault::TankSensors);
    return;
  }
  if (board_.input(Config::WELL_FAULT_IN))
  {
    latch(Fault::WellOverload);
    return;
  }
  if (board_.input(Config::WELL_DRY_IN))
  {
    latch(Fault::WellDry);
    return;
  }
  if (full)
  {
    setWell(false);
    refillState_ = 0;
    return;
  }
  if (empty && refillState_ == 0)
  {
    stopIrrigation();
    refillState_ = 1;
    refillStateSince_ = now;
  }
  if (refillState_ == 1 && now - refillStateSince_ >= settings_.refillDelaySec * 1000UL && now - lastWellOff_ >= settings_.minPumpOffSec * 1000UL)
  {
    setWell(true);
    refillState_ = 2;
    refillStateSince_ = now;
  }
  if (refillState_ == 2)
  {
    if (now - wellStarted_ >= settings_.maxRefillSec * 1000UL)
    {
      latch(Fault::RefillTimeout);
      return;
    }
    if (now - wellStarted_ >= settings_.noFlowTimeoutSec * 1000UL && wellLpm_ < settings_.minWellFlowLpm)
      latch(Fault::WellNoFlow);
  }
}
void Controller::updateIrrigation(uint32_t now)
{
  struct tm tmNow;
  bool haveTime = getLocalTime(&tmNow, 0);
  int dayKey = haveTime ? (tmNow.tm_year * 400 + tmNow.tm_yday) : -1;
  for (uint8_t z = 0; z < 4; z++)
    if (!zoneOn_[z])
    {
      auto &zc = settings_.zones[z];
      int8_t si = zc.sensorIndex;
      if (zc.automatic && si >= 0 && si < 2 && sensors_[si].valid && sensors_[si].moisture <= zc.startMoisture)
        requestZone(z, true);
      auto &sc = zc.schedule;
      if (haveTime && sc.enabled && (sc.days & (1u << tmNow.tm_wday)) && tmNow.tm_hour == sc.hour && tmNow.tm_min == sc.minute && lastScheduleDay_[z] != dayKey)
      {
        lastScheduleDay_[z] = dayKey;
        if (requestZone(z, true))
          scheduledStopAt_[z] = now + sc.durationMin * 60000UL;
      }
    }
  bool any = false;
  for (uint8_t z = 0; z < 4; z++)
    if (zoneOn_[z])
    {
      any = true;
      if (now >= zoneStarted_[z])
        board_.setOutput(Config::ZONE_OUT[z], true);
      if (scheduledStopAt_[z] && (int32_t)(now - scheduledStopAt_[z]) >= 0)
        requestZone(z, false);
      if (now - zoneStarted_[z] >= settings_.zones[z].maxRuntimeMin * 60000UL)
        requestZone(z, false);
      int8_t si = settings_.zones[z].sensorIndex;
      if (settings_.zones[z].automatic && si >= 0 && si < 2 && sensors_[si].valid && sensors_[si].moisture >= settings_.zones[z].stopMoisture)
        requestZone(z, false);
    }
  any = false;
  for (bool v : zoneOn_)
    any |= v;
  if (!any && masterOn_ && now - allOffSince_ >= Config::MASTER_LAG_MS)
  {
    masterOn_ = false;
    board_.setOutput(Config::MASTER_OUT, false);
    board_.setOutput(Config::TANK_OUTLET_PUMP_OUT, false);
    allOffSince_ = now;
    irrigationCycleStarted_ = 0;
  }
  if (any && irrigationCycleStarted_ && now - irrigationCycleStarted_ >= Config::FLOW_GRACE_MS && irrigationLpm_ < settings_.minIrrigationFlowLpm)
    latch(Fault::IrrigationNoFlow);
  if (!any && !masterOn_ && now - allOffSince_ > Config::FLOW_GRACE_MS && irrigationLpm_ > settings_.leakFlowLpm)
    latch(Fault::Leak);
}
void Controller::updateFlow(uint32_t now)
{
  if (now - lastFlowCalc_ < Config::FLOW_WINDOW_MS)
    return;
  uint32_t dt = now - lastFlowCalc_;
  lastFlowCalc_ = now;
  noInterrupts();
  uint32_t ip = irrigationPulses_, wp = wellPulses_;
  irrigationPulses_ = wellPulses_ = 0;
  interrupts();
  irrigationLiters_ += ip / settings_.irrigationPulsesPerLiter;
  wellLiters_ += wp / settings_.wellPulsesPerLiter;
  irrigationLpm_ = (ip / settings_.irrigationPulsesPerLiter) * 60000.0f / dt;
  wellLpm_ = (wp / settings_.wellPulsesPerLiter) * 60000.0f / dt;
}
void Controller::update()
{
  uint32_t now = millis();
  updateFlow(now);
  if (maintenanceMode_)
  {
    stopIrrigation();
    setWell(false);
    return;
  }
  updateTank(now);
  if (criticalFault())
  {
    stopIrrigation();
    return;
  }
  updateIrrigation(now);
}
