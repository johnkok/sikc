# KC868-A16 irrigation and buffer-tank controller

PlatformIO/Arduino firmware for the **original ESP32 KC868-A16**. It controls four irrigation zones, a master valve, tank outlet pump, well-pump contactor, and alarm through suitable external interfaces. It reads two Modbus soil probes, two pulse flow meters, rain, tank floats, and well protection contacts. Status and commands are available through MQTT, automatic Home Assistant discovery, the companion Android app, and password-protected Wi-Fi firmware updates.

## Documentation

- [Implementation and data flow](docs/IMPLEMENTATION.md)
- [Hardware, wiring, and electrical safety](docs/HARDWARE.md)
- [Home Assistant configuration and examples](docs/HOME_ASSISTANT.md)
- [Wi-Fi OTA updates](docs/OTA.md)
- [Commissioning checklist](docs/COMMISSIONING.md)
- [Troubleshooting and fault help](docs/TROUBLESHOOTING.md)

## Safety

- KC868-A16 outputs are low-side MOSFET outputs. Do **not** connect 24 VAC valves or a well motor directly. Use external dry-contact relays/triac interfaces for valves and a correctly rated motor contactor/control panel for the well pump.
- Motor overload, dry-run and upstream electrical protections must work independently of software.
- Confirm that your board is the original KC868-A16, not A16 V3. Verify `include/config.h` against your schematic before use.
- Bench-test with lamps/LEDs and the pump contactor disconnected before operating water equipment.

## I/O map

| Channel | Function |
|---|---|
| IN1 | Irrigation flow pulse |
| IN2 | Rain contact |
| IN3 | Tank EMPTY/low float |
| IN4 | Tank FULL/high float |
| IN5 | Well overload/fault contact |
| IN6 | Well dry-run contact |
| IN7 | Well-fill flow pulse |
| OUT1-4 | Zones 1-4 interface |
| OUT5 | Master valve interface |
| OUT6 | Well-pump contactor command |
| OUT7 | Tank outlet pump contactor command |
| OUT8 | Alarm |

Inputs are assumed active when closed to the A16 input common. Outputs are configured active-low at the PCF8574 electrical layer; the `Board` API uses normal `true = ON` semantics.

## Build

1. Install [PlatformIO](https://platformio.org/) in VS Code or its CLI.
2. Copy `include/secrets.example.h` to `include/secrets.h` and set Wi-Fi, MQTT, and a separate OTA password (minimum eight characters). Never commit this file.
3. Connect the board over USB and run:

   ```sh
   cd kc868-a16-irrigation
   pio run
   pio run --target upload
   pio device monitor
   ```

4. Set the two soil probes to Modbus IDs 1 and 2 at 9600 8N1.
5. Verify the board revision, I/O polarity, and probe register map in `include/config.h` before connecting field loads.

The first upload must use USB. After the board joins Wi-Fi, use `pio run -e kc868-a16-ota -t upload`; see [OTA.md](docs/OTA.md).

## USB-RS485 soil probe utility

`tools/soil_sensor.py` reads a probe or changes its Modbus ID through a USB-RS485 adapter. Install the dependency with `python -m pip install pyserial`. On Linux, list ports with `python -m serial.tools.list_ports`; on Windows, use the adapter `COM` port.

```sh
python tools/soil_sensor.py --port /dev/ttyUSB0 --id 1 read
python tools/soil_sensor.py --port /dev/ttyUSB0 --id 1 set-address 2
```

Read command and firmware use 9600 8N1 and read seven consecutive holding registers from 0x0000: signed temperature /10, moisture /10, EC, pH /10, then nitrogen, phosphorus, and potassium. Check the exact probe manual if its register map differs. The firmware publishes all seven values for each sensor over MQTT/Home Assistant. Connect only one probe while changing its address, then use the new ID for subsequent reads.

## Tank refill sequence

EMPTY stops irrigation, waits `refillDelaySec`, observes `minPumpOffSec`, starts the well contactor, and fills until FULL. Failure to reach FULL within `maxRefillSec`, overload, dry-well, or no fill-flow latches a fault and stops the pump. Clear faults only after the physical fault input clears.

## Known commissioning items

The defaults are conservative examples, not final site settings. Calibrate both pulse-per-liter values, invert any float/input whose contact convention differs, confirm soil Modbus scaling, and measure normal zone/well flows before enabling no-flow and leak thresholds.

## Configuration boundaries

- Compile-time board pins, addresses, timing constants, device identity, and MQTT prefix are in `include/config.h`.
- Network and broker secrets are in ignored `include/secrets.h`.
- Runtime zone plans and system settings are stored in ESP32 NVS and can be changed over MQTT by the Android client.
- The controller remains responsible for interlocks even if Wi-Fi, MQTT, Home Assistant, or the phone is unavailable.

