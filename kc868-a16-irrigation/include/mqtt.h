#pragma once
#include "controller.h"
#include "types.h"
class MqttService
{
public:
    MqttService(Controller &c, Settings &s) : controller_(c), settings_(s) {}
    void begin();
    void update(const SensorReading sensors[2]);
    void command(char *topic, byte *payload, unsigned int len);
    bool connected() const;

private:
    void connect();
    void publishDiscovery();
    void publishState(const SensorReading s[2]);
    Controller &controller_;
    Settings &settings_;
    uint32_t lastPublish_ = 0;
};
