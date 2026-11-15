---
name: kc868-irrigation
description: Maintain, diagnose, document, and extend this repository's KC868-A16 ESP32 irrigation firmware, MQTT/Home Assistant integration, and companion Android client.
---

# KC868 irrigation workspace

Treat `kc868-a16-irrigation` as the safety-authoritative controller and `irrigation-android` as an MQTT client. Read the relevant project README and focused document before changing behavior.

## Preserve these invariants

- Default and boot-time output state is OFF.
- A fault, tank EMPTY, rain/weather block, refill, or OTA maintenance state must not permit irrigation to start.
- Pump overload and dry-run protection remains independent hardware; never document firmware as a replacement.
- KC868-A16 transistor outputs drive suitable interfaces/contactors, never pumps, mains, or 24-VAC valves directly.
- MQTT and Android loss must not disable local schedules, limits, or interlocks.
- OTA start must place the controller in maintenance mode and command controlled water outputs off.
- Do not commit `include/secrets.h` or put credentials in examples, logs, MQTT captures, or PlatformIO configuration.

## Change routing

- Board mapping, compile-time timing, Modbus IDs, identity: `include/config.h` and `docs/HARDWARE.md`.
- Control state machines and fault behavior: `src/controller.cpp`, `include/controller.h`, and `docs/IMPLEMENTATION.md`.
- MQTT state, commands, discovery: `src/mqtt.cpp`, `irrigation-android/docs/MQTT_PROTOCOL.md`, and `docs/HOME_ASSISTANT.md`.
- Persistent setting layouts: `include/types.h` and `src/storage.cpp`; bump `NVS_VERSION` for incompatible binary-layout changes.
- OTA lifecycle: `src/ota.cpp`, `include/ota.h`, and `docs/OTA.md`.
- Android behavior: `irrigation-android/app/src/main` and its README.

Keep protocol documentation, Home Assistant discovery, Android parsing, and firmware payloads synchronized. Build the PlatformIO environment after firmware changes and the Android debug target after client changes when the required toolchains are available. If a toolchain is absent, report that limitation and still perform static consistency checks.
