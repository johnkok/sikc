# Hardware and wiring

## Supported controller

This pin map targets the original ESP32-based KinCony KC868-A16, not the A16 V3 or another similarly named revision. Before power-up, compare the PCB markings and schematic with `include/config.h`:

- I2C SDA GPIO 4, SCL GPIO 5
- input PCF8574 addresses `0x21`, `0x22`
- output PCF8574 addresses `0x24`, `0x25`
- RS-485 RX GPIO 16, TX GPIO 13, 9600 8N1
- WS2812B display data GPIO 32 (HT1), two 8x8 matrices daisy-chained
- active-low expander inputs and outputs

If any item differs, adapt the configuration and bench-test before attaching valves or contactors.

## Optional subsystem switches

`include/config.h` provides four compile-time switches, all enabled by default:

| Switch | Effect when `false` |
|---|---|
| `ENABLE_RAIN_SENSOR` | IN2 is ignored as a watering block. The MQTT weather block still works. |
| `ENABLE_FLOW_SENSORS` | IN1/IN7 pulses, water totals, flow readings, no-flow faults, and leak faults are disabled. |
| `ENABLE_SOIL_SENSORS` | RS-485 polling and moisture-based automatic start/stop are disabled. Manual and scheduled zones still work. |
| `ENABLE_WELL_MANAGEMENT` | Well overload/dry-run monitoring and automatic refill are disabled, and OUT6 remains off. Tank EMPTY/FULL protection remains active. |

Change only the required values to `false`, rebuild, and recommission the affected inputs and safety behavior. Disabling a sensor also disables the protection derived from it; do not disable an independent physical motor or electrical protection.

## WS2812B status matrices

The firmware drives 128 pixels as two daisy-chained 8x8 WS2812B modules. Connect GPIO32/HT1 through a 5 V logic-level shifter such as a 74AHCT125 and a 330–470 ohm series resistor to the first module's `DIN`; connect its `DOUT` to the second module's `DIN`. Both modules must share ground with the controller.

Power the matrices from a separate regulated and fused 5 V supply. Do not power them from the A16 sensor connector. Two 64-pixel modules can approach 7.7 A at full-white maximum brightness. Feed 5 V and ground to each module, add bulk capacitance near the modules, and size the supply and wiring for the configured maximum. Firmware defaults to 8% brightness, and the Android app can set a persistent value from 0–100%.

The supplied pixel mapping uses progressive row-major ordering, with the second module continuing at pixel 64. `DISPLAY_SERPENTINE_ROWS`, `DISPLAY_FLIP_X`, `DISPLAY_FLIP_Y`, and `DISPLAY_REVERSE_CHAIN` in `include/config.h` adapt the mapping for other panel layouts or mounting orientations without changing display code.

The first panel rotates every two seconds through Wi-Fi, MQTT, rain/weather block, valve/active zone, well/refill, and irrigation/well flow icons. Wi-Fi and MQTT are green when connected and red when disconnected. Rain is green when clear, amber for a manual weather block, and red when the rain input is active. Valve and well icons change color when operating. The two flow droplets are green with measured flow, amber when flow is expected but not yet detected, and dim blue while idle. A fault overrides the rotation with a red X.

The second panel cycles every three seconds through temperature (orange) and moisture (blue) for probes 1 and 2. One or two top-right pixels identify the probe. During OTA, both panels become a progress display; completion shows green checks and an OTA error shows red crosses.

## Field I/O assignment

| A16 terminal | Connected function | Expected signal |
|---|---|---|
| IN1 | Irrigation flow meter | Dry-contact/open-collector pulse suitable for A16 input |
| IN2 | Rain switch | Closed means rain/block |
| IN3 | Tank low/EMPTY float | Closed means empty |
| IN4 | Tank high/FULL float | Closed means full |
| IN5 | Well overload/fault | Closed means fault |
| IN6 | Well dry-run protection | Closed means dry/fault |
| IN7 | Well-fill flow meter | Dry-contact/open-collector pulse suitable for A16 input |
| OUT1–OUT4 | Zone 1–4 interfaces | Low-voltage command only |
| OUT5 | Master-valve interface | Low-voltage command only |
| OUT6 | Well contactor interface | Contactor/control-panel command only |
| OUT7 | Tank outlet pump interface | Contactor/control-panel command only |
| OUT8 | Alarm interface | Low-voltage command only |

Unused inputs and OUT9–OUT16 are reserved. The firmware's `Board` API uses logical `true = ON`; it hides the active-low PCF8574 electrical representation.

## Power switching

KC868-A16 outputs are low-side transistor outputs, not dry contacts and not motor outputs. Do not connect a 24-VAC solenoid, pump motor, or mains circuit directly. Each field load needs a correctly rated interposing relay, opto-isolated driver, or purpose-designed AC interface. Pump motors require a correctly rated contactor and motor protection/control panel.

Keep these functions independent of firmware:

- branch over-current and earth-leakage protection;
- motor overload and dry-run protection;
- emergency isolation;
- enclosure grounding, ingress protection, and separation of extra-low voltage from mains;
- suppression/snubbing appropriate to relay coils, contactors, and inductive loads.

Have mains and pump wiring designed and checked by a qualified electrician.

## RS-485 soil probes

Wire both probes on one A/B bus with a shared reference as required by their manuals. Use a daisy chain, avoid star stubs, and fit termination/biasing only as the bus hardware and probe documentation require. Power the probes from a supply of the specified voltage; do not assume the A16 can power them.

Each probe needs a unique address. The configured addresses are listed in `SENSOR_IDS` (currently IDs 2 and 3). The expected seven holding registers from `0x0000` are temperature (signed, ÷10), moisture (÷10), EC, pH (÷10), nitrogen, phosphorus, and potassium. Probe vendors vary, so verify addresses, function code, byte order, scaling, and supply requirements against the supplied manual.

Soil timing is configured in `include/config.h`:

- `SOIL_UPDATE_INTERVAL_MS` controls how often each probe receives a complete seven-register read. The two probe reads are staggered evenly within that interval.
- `SOIL_STALE_AFTER_MS` controls how long the last successful value remains valid after Modbus failures.

Keep `SOIL_STALE_AFTER_MS` longer than `SOIL_UPDATE_INTERVAL_MS`. Very short update intervals can overload slow probes or the 9600-baud RS-485 bus; the driver enforces a minimum 250 ms spacing between transactions.

## Float and contact polarity

The default assumes a closed input is active. Choose normally closed contact arrangements where practical so broken wires become visible, but change firmware polarity/logic deliberately to match the actual safety design. Verify every input through MQTT before enabling an output. EMPTY and FULL active together latches a fault.

## Flow-meter calibration

For each meter, pass a measured volume, record pulse count, and compute:

```text
pulses_per_liter = counted_pulses / measured_liters
```

Repeat at low and normal operating flows. Configure realistic no-flow/leak thresholds only after observing a stable installation. The input scan/debounce settings limit the maximum pulse frequency; high-frequency meters may need dedicated ESP32 GPIO interrupt inputs instead of the expander inputs.
