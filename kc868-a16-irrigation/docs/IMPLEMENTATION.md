# Firmware implementation

## Runtime architecture

`src/main.cpp` wires together five cooperative services. There is no RTOS task created by the application; each service performs a small amount of non-blocking work from `loop()`.

| Component | Responsibility |
|---|---|
| `Board` | PCF8574 input scanning/debounce and fail-safe output writes |
| `SoilSensors` | Alternating Modbus reads for probe IDs 1 and 2 |
| `Controller` | Irrigation, refill state machine, schedules, flow calculations, and fault latching |
| `MqttService` | Wi-Fi startup, MQTT connection, Home Assistant discovery, state, and commands |
| `OtaService` | Password-protected Arduino OTA and maintenance-mode shutdown |
| `storage.cpp` | Versioned binary settings in ESP32 NVS/Preferences |

The main loop updates inputs and sensors, transfers the latest soil readings into the controller, converts debounced flow-input edges into pulse counts, runs control logic, services MQTT, and finally services OTA.

## Control behavior

### Irrigation

A zone may start manually, from its weekly schedule, or when its assigned valid soil sensor is at or below `startMoisture`. A start is rejected when any fault is latched, maintenance or weather blocking is active, rain is detected, the tank is empty, or refill is active.

The master valve and tank outlet pump start first. The zone output is asserted after `MASTER_LEAD_MS`. A zone stops at its scheduled duration, maximum runtime, or moisture `stopMoisture`. When the final zone stops, the master and outlet pump remain on for `MASTER_LAG_MS`. Moisture start/stop hysteresis prevents rapid cycling.

### Tank refill

The refill states are idle, delay, and filling. An EMPTY input stops irrigation and enters delay. Once both the configurable refill delay and minimum pump-off time have elapsed, the well contactor output starts. FULL stops the pump and returns to idle. Simultaneous EMPTY and FULL inputs are treated as a sensor fault.

### Flow and faults

Debounced rising edges from IN1 and IN7 are accumulated. Every `FLOW_WINDOW_MS`, pulses are converted to L/min and cumulative liters using the configured pulses-per-liter values. Faults are latched for impossible tank inputs, refill timeout, well overload/dry-run, well no-flow, irrigation no-flow, and off-state leakage.

A latched fault switches irrigation off and energizes the alarm output. Most faults also stop the well command. Fault clearing is refused while a physical well-fault/dry-run input or the impossible float combination remains active.

Fault-mask bits are:

| Value | Meaning |
|---:|---|
| 1 | Tank sensors contradictory |
| 2 | Refill timeout |
| 4 | Well overload contact |
| 8 | Well dry-run contact |
| 16 | Well no-flow |
| 32 | Irrigation no-flow |
| 64 | Flow detected while irrigation is off (leak) |
| 128 | Reserved sensor-failure fault |

If multiple bits are set, add their values. For example, `20` means well overload (`4`) plus well no-flow (`16`).

## Persistence and protocol

`Settings` is stored as a versioned binary blob in the ESP32 `Preferences` namespace `irrigation`. A missing blob, size mismatch, or `NVS_VERSION` mismatch loads safe defaults. Change `NVS_VERSION` when the binary layout changes incompatibly.

The default MQTT prefix is `garden/irrigation`. Commands live below `garden/irrigation/cmd/`; state is retained at `garden/irrigation/state`, and availability is retained at `garden/irrigation/availability`. See the Android client's [MQTT protocol](../../irrigation-android/docs/MQTT_PROTOCOL.md) and the Home Assistant guide for payload examples.

## OTA safety behavior

The OTA service only starts after Wi-Fi connects and only when `OTA_PASSWORD` exists and is at least eight characters. At the beginning of an accepted upload, `Controller::prepareForFirmwareUpdate()` enters maintenance mode and commands every controlled water output off. Normal control cannot restart outputs before the successful OTA reboot. If an upload errors, reboot the board locally after diagnosing the transfer; outputs remain inhibited for the current boot.
