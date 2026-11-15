# Troubleshooting

## Board does not connect

- Check the SSID/password in the local ignored `include/secrets.h`; ESP32 Wi-Fi requires a compatible 2.4-GHz network.
- Verify MQTT host, port, account, broker ACL, and routing.
- Use the 115200-baud USB serial monitor. Control and safety interlocks continue locally without MQTT.

## A zone command does nothing

Inspect `faults`, `tank_state`, weather block, rain input, and current refill state. Starts are deliberately rejected during any fault, rain, EMPTY, refill, weather block, or OTA maintenance. Also verify the output interface separately from the valve power circuit.

## Fault will not clear

The clear command is ignored while well overload, well dry-run, or both tank floats remain active. Correct the physical condition and wiring first. Decode the numeric fault mask with the table in [IMPLEMENTATION.md](IMPLEMENTATION.md).

## Soil value is unavailable

- Test one probe at a time with `tools/soil_sensor.py` and a USB-RS485 adapter.
- Verify unique IDs 1 and 2, 9600 8N1, A/B polarity, power, common reference, and register map.
- A probe becomes invalid after 60 seconds without a successful response; Home Assistant then receives `null` values.

## Flow is zero or inaccurate

Confirm that each pulse changes the appropriate A16 input and that the meter does not exceed the expander scan/debounce rate. Recalibrate pulses per liter with a measured volume. Very short or high-frequency pulses require a direct ESP32 counter input and corresponding firmware/hardware redesign.

## Pump stops or alarm activates

Read the fault mask before clearing it. Check physical overload/dry-run contacts, float states, measured flow, timeout configuration, and plumbing. Never bypass motor protection to make a software fault disappear.

## Recovery after a bad update

Power-cycle and inspect serial output. If Wi-Fi OTA is unavailable or the image cannot boot, connect USB and run `pio run -e kc868-a16 -t upload`. Recommission affected I/O after changes to board configuration or control logic.
