# Irrigation Control Android app

Native Kotlin/Jetpack Compose application for the KC868-A16 irrigation firmware in the companion package.

## Features

- Live tank, well pump, flow, water total, soil and fault status
- Manual control of four zones and weather blocking
- Four independent weekly schedules with day selection, start time and duration
- Per-zone moisture start/stop hysteresis and maximum runtime
- MQTT configuration stored on the controller, not just the phone
- Persistent 0–100% brightness control for the optional WS2812B status matrices
- Local broker credentials stored in Android private preferences

## Build

Open this directory in Android Studio, allow Gradle sync, then select **Build > Build APK(s)**. Minimum Android version is Android 8 (API 26).

Before using plan editing, flash the updated KC868-A16 firmware from the combined package. Configure the Android app with the same MQTT broker and `garden/irrigation` topic prefix.

For remote access, connect to the home network through WireGuard/Home Assistant rather than exposing MQTT port 1883 publicly.

