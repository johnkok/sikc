# Home Assistant configuration and help

## Prerequisites

Home Assistant and the controller must reach the same MQTT broker. A common installation uses the Mosquitto broker add-on and Home Assistant's MQTT integration. Create a dedicated broker account for the controller, put those credentials in `include/secrets.h`, and do not expose port 1883 to the internet.

The firmware uses:

| Purpose | Topic |
|---|---|
| Availability | `garden/irrigation/availability` |
| Retained state | `garden/irrigation/state` |
| Commands | `garden/irrigation/cmd/#` |
| Discovery | `homeassistant/<component>/kc868_a16_irrigation/<entity>/config` |

## Automatic discovery

1. Enable MQTT discovery in Home Assistant (it is enabled by default for the MQTT integration).
2. Configure the same broker host, port, username, and password in the firmware.
3. Restart the board or MQTT connection.
4. In Home Assistant, open **Settings → Devices & services → MQTT**. The device **Garden Irrigation Controller** should appear with zone switches, weather block, clear-fault button, tank/well/flow and water-total entities, maintenance state, and both soil probes.

Discovery messages are retained, so entities survive a Home Assistant restart. The controller publishes `online`/`offline` availability using an MQTT last will.

## Manual MQTT checks

From a host with Mosquitto clients installed:

```sh
mosquitto_sub -h BROKER_IP -u irrigation -P 'PASSWORD' -v -t 'garden/irrigation/#'
mosquitto_pub -h BROKER_IP -u irrigation -P 'PASSWORD' -t 'garden/irrigation/cmd/zone1' -m 'ON'
mosquitto_pub -h BROKER_IP -u irrigation -P 'PASSWORD' -t 'garden/irrigation/cmd/zone1' -m 'OFF'
```

A command can be accepted by MQTT but rejected by the controller when a fault, rain, empty tank, refill, weather block, or OTA maintenance mode is active.

## Suggested dashboard card

After discovery, Home Assistant assigns entity IDs from the names. Confirm the actual IDs in **Developer tools → States**, then adapt this example:

```yaml
type: entities
title: Garden irrigation
entities:
  - switch.garden_irrigation_controller_zone_1
  - switch.garden_irrigation_controller_zone_2
  - switch.garden_irrigation_controller_zone_3
  - switch.garden_irrigation_controller_zone_4
  - switch.garden_irrigation_controller_weather_block
  - sensor.garden_irrigation_controller_tank_state
  - binary_sensor.garden_irrigation_controller_well_pump
  - sensor.garden_irrigation_controller_irrigation_flow
  - sensor.garden_irrigation_controller_fault_mask
  - button.garden_irrigation_controller_clear_faults
```

## Weather blocking automation

The weather-block switch stops active irrigation immediately and prevents new starts. This example blocks watering when a rain sensor entity is wet; replace the entity ID with yours:

```yaml
alias: Irrigation weather block
triggers:
  - trigger: state
    entity_id: binary_sensor.garden_rain
actions:
  - action: >
      {% if is_state('binary_sensor.garden_rain', 'on') %}
        switch.turn_on
      {% else %}
        switch.turn_off
      {% endif %}
    target:
      entity_id: switch.garden_irrigation_controller_weather_block
mode: restart
```

Never use an online weather service as the only physical rain or pump protection.

## Missing or stale entities

1. Check that the controller appears online in the router and serial log.
2. Subscribe to `garden/irrigation/#` and `homeassistant/#` to verify state and discovery traffic.
3. Confirm MQTT discovery is enabled and the broker ACL lets the controller publish both prefixes.
4. Restart the controller to republish discovery.
5. If `state` arrives but a soil value is `null`, diagnose the corresponding RS-485 probe.

When changing `DEVICE_ID` or discovery entity names, old retained discovery topics may create duplicates. Remove the old MQTT device/entities in Home Assistant or clear only the obsolete retained configuration topics after confirming their exact names.
