#include "ota.h"
#include "config.h"
#include "secrets.h"
#include <ArduinoOTA.h>
#include <WiFi.h>

void OtaService::begin()
{
#if !defined(OTA_PASSWORD)
  if (!warned_)
  {
    Serial.println("OTA disabled: define OTA_PASSWORD in include/secrets.h");
    warned_ = true;
  }
  return;
#else
  if (strlen(OTA_PASSWORD) < 8)
  {
    if (!warned_)
    {
      Serial.println("OTA disabled: OTA_PASSWORD must contain at least 8 characters");
      warned_ = true;
    }
    return;
  }

  ArduinoOTA.setHostname(Config::OTA_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.onStart([this]()
                     {
    controller_.prepareForFirmwareUpdate();
    Serial.println("OTA update started; all controlled outputs are OFF"); });
  ArduinoOTA.onEnd([]()
                   { Serial.println("\nOTA update complete; rebooting"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        { Serial.printf("OTA progress: %u%%\r", (progress * 100U) / total); });
  ArduinoOTA.onError([](ota_error_t error)
                     { Serial.printf("\nOTA error %u\n", error); });
  ArduinoOTA.begin();
  started_ = true;
  Serial.printf("OTA ready at %s.local (%s)\n", Config::OTA_HOSTNAME, WiFi.localIP().toString().c_str());
#endif
}

void OtaService::update()
{
  if (WiFi.status() != WL_CONNECTED)
    return;
  if (!started_)
    begin();
  if (started_)
    ArduinoOTA.handle();
}
