#include "mqtt.h"
#include "config.h"
#include "storage.h"
#include "secrets.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
static WiFiClient net;
static PubSubClient mqtt(net);
static MqttService *self = nullptr;
static void callback(char *t, byte *p, unsigned int l)
{
  if (self)
    self->command(t, p, l);
}
void MqttService::begin()
{
  self = this;
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setBufferSize(4096);
  mqtt.setCallback(callback);
}
void MqttService::connect()
{
  if (WiFi.status() != WL_CONNECTED || mqtt.connected())
    return;
  if (mqtt.connect(Config::DEVICE_ID, MQTT_USER, MQTT_PASSWORD, (String(Config::MQTT_PREFIX) + "/availability").c_str(), 1, true, "offline"))
  {
    mqtt.publish((String(Config::MQTT_PREFIX) + "/availability").c_str(), "online", true);
    mqtt.subscribe((String(Config::MQTT_PREFIX) + "/cmd/#").c_str());
    publishDiscovery();
  }
}
void MqttService::publishDiscovery()
{
  auto entity = [&](const char *component, const String &id, const String &name, const String &stateTopic, const char *valueTemplate, const String &commandTopic = "", const char *unit = nullptr, const char *stateClass = nullptr, const char *deviceClass = nullptr)
  {
    JsonDocument d;
    d["name"] = name;
    d["unique_id"] = String(Config::DEVICE_ID) + "_" + id;
    d["state_topic"] = stateTopic;
    d["availability_topic"] = String(Config::MQTT_PREFIX) + "/availability";
    if (valueTemplate && *valueTemplate)
      d["value_template"] = valueTemplate;
    if (unit && *unit)
      d["unit_of_measurement"] = unit;
    if (stateClass && *stateClass)
      d["state_class"] = stateClass;
    if (deviceClass && *deviceClass)
      d["device_class"] = deviceClass;
    if (commandTopic.length())
      d["command_topic"] = commandTopic;
    JsonObject dev = d["device"].to<JsonObject>();
    dev["identifiers"][0] = Config::DEVICE_ID;
    dev["name"] = Config::DEVICE_NAME;
    dev["manufacturer"] = "KinCony/custom";
    dev["model"] = "KC868-A16 Irrigation";
    String out;
    serializeJson(d, out);
    String topic = "homeassistant/" + String(component) + "/" + Config::DEVICE_ID + "/" + id + "/config";
    mqtt.publish(topic.c_str(), out.c_str(), true);
  };
  String st = String(Config::MQTT_PREFIX) + "/state";
  for (int i = 0; i < 4; i++)
    entity("switch", "zone" + String(i + 1), "Zone " + String(i + 1), st, ("{{ value_json.zone" + String(i + 1) + " }}").c_str(), String(Config::MQTT_PREFIX) + "/cmd/zone" + String(i + 1));
  entity("switch", "weather_block", "Weather block", st, "{{ value_json.weather_block }}", String(Config::MQTT_PREFIX) + "/cmd/weather_block");
  entity("button", "clear_faults", "Clear faults", st, "", String(Config::MQTT_PREFIX) + "/cmd/clear_faults");
  entity("sensor", "tank_state", "Tank state", st, "{{ value_json.tank_state }}");
  entity("binary_sensor", "well_pump", "Well pump", st, "{{ value_json.well_pump }}");
  entity("sensor", "faults", "Fault mask", st, "{{ value_json.faults }}");
  entity("sensor", "irrigation_flow", "Irrigation flow", st, "{{ value_json.irrigation_lpm }}");
  entity("sensor", "well_flow", "Well flow", st, "{{ value_json.well_lpm }}");
  for (int i = 0; i < 2; i++)
  {
    entity("sensor", "soil" + String(i + 1) + "_moisture", "Soil " + String(i + 1) + " moisture", st, ("{{ value_json.soil" + String(i + 1) + "_moisture }}").c_str(), "", "%", "measurement");
    entity("sensor", "soil" + String(i + 1) + "_temperature", "Soil " + String(i + 1) + " temperature", st, ("{{ value_json.soil" + String(i + 1) + "_temperature }}").c_str(), "", "°C", "measurement", "temperature");
    entity("sensor", "soil" + String(i + 1) + "_ec", "Soil " + String(i + 1) + " EC", st, ("{{ value_json.soil" + String(i + 1) + "_ec }}").c_str(), "", "µS/cm", "measurement");
    entity("sensor", "soil" + String(i + 1) + "_ph", "Soil " + String(i + 1) + " pH", st, ("{{ value_json.soil" + String(i + 1) + "_ph }}").c_str(), "", "pH", "measurement");
    entity("sensor", "soil" + String(i + 1) + "_nitrogen", "Soil " + String(i + 1) + " nitrogen", st, ("{{ value_json.soil" + String(i + 1) + "_nitrogen }}").c_str(), "", "mg/kg", "measurement");
    entity("sensor", "soil" + String(i + 1) + "_phosphorus", "Soil " + String(i + 1) + " phosphorus", st, ("{{ value_json.soil" + String(i + 1) + "_phosphorus }}").c_str(), "", "mg/kg", "measurement");
    entity("sensor", "soil" + String(i + 1) + "_potassium", "Soil " + String(i + 1) + " potassium", st, ("{{ value_json.soil" + String(i + 1) + "_potassium }}").c_str(), "", "mg/kg", "measurement");
  }
}
void MqttService::command(char *topic, byte *payload, unsigned int len)
{
  String t(topic), v;
  for (unsigned i = 0; i < len; i++)
    v += (char)payload[i];
  v.trim();
  bool on = v == "ON" || v == "1" || v == "true";
  if (t.indexOf("/cmd/config/zone") >= 0)
  {
    int z = t.substring(t.lastIndexOf("zone") + 4).toInt() - 1;
    if (z < 0 || z >= 4)
      return;
    JsonDocument d;
    if (deserializeJson(d, v))
      return;
    auto &c = settings_.zones[z];
    c.automatic = d["automatic"] | c.automatic;
    c.sensorIndex = d["sensor_index"] | c.sensorIndex;
    c.startMoisture = d["start_moisture"] | c.startMoisture;
    c.stopMoisture = d["stop_moisture"] | c.stopMoisture;
    c.maxRuntimeMin = d["max_runtime_min"] | c.maxRuntimeMin;
    c.schedule.enabled = d["schedule_enabled"] | c.schedule.enabled;
    c.schedule.hour = d["hour"] | c.schedule.hour;
    c.schedule.minute = d["minute"] | c.schedule.minute;
    c.schedule.durationMin = d["duration_min"] | c.schedule.durationMin;
    c.schedule.days = d["days"] | c.schedule.days;
    c.schedule.hour = constrain(c.schedule.hour, 0, 23);
    c.schedule.minute = constrain(c.schedule.minute, 0, 59);
    c.schedule.durationMin = constrain(c.schedule.durationMin, 1, 1440);
    c.maxRuntimeMin = constrain(c.maxRuntimeMin, 1, 1440);
    if (c.stopMoisture <= c.startMoisture)
      c.stopMoisture = c.startMoisture + 1.0f;
    saveSettings(settings_);
    return;
  }
  if (t.endsWith("/cmd/config/system"))
  {
    JsonDocument d;
    if (deserializeJson(d, v))
      return;
    settings_.refillDelaySec = d["refill_delay_sec"] | settings_.refillDelaySec;
    settings_.minPumpOffSec = d["min_pump_off_sec"] | settings_.minPumpOffSec;
    settings_.maxRefillSec = d["max_refill_sec"] | settings_.maxRefillSec;
    saveSettings(settings_);
    return;
  }
  int pos = t.lastIndexOf("zone");
  if (pos >= 0)
  {
    int z = t.substring(pos + 4).toInt() - 1;
    if (z >= 0 && z < 4)
      controller_.requestZone(z, on, true);
  }
  else if (t.endsWith("weather_block"))
    controller_.setWeatherBlock(on);
  else if (t.endsWith("clear_faults"))
    controller_.clearFaults();
}
void MqttService::publishState(const SensorReading s[2])
{
  JsonDocument d;
  for (int i = 0; i < 4; i++)
    d["zone" + String(i + 1)] = controller_.zoneOn(i) ? "ON" : "OFF";
  d["well_pump"] = controller_.wellOn() ? "ON" : "OFF";
  d["tank_state"] = controller_.tankState();
  d["faults"] = controller_.faultMask();
  d["irrigation_lpm"] = controller_.irrigationLpm();
  d["well_lpm"] = controller_.wellLpm();
  d["irrigation_liters"] = controller_.irrigationLiters();
  d["well_liters"] = controller_.wellLiters();
  d["weather_block"] = controller_.weatherBlocked() ? "ON" : "OFF";
  for (int i = 0; i < 2; i++)
  {
    if (s[i].valid)
    {
      d["soil" + String(i + 1) + "_moisture"] = s[i].moisture;
      d["soil" + String(i + 1) + "_temperature"] = s[i].temperature;
      d["soil" + String(i + 1) + "_ec"] = s[i].ec;
      d["soil" + String(i + 1) + "_ph"] = s[i].ph;
      d["soil" + String(i + 1) + "_nitrogen"] = s[i].nitrogen;
      d["soil" + String(i + 1) + "_phosphorus"] = s[i].phosphorus;
      d["soil" + String(i + 1) + "_potassium"] = s[i].potassium;
    }
    else
    {
      d["soil" + String(i + 1) + "_moisture"] = nullptr;
      d["soil" + String(i + 1) + "_temperature"] = nullptr;
      d["soil" + String(i + 1) + "_ec"] = nullptr;
      d["soil" + String(i + 1) + "_ph"] = nullptr;
      d["soil" + String(i + 1) + "_nitrogen"] = nullptr;
      d["soil" + String(i + 1) + "_phosphorus"] = nullptr;
      d["soil" + String(i + 1) + "_potassium"] = nullptr;
    }
  }
  JsonArray plans = d["plans"].to<JsonArray>();
  for (int i = 0; i < 4; i++)
  {
    auto &c = settings_.zones[i];
    JsonObject p = plans.add<JsonObject>();
    p["automatic"] = c.automatic;
    p["sensor_index"] = c.sensorIndex;
    p["start_moisture"] = c.startMoisture;
    p["stop_moisture"] = c.stopMoisture;
    p["max_runtime_min"] = c.maxRuntimeMin;
    p["schedule_enabled"] = c.schedule.enabled;
    p["hour"] = c.schedule.hour;
    p["minute"] = c.schedule.minute;
    p["duration_min"] = c.schedule.durationMin;
    p["days"] = c.schedule.days;
  }
  String out;
  serializeJson(d, out);
  mqtt.publish((String(Config::MQTT_PREFIX) + "/state").c_str(), out.c_str(), true);
}
void MqttService::update(const SensorReading s[2])
{
  connect();
  mqtt.loop();
  if (millis() - lastPublish_ >= Config::MQTT_PUBLISH_MS)
  {
    lastPublish_ = millis();
    if (mqtt.connected())
      publishState(s);
  }
}
