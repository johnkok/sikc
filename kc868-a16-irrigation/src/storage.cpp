#include "storage.h"
#include "config.h"
#include <Preferences.h>
#include <cstring>
void defaults(Settings &s)
{
    s = Settings{};
    s.version = Config::NVS_VERSION;
    s.zones[0].sensorIndex = 0;
    s.zones[1].sensorIndex = 1;
    s.zones[2].sensorIndex = -1;
    s.zones[3].sensorIndex = -1;
}
bool loadSettings(Settings &s)
{
    Preferences p;
    p.begin("irrigation", true);
    if (p.getBytesLength("settings") != sizeof(Settings))
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
