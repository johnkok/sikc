#include "storage.h"
#include "config.h"
#include <Preferences.h>
#include <cstring>

// Version 1 is kept here so adding display settings does not erase existing
// irrigation plans and calibrated system values on the first version 2 boot.
struct SettingsV1
{
    uint32_t version;
    ZoneConfig zones[4];
    uint32_t refillDelaySec;
    uint32_t minPumpOffSec;
    uint32_t maxRefillSec;
    uint32_t noFlowTimeoutSec;
    float irrigationPulsesPerLiter;
    float wellPulsesPerLiter;
    float minIrrigationFlowLpm;
    float minWellFlowLpm;
    float leakFlowLpm;
    bool requireFullBeforeResume;
};

void defaults(Settings &s)
{
    s = Settings{};
    s.version = Config::NVS_VERSION;
    s.displayBrightness = Config::DISPLAY_DEFAULT_BRIGHTNESS;
    s.zones[0].sensorIndex = 0;
    s.zones[1].sensorIndex = 1;
    s.zones[2].sensorIndex = -1;
    s.zones[3].sensorIndex = -1;
}
bool loadSettings(Settings &s)
{
    Preferences p;
    p.begin("irrigation", true);
    const size_t storedSize = p.getBytesLength("settings");
    if (storedSize == sizeof(SettingsV1))
    {
        SettingsV1 old{};
        p.getBytes("settings", &old, sizeof(old));
        if (old.version == 1)
        {
            p.end();
            defaults(s);
            memcpy(s.zones, old.zones, sizeof(s.zones));
            s.refillDelaySec = old.refillDelaySec;
            s.minPumpOffSec = old.minPumpOffSec;
            s.maxRefillSec = old.maxRefillSec;
            s.noFlowTimeoutSec = old.noFlowTimeoutSec;
            s.irrigationPulsesPerLiter = old.irrigationPulsesPerLiter;
            s.wellPulsesPerLiter = old.wellPulsesPerLiter;
            s.minIrrigationFlowLpm = old.minIrrigationFlowLpm;
            s.minWellFlowLpm = old.minWellFlowLpm;
            s.leakFlowLpm = old.leakFlowLpm;
            s.requireFullBeforeResume = old.requireFullBeforeResume;
            saveSettings(s);
            return true;
        }
    }
    if (storedSize != sizeof(Settings))
    {
        p.end();
        defaults(s);
        return false;
    }
    p.getBytes("settings", &s, sizeof(s));
    p.end();
    if (s.version != Config::NVS_VERSION)
    {
        defaults(s);
        return false;
    }
    s.displayBrightness = constrain(s.displayBrightness, 0, 100);
    return true;
}
bool saveSettings(const Settings &s)
{
    Preferences p;
    if (!p.begin("irrigation", false))
        return false;
    bool ok = p.putBytes("settings", &s, sizeof(s)) == sizeof(s);
    p.end();
    return ok;
}
