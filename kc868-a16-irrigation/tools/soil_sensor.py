#!/usr/bin/env python3
"""Read or change the Modbus address of a compatible RS485 soil probe.

Defaults for this sensor are 9600 bps, current ID 2, and address register 0x0080.
Sensor variants differ; baud, ID, and address register are configurable.
"""

import argparse
import struct
import sys
import time

try:
    import serial
except ImportError:
    print("Missing dependency: install it with: python -m pip install pyserial", file=sys.stderr)
    raise SystemExit(2)


def crc16(data: bytes) -> int:
    crc = 0xFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            crc = (crc >> 1) ^ (0xA001 if crc & 1 else 0)
    return crc


def make_frame(payload: bytes) -> bytes:
    return payload + struct.pack("<H", crc16(payload))


def transact(port, payload: bytes, expected_min: int = 5) -> bytes:
    port.reset_input_buffer()
    port.write(make_frame(payload))
    port.flush()
    deadline = time.monotonic() + 1.0
    response = bytearray()
    while time.monotonic() < deadline:
        chunk = port.read(max(1, expected_min - len(response)))
        if chunk:
            response.extend(chunk)
            # Read the remainder of a variable-length response using the byte count.
            if len(response) >= 3 and response[1] == 0x03:
                total = 3 + response[2] + 2
                if len(response) >= total:
                    break
            elif len(response) >= 5 and response[1] & 0x80:
                break
            elif len(response) >= expected_min:
                break
    if len(response) < expected_min:
        raise TimeoutError("No complete response. Check wiring, port, baud, and sensor address.")
    frame = bytes(response)
    if crc16(frame[:-2]) != struct.unpack("<H", frame[-2:])[0]:
        raise ValueError(f"Bad Modbus CRC in response: {frame.hex(' ')}")
    if frame[0] != payload[0]:
        raise ValueError(f"Response from unexpected address {frame[0]}: {frame.hex(' ')}")
    if frame[1] & 0x80:
        code = frame[2] if len(frame) > 4 else -1
        raise RuntimeError(f"Modbus exception 0x{code:02X} from sensor")
    return frame


def read_registers(port, address: int, start: int, count: int) -> list[int]:
    request = struct.pack(">BBHH", address, 0x03, start, count)
    response = transact(port, request, expected_min=5 + count * 2)
    if response[1] != 0x03 or response[2] != count * 2:
        raise ValueError(f"Unexpected read response: {response.hex(' ')}")
    return list(struct.unpack(">" + "H" * count, response[3:-2]))


def write_register(port, address: int, register: int, value: int) -> None:
    # Function 0x10 writes one holding register. Its response echoes the
    # starting register and register count, not the value written.
    request = struct.pack(">BBHHBH", address, 0x08, register, 1, 2, value)
    response = transact(port, request, expected_min=8)
    expected = make_frame(struct.pack(">BBHH", address, 0x08, register, 1))
    if response != expected:
        raise ValueError(f"Unexpected write response: {response.hex(' ')}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True, help="USB-RS485 serial port, e.g. /dev/ttyUSB0 or COM3")
    parser.add_argument("--baud", type=int, default=9600, help="Baud rate (default: 9600)")
    parser.add_argument("--id", type=int, default=2, help="Current Modbus sensor ID (default: 2)")
    parser.add_argument("--timeout", type=float, default=0.15, help="Serial read timeout in seconds")
    parser.add_argument("--address-register", type=lambda x: int(x, 0), default=0x0080,
                        help="Register holding the sensor ID (default: 0x0080; check sensor variant)")
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("read", help="Read temperature, moisture, EC, pH, nitrogen, phosphorus, and potassium")
    set_cmd = sub.add_parser("set-address", help="Write a new Modbus ID to the sensor")
    set_cmd.add_argument("new_id", type=int, help="New sensor ID, 1-247")
    args = parser.parse_args()

    if not 1 <= args.id <= 247:
        parser.error("--id must be between 1 and 247")
    if args.command == "set-address" and not 1 <= args.new_id <= 247:
        parser.error("new_id must be between 1 and 247")

    try:
        with serial.Serial(args.port, args.baud, bytesize=8, parity="N", stopbits=1,
                           timeout=args.timeout, write_timeout=1) as port:
            time.sleep(0.1)  # allow USB/RS485 adapters to settle
            if args.command == "read":
                print(f"Address:     {args.id}")

                temperature_raw, moisture, ec, nitrogen, phosphorus, potassium, ph_raw  = read_registers(port, args.id, 0x0000, 7)
                temperature = struct.unpack(">h", struct.pack(">H", temperature_raw))[0] / 10
                print(f"Temperature: {temperature:.1f} °C")
                print(f"Moisture:    {moisture / 10:.1f} %")
                print(f"EC:          {ec} us/cm")
                print(f"pH:          {ph_raw / 10:.1f}")
                print(f"Nitrogen:    {nitrogen} mg/kg")
                print(f"Phosphorus:  {phosphorus} mg/kg")
                print(f"Potassium:   {potassium} mg/kg")
                
                time.sleep(1)

                for add in range(0x080, 0x0100, 0x1):
                    val  = read_registers(port, args.id, add, 1)
                    print(f"val {add}:  {val}")
                    time.sleep(1)
                
            else:
                # Verify the old address answers before making a persistent change.
                read_registers(port, args.id, 0, 1)
                write_register(port, args.id, args.address_register, args.new_id)
                print(f"Address changed from {args.id} to {args.new_id}.")
                print(f"Reconnect using --id {args.new_id} and verify with the read command.")
    except (serial.SerialException, OSError, TimeoutError, ValueError, RuntimeError) as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
