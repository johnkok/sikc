# SIKC irrigation workspace

This repository contains a complete local irrigation-control stack built around a KinCony KC868-A16:

- [`kc868-a16-irrigation/`](kc868-a16-irrigation/) — ESP32 firmware for valve, pump, tank, flow-meter, and RS-485 soil-sensor control.
- [`irrigation-android/`](irrigation-android/) — native Android MQTT client for status, manual control, and irrigation-plan editing.

The firmware is the source of truth for safety interlocks, schedules, sensor thresholds, and persistent settings. Home Assistant and the Android application are MQTT clients; neither replaces the controller's local protections.

## Start here

1. Read the firmware [safety and hardware guide](kc868-a16-irrigation/docs/HARDWARE.md).
2. Prepare credentials from `kc868-a16-irrigation/include/secrets.example.h`.
3. Build and initially flash over USB using the [firmware README](kc868-a16-irrigation/README.md).
4. Follow the [commissioning checklist](kc868-a16-irrigation/docs/COMMISSIONING.md).
5. Connect [Home Assistant](kc868-a16-irrigation/docs/HOME_ASSISTANT.md) and, if desired, the Android app.
6. After the first USB flash, use the [Wi-Fi update guide](kc868-a16-irrigation/docs/OTA.md).

Do not connect mains-powered pumps or 24-VAC valves directly to the KC868-A16 outputs. Use suitable interfaces/contactors and independent motor protection.
