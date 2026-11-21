#pragma once

#include "controller.h"
#include "types.h"
#include <Adafruit_NeoPixel.h>

class StatusDisplay
{
public:
  StatusDisplay(Controller &controller, Settings &settings);
  void begin();
  void update(const SensorReading sensors[2], bool wifiConnected, bool mqttConnected);
  void otaStarted();
  void otaProgress(uint8_t percent);
  void otaFinished();
  void otaError();

private:
  enum class OtaState : uint8_t { Idle, Running, Finished, Error };
  void render(const SensorReading sensors[2], bool wifiConnected, bool mqttConnected);
  void renderStatus(bool wifiConnected, bool mqttConnected);
  void renderSensor(const SensorReading sensors[2]);
  void renderOta();
  void setPixel(uint8_t matrix, int8_t x, int8_t y, uint32_t color);
  void drawDigit(uint8_t matrix, int8_t x, int8_t y, uint8_t digit, uint32_t color);
  void drawNumber(uint8_t matrix, int value, uint32_t color);
  void drawX(uint8_t matrix, uint32_t color);
  void drawCheck(uint8_t matrix, uint32_t color);

  Controller &controller_;
  Settings &settings_;
  Adafruit_NeoPixel pixels_;
  uint32_t lastFrame_ = 0;
  uint8_t appliedBrightness_ = 255;
  OtaState otaState_ = OtaState::Idle;
  uint8_t otaPercent_ = 0;
};
