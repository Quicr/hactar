#!/usr/bin/env python3
"""
Test script to verify typed response functionality for PR #460.

Exercises all get commands on NET and UI chips and verifies that the
response types match the expected typed response codes.
"""

import argparse
import struct
import sys
import time

import serial

# Add utility directory to path
sys.path.insert(0, "utility")
sys.path.insert(0, "monitor")

from hactar_commands import (
    bypass_map, net_command_map, ui_command_map, Link_Sync_Word,
    Response_Ack, Response_Error, Response_WifiSsids, Response_RelayUrl,
    Response_Loopback, Response_LogsEnabled, Response_Language,
    Response_Channel, Response_AiConfig, Response_SframeKey, Response_StackInfo,
    RESPONSE_TYPE_NAMES, is_data_response
)
from hactar_scanning import ResetDevice


def build_command(chip: str, command: str, payload: bytes = b"") -> bytes:
    """Build a TLV command packet for NET or UI chip."""
    chip_commands = net_command_map if chip == "net" else ui_command_map
    command_id = chip_commands[command]["id"]

    # Inner TLV (to chip)
    inner = Link_Sync_Word
    inner += command_id.to_bytes(2, byteorder="little")
    inner += len(payload).to_bytes(4, byteorder="little")
    inner += payload

    # Outer TLV (to MGMT, routing to chip)
    outer = Link_Sync_Word
    outer += bypass_map[chip].to_bytes(2, byteorder="little")
    outer += len(inner).to_bytes(4, byteorder="little")
    outer += inner

    return bytes(outer)


def read_tlv_response(uart: serial.Serial, timeout: float = 2.0) -> tuple[int, bytes]:
    """
    Read a TLV response from the device.

    Returns (response_type, payload) or (None, None) on timeout/error.
    Skips any log data before the sync word.
    """
    uart.timeout = timeout
    start = time.time()

    # Scan for sync word
    sync_buffer = b""
    while time.time() - start < timeout:
        byte = uart.read(1)
        if not byte:
            continue
        sync_buffer += byte
        if len(sync_buffer) > 4:
            sync_buffer = sync_buffer[-4:]
        if sync_buffer == Link_Sync_Word:
            break
    else:
        return None, None

    # Read header: type (2 bytes) + length (4 bytes)
    header = uart.read(6)
    if len(header) < 6:
        return None, None

    msg_type, msg_len = struct.unpack("<HI", header)

    # Read payload
    payload = b""
    if msg_len > 0:
        payload = uart.read(msg_len)

    return msg_type, payload


def run_test(uart: serial.Serial, chip: str, command: str, expected_type: int) -> bool:
    """
    Run a single test: send command and verify response type.

    Returns True if response type matches expected, False otherwise.
    """
    type_name = RESPONSE_TYPE_NAMES.get(expected_type, f"0x{expected_type:04x}")
    print(f"  {chip} {command} -> expecting {type_name}...", end=" ", flush=True)

    # Flush any pending data
    uart.reset_input_buffer()

    # Send command
    cmd = build_command(chip, command)
    uart.write(cmd)

    # Read response
    resp_type, payload = read_tlv_response(uart)

    if resp_type is None:
        print("\033[91mTIMEOUT\033[0m")
        return False

    actual_name = RESPONSE_TYPE_NAMES.get(resp_type, f"0x{resp_type:04x}")

    if resp_type == expected_type:
        # Show payload summary for data responses
        if is_data_response(resp_type):
            if len(payload) <= 2:
                summary = payload.hex()
            else:
                try:
                    summary = payload.decode("utf-8")[:50]
                    if len(payload) > 50:
                        summary += "..."
                except:
                    summary = payload.hex()[:50] + "..."
            print(f"\033[92mPASS\033[0m [{actual_name}] {summary}")
        else:
            print(f"\033[92mPASS\033[0m [{actual_name}]")
        return True
    else:
        print(f"\033[91mFAIL\033[0m - got {actual_name} (0x{resp_type:04x})")
        return False


def main():
    parser = argparse.ArgumentParser(description="Test typed response functionality")
    parser.add_argument("port", help="Serial port (e.g., /dev/cu.usbserial-110)")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate (default: 115200)")
    parser.add_argument("--no-reset", action="store_true", help="Skip device reset")
    args = parser.parse_args()

    print(f"Connecting to {args.port} at {args.baud} baud...")
    uart = serial.Serial(args.port, args.baud, timeout=1)

    if not args.no_reset:
        ResetDevice(uart, usermode=True)
        time.sleep(0.5)  # Wait for boot

    # Drain any boot messages
    uart.reset_input_buffer()
    time.sleep(0.2)
    uart.reset_input_buffer()

    print("\n=== NET Chip Tests ===")
    net_tests = [
        ("get_wifi", Response_WifiSsids),
        ("get_relay_url", Response_RelayUrl),
        ("get_loopback", Response_Loopback),
        ("get_logs_enabled", Response_LogsEnabled),
        ("get_language", Response_Language),
        ("get_channel", Response_Channel),
        ("get_ai", Response_AiConfig),
    ]

    net_pass = 0
    net_fail = 0
    for cmd, expected in net_tests:
        if run_test(uart, "net", cmd, expected):
            net_pass += 1
        else:
            net_fail += 1
        time.sleep(0.1)  # Small delay between commands

    print("\n=== UI Chip Tests ===")
    ui_tests = [
        ("get_sframe_key", Response_SframeKey),  # May return ERROR if not configured
        ("get_loopback", Response_Loopback),
        ("get_logs_enabled", Response_LogsEnabled),
        ("get_stack_info", Response_StackInfo),
    ]

    ui_pass = 0
    ui_fail = 0
    for cmd, expected in ui_tests:
        # get_sframe_key may return ERROR if no key is set - that's OK
        if cmd == "get_sframe_key":
            print(f"  ui {cmd} -> expecting {RESPONSE_TYPE_NAMES.get(expected)}...", end=" ", flush=True)
            uart.reset_input_buffer()
            uart.write(build_command("ui", cmd))
            resp_type, payload = read_tlv_response(uart)
            if resp_type == expected:
                print(f"\033[92mPASS\033[0m [SFRAME_KEY] {payload.hex()}")
                ui_pass += 1
            elif resp_type == Response_Error:
                print(f"\033[93mSKIP\033[0m [ERROR] - no key configured")
                # Don't count as pass or fail
            else:
                actual = RESPONSE_TYPE_NAMES.get(resp_type, f"0x{resp_type:04x}" if resp_type else "TIMEOUT")
                print(f"\033[91mFAIL\033[0m - got {actual}")
                ui_fail += 1
        else:
            if run_test(uart, "ui", cmd, expected):
                ui_pass += 1
            else:
                ui_fail += 1
        time.sleep(0.1)

    uart.close()

    # Summary
    print("\n=== Summary ===")
    total_pass = net_pass + ui_pass
    total_fail = net_fail + ui_fail
    print(f"NET: {net_pass}/{net_pass + net_fail} passed")
    print(f"UI:  {ui_pass}/{ui_pass + ui_fail} passed")
    print(f"Total: {total_pass}/{total_pass + total_fail} passed")

    if total_fail > 0:
        print("\n\033[91mSome tests failed!\033[0m")
        return 1
    else:
        print("\n\033[92mAll tests passed!\033[0m")
        return 0


if __name__ == "__main__":
    sys.exit(main())
