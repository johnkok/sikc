#include <Arduino.h>
#include "board.h"
#include "controller.h"
#include "mqtt.h"
#include "sensors.h"
#include "storage.h"
#include "config.h"

Board board;
Settings settings;
SoilSensors soil;
Controller controller(board, settings);
MqttService mqttService(controller, settings);
bool lastIrrFlow = false, lastWellFlow = false;

void setup()
{
  Serial.begin(115200);
  loadSettings(settings);
  board.begin();
  controller.begin();
  soil.begin();
  mqttService.begin();
  configTzTime(Config::TIMEZONE, "pool.ntp.org", "time.cloudflare.com");
  Serial.println("KC868-A16 irrigation controller started");
}

void loop()
{
  board.update();
  soil.update();
  controller.setSensors(soil.readings());
  bool a = board.input(Config::FLOW_IRRIGATION_IN), b = board.input(Config::FLOW_WELL_IN);
  if (a && !lastIrrFlow)
    controller.onFlowPulse(false);
  if (b && !lastWellFlow)
    controller.onFlowPulse(true);
  lastIrrFlow = a;
  lastWellFlow = b;
  controller.update();
  mqttService.update(soil.readings());
  delay(2);
}
