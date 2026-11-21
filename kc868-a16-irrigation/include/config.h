#pragma once
#include <Arduino.h>

namespace Config
{
    constexpr char DEVICE_ID[] = "kc868_a16_irrigation";
    constexpr char DEVICE_NAME[] = "Garden Irrigation Controller";
    constexpr char OTA_HOSTNAME[] = "kc868-a16-irrigation";
    constexpr char MQTT_PREFIX[] = "garden/irrigation";

    // Optional subsystems. Set a value to false to compile that sensor or
    // management path out of normal operation.
    constexpr bool ENABLE_RAIN_SENSOR = false;
    constexpr bool ENABLE_FLOW_SENSORS = false;
    constexpr bool ENABLE_SOIL_SENSORS = true;
    constexpr bool ENABLE_WELL_MANAGEMENT = false;

    // Original ESP32 KC868-A16. Confirm the board revision before energizing outputs.
    constexpr uint8_t I2C_SDA = 4;
    constexpr uint8_t I2C_SCL = 5;
    constexpr uint8_t INPUT_EXPANDER_1 = 0x21;
    constexpr uint8_t INPUT_EXPANDER_2 = 0x22;
    constexpr uint8_t OUTPUT_EXPANDER_1 = 0x24;
    constexpr uint8_t OUTPUT_EXPANDER_2 = 0x25;
    constexpr bool INPUT_ACTIVE_LOW = true;
    constexpr bool OUTPUT_ACTIVE_LOW = true;

    constexpr uint8_t RS485_RX = 16;
    constexpr uint8_t RS485_TX = 13;
    constexpr uint32_t RS485_BAUD = 9600;

    // Two daisy-chained 8x8 WS2812B matrices on the original A16 HT1 terminal.
    constexpr uint8_t DISPLAY_PIN = 32;
    constexpr uint16_t DISPLAY_PIXEL_COUNT = 128;
    constexpr uint8_t DISPLAY_MATRIX_WIDTH = 8;
    constexpr uint8_t DISPLAY_MATRIX_HEIGHT = 8;
    constexpr uint32_t DISPLAY_REFRESH_MS = 250;
    constexpr uint8_t DISPLAY_DEFAULT_BRIGHTNESS = 8; // percent

    constexpr uint8_t FLOW_IRRIGATION_IN = 0; // IN1
    constexpr uint8_t RAIN_IN = 1;            // IN2
    constexpr uint8_t TANK_EMPTY_IN = 2;      // IN3
    constexpr uint8_t TANK_FULL_IN = 3;       // IN4
    constexpr uint8_t WELL_FAULT_IN = 4;      // IN5
    constexpr uint8_t WELL_DRY_IN = 5;        // IN6
    constexpr uint8_t FLOW_WELL_IN = 6;       // IN7

    constexpr uint8_t ZONE_OUT[4] = {0, 1, 2, 3};
    constexpr uint8_t MASTER_OUT = 4;
    constexpr uint8_t WELL_PUMP_OUT = 5;
    constexpr uint8_t TANK_OUTLET_PUMP_OUT = 6;
    constexpr uint8_t ALARM_OUT = 7;

    constexpr uint8_t SENSOR_IDS[2] = {2, 3};
    constexpr size_t SENSOR_COUNT = sizeof(SENSOR_IDS) / sizeof(SENSOR_IDS[0]);
    constexpr uint16_t REG_SOIL_START = 0x0000;
    constexpr uint8_t SENSOR_REG_COUNT = 7; // temperature, moisture, EC, pH, N, P, K

    constexpr uint32_t INPUT_SCAN_MS = 20;
    constexpr uint32_t INPUT_DEBOUNCE_MS = 100;
    // Each soil probe is refreshed once per SOIL_UPDATE_INTERVAL_MS. Reads are
    // staggered evenly across the two probes to avoid back-to-back Modbus traffic.
    constexpr uint32_t SOIL_UPDATE_INTERVAL_MS = 10000;
    // Keep publishing the last successful reading during short communication
    // failures; mark it unavailable after this timeout.
    constexpr uint32_t SOIL_STALE_AFTER_MS = 60000;
    constexpr uint32_t MQTT_PUBLISH_MS = 10000;
    constexpr uint32_t FLOW_WINDOW_MS = 2000;
    constexpr uint32_t MASTER_LEAD_MS = 2000;
    constexpr uint32_t MASTER_LAG_MS = 2000;
    constexpr uint32_t FLOW_GRACE_MS = 8000;
    constexpr uint32_t NVS_VERSION = 2;
    constexpr char TIMEZONE[] = "EET-2EEST,M3.5.0/3,M10.5.0/4";
}
