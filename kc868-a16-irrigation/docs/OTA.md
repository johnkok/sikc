# Updating the KC868-A16 over Wi-Fi

Wireless updating uses ArduinoOTA on the local network. It does not provide an internet update service. Keep it behind a trusted LAN or VPN and do not forward its ports from the router.

## One-time setup

1. Make the first firmware upload over USB.
2. In `include/secrets.h`, define a unique password of at least eight characters:

   ```cpp
   #define OTA_PASSWORD "use-a-long-unique-password"
   ```

3. Upload over USB again, monitor serial output, and wait for:

   ```text
   OTA ready at kc868-a16-irrigation.local (...)
   ```

The password is compiled into the firmware. It protects upload authorization but is not a substitute for an isolated network. Do not reuse the Wi-Fi or MQTT password.

## Build and upload

The normal PlatformIO `espota` upload currently enables its uploader's debug output. That debug output includes the plaintext authentication value, so the one-command `pio run -e kc868-a16-ota -t upload` workflow is not recommended in shared terminals, IDE logs, or CI systems.

Build the image first, then invoke the same Espressif uploader without `--debug`:

```bash
pio run -e kc868-a16-ota

read -rsp "OTA password: " KC_OTA_PASSWORD; printf '\n'
KC_PIO_HOME="${PLATFORMIO_CORE_DIR:-$HOME/.platformio}"
python3 "$KC_PIO_HOME/packages/framework-arduinoespressif32/tools/espota.py" \
  -i kc868-a16-irrigation.local \
  -a "$KC_OTA_PASSWORD" \
  -f .pio/build/kc868-a16-ota/firmware.bin
unset KC_OTA_PASSWORD KC_PIO_HOME
```

The default hostname is `kc868-a16-irrigation.local`. If mDNS is unavailable, replace it after `-i` with the DHCP address printed by the serial monitor and reserve that address in DHCP. The direct uploader avoids printing the password, but the value is briefly visible in the local process arguments; perform uploads only from a trusted computer.

Do not store the password in `platformio.ini`, commit it to the repository, type it directly into a shell-history command, or expose it in an IDE/CI variable dump. If output ever contains a debug `Options` line showing the `auth` value, treat the password as exposed: change `OTA_PASSWORD` in `include/secrets.h` and perform the next upload over USB.

## Safe update sequence

Before uploading, stop watering and verify the site is in a safe state. When the board accepts the transfer, firmware maintenance mode switches all zone, master, tank outlet, and well commands off. On success, the ESP32 reboots and normal control resumes. Confirm serial/MQTT availability and firmware behavior after every update.

Do not remove power during flash writing. Keep USB access available as the recovery path for a failed or incompatible image.

## Troubleshooting

- **No OTA-ready serial message:** confirm Wi-Fi association and that `OTA_PASSWORD` exists and has at least eight characters.
- **Hostname not found:** use the DHCP IP; mDNS may be blocked between VLANs or unsupported on the host.
- **Authentication failure:** ensure the local `secrets.h` password matches the running firmware.
- **Connection timeout:** keep the computer and board on routable local networks and allow ArduinoOTA traffic on port 3232 through the host firewall. A direct TCP probe may be refused while the board is idle because the real exchange begins with a UDP invitation and the board connects back to the uploader.
- **Upload aborts:** the controller intentionally remains in maintenance mode for that boot. Reboot after checking network stability, then retry or recover through USB.
