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

## PlatformIO upload

The OTA environment is already defined in `platformio.ini`:

```sh
cd kc868-a16-irrigation
pio run -e kc868-a16-ota -t upload --upload-port kc868-a16-irrigation.local
```

Pass the password to PlatformIO's official `espota` uploader without saving it in the repository. In Bash, this avoids placing the password in shell history:

```sh
read -rsp "OTA password: " KC_OTA_PASSWORD; printf '\n'
PLATFORMIO_UPLOAD_FLAGS="--auth=${KC_OTA_PASSWORD}" \
  pio run -e kc868-a16-ota -t upload --upload-port kc868-a16-irrigation.local
unset KC_OTA_PASSWORD
```

`PLATFORMIO_UPLOAD_FLAGS` sets PlatformIO's `upload_flags` option for that process. Do not put a plaintext `--auth` value in version-controlled `platformio.ini`.

If mDNS is unavailable, replace the hostname with the board's DHCP address:

Use the same authenticated command above but replace the final hostname with `192.168.1.50`.

Reserve the address in DHCP so it remains predictable.

## Safe update sequence

Before uploading, stop watering and verify the site is in a safe state. When the board accepts the transfer, firmware maintenance mode switches all zone, master, tank outlet, and well commands off. On success, the ESP32 reboots and normal control resumes. Confirm serial/MQTT availability and firmware behavior after every update.

Do not remove power during flash writing. Keep USB access available as the recovery path for a failed or incompatible image.

## Troubleshooting

- **No OTA-ready serial message:** confirm Wi-Fi association and that `OTA_PASSWORD` exists and has at least eight characters.
- **Hostname not found:** use the DHCP IP; mDNS may be blocked between VLANs or unsupported on the host.
- **Authentication failure:** ensure the local `secrets.h` password matches the running firmware.
- **Connection timeout:** keep the computer and board on routable local networks and allow mDNS/ArduinoOTA traffic through the host firewall.
- **Upload aborts:** the controller intentionally remains in maintenance mode for that boot. Reboot after checking network stability, then retry or recover through USB.
