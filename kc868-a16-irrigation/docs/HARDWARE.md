# Hardware and wiring

## Supported controller

This pin map targets the original ESP32-based KinCony KC868-A16, not the A16 V3 or another similarly named revision. Before power-up, compare the PCB markings and schematic with `include/config.h`:

- I2C SDA GPIO 4, SCL GPIO 5
- input PCF8574 addresses `0x21`, `0x22`
- output PCF8574 addresses `0x24`, `0x25`
- RS-485 RX GPIO 16, TX GPIO 13, 9600 8N1
- active-low expander inputs and outputs

If any item differs, adapt the configuration and bench-test before attaching valves or contactors.

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

Each probe needs a unique address. Firmware defaults are ID 1 and ID 2. The expected seven holding registers from `0x0000` are temperature (signed, ÷10), moisture (÷10), EC, pH (÷10), nitrogen, phosphorus, and potassium. Probe vendors vary, so verify addresses, function code, byte order, scaling, and supply requirements against the supplied manual.

## Float and contact polarity

The default assumes a closed input is active. Choose normally closed contact arrangements where practical so broken wires become visible, but change firmware polarity/logic deliberately to match the actual safety design. Verify every input through MQTT before enabling an output. EMPTY and FULL active together latches a fault.

## Flow-meter calibration

For each meter, pass a measured volume, record pulse count, and compute:

```text
pulses_per_liter = counted_pulses / measured_liters
```

Repeat at low and normal operating flows. Configure realistic no-flow/leak thresholds only after observing a stable installation. The input scan/debounce settings limit the maximum pulse frequency; high-frequency meters may need dedicated ESP32 GPIO interrupt inputs instead of the expander inputs.
