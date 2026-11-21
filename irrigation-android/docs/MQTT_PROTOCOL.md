# MQTT protocol

Default prefix: `garden/irrigation`

| Direction | Topic | Payload |
|---|---|---|
| App → board | `cmd/zone1` … `cmd/zone4` | `ON` or `OFF` |
| App → board | `cmd/weather_block` | `ON` or `OFF` |
| App → board | `cmd/clear_faults` | `PRESS` |
| App → board | `cmd/config/zone1` … `cmd/config/zone4` | Plan JSON below |
| App → board | `cmd/config/system` | System JSON, including `display_brightness` from 0–100 |
| Board → app | `state` | Retained state JSON, including `plans` and `display_brightness` |

Plan JSON fields:

```json
{
  "automatic": true,
  "sensor_index": 0,
  "start_moisture": 30.0,
  "stop_moisture": 45.0,
  "max_runtime_min": 30,
  "schedule_enabled": true,
  "hour": 6,
  "minute": 0,
  "duration_min": 10,
  "days": 127
}
```

`days` is a bit mask: bit 0 Sunday through bit 6 Saturday. The firmware validates times and ensures stop moisture is above start moisture before saving the plan to NVS.

Display brightness is persistent and expressed as a percentage. Zero switches the matrices off. The default after a fresh installation is deliberately low:

```json
{
  "display_brightness": 8
}
```

