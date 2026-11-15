#include "sensors.h"
#include "config.h"
#include <ModbusMaster.h>

static HardwareSerial rs485(2);
static ModbusMaster node;

void SoilSensors::begin() {
  rs485.begin(Config::RS485_BAUD, SERIAL_8N1, Config::RS485_RX, Config::RS485_TX);
}

void SoilSensors::update()
{
  uint32_t now = millis();

  if (now - lastPoll_ < Config::SENSOR_POLL_MS / 2)
    return;

  Serial.println("Sensor update");

  lastPoll_ = now;
  node.begin(Config::SENSOR_IDS[next_], rs485);

  auto &r = readings_[next_];
  uint8_t result = node.readHoldingRegisters(Config::REG_SOIL_START, Config::SENSOR_REG_COUNT);
  if (result == node.ku8MBSuccess)
  {
    r.temperature = (int16_t)node.getResponseBuffer(0) / 10.0f;
    r.moisture = node.getResponseBuffer(1) / 10.0f;
    r.ec = node.getResponseBuffer(2);
    r.ph = node.getResponseBuffer(3) / 10.0f;
    r.nitrogen = node.getResponseBuffer(4);
    r.phosphorus = node.getResponseBuffer(5);
    r.potassium = node.getResponseBuffer(6);
    r.valid = true;
    r.lastOkMs = now;
    Serial.println("Sensor success");
  }
  else if (now - r.lastOkMs > 60000)
    r.valid = false;
  next_ = (next_ + 1) % Config::SENSOR_COUNT;
}
