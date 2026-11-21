#include <Arduino.h>
#include "board.h"
#include "controller.h"
#include "mqtt.h"
#include "ota.h"
#include "sensors.h"
#include "storage.h"
#include "config.h"
#include "display.h"
#include <WiFi.h>

Board board;
Settings settings;
SoilSensors soil;
Controller controller(board, settings);
MqttService mqttService(controller, settings);
StatusDisplay statusDisplay(controller, settings);
OtaService otaService(controller, statusDisplay);
bool lastIrrFlow = false, lastWellFlow = false;

void setup()
{
  Serial.begin(115200);
  loadSettings(settings);
  board.begin();
  statusDisplay.begin();
  controller.begin();
  if (Config::ENABLE_SOIL_SENSORS)
    soil.begin();
  mqttService.begin();
  configTzTime(Config::TIMEZONE, "pool.ntp.org", "time.cloudflare.com");
  Serial.println("KC868-A16 irrigation controller started");
}

void loop()
{
  board.update();
  if (Config::ENABLE_SOIL_SENSORS)
    soil.update();
  controller.setSensors(soil.readings());
  if (Config::ENABLE_FLOW_SENSORS)
  {
    bool a = board.input(Config::FLOW_IRRIGATION_IN), b = board.input(Config::FLOW_WELL_IN);
    if (a && !lastIrrFlow)
      controller.onFlowPulse(false);
    if (b && !lastWellFlow)
      controller.onFlowPulse(true);
    lastIrrFlow = a;
    lastWellFlow = b;
  }
  controller.update();
  mqttService.update(soil.readings());
  statusDisplay.update(soil.readings(), WiFi.status() == WL_CONNECTED, mqttService.connected());
  otaService.update();
  delay(2);
}
