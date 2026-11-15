# Commissioning checklist

1. Keep all contactor/valve control wiring disconnected. Confirm the exact A16 revision and I2C/RS-485 mapping.
2. Flash firmware and confirm all outputs remain OFF after boot and after Wi-Fi/MQTT loss.
3. Operate each float/contact by hand and observe MQTT state. Correct contact polarity in `config.h` if needed.
4. Connect only the external relay/interface inputs. Test OUT1-OUT8 individually with no loads.
5. Set Modbus addresses 1 and 2 one probe at a time. Confirm moisture, temperature, EC, and pH registers/scales.
6. Flow-calibrate: pass a measured volume, count pulses, then set pulses/liter. Repeat for both meters.
7. Test EMPTY: irrigation must stop. After the refill delay, only the well contactor command may start.
8. Test FULL, overload, dry-run, no-flow, refill timeout, irrigation no-flow, and off-state leak detection.
9. Connect one valve channel and verify the master lead/lag behavior. Add remaining zones afterward.
10. Have the pump power circuit and protective devices checked by a qualified electrician.

