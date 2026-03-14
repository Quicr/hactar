#!/usr/bin/env python3
"""
Test script to verify NET config parameters are stored in non-volatile storage.

This script has two parts:
1. Set all NET config parameters to known test values
2. Read back all NET config parameters to verify persistence

Usage:
  # Part 1: Set all values, then power cycle the device
  python test_nvs_persistence.py --set

  # Part 2: After power cycle, read values back to verify persistence
  python test_nvs_persistence.py --get

  # Or run both (for quick testing without power cycle)
  python test_nvs_persistence.py --set --get
"""

import argparse
import os
import sys

# Add utility and vendor paths (same as main.py)
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "utility"))
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "vendor"))
import json
import signal
import struct
import time

import serial
from hactar_scanning import SelectHactarPort, ResetDevice
from hactar_commands import (
    bypass_map,
    net_command_map,
    Reply_Ack,
    Reply_Nack,
    encode_namespace,
    hactar_get_ack,
)

# =============================================================================
# TEST VALUES - Known values to set for each config parameter
# =============================================================================

TEST_VALUES = {
    "ssid_name": "TestNetwork_NVS",
    "ssid_password": "TestPassword123!",
    "moq_url": "https://test-relay.example.com:4433/moq",
    "language": "es-ES",
    "channel_namespace": ["test", "channel", "nvs", "persistence"],
    "ai_query_namespace": ["ai", "query", "test"],
    "ai_audio_namespace": ["ai", "audio", "response", "test"],
    "ai_cmd_namespace": ["ai", "cmd", "response", "test"],
}


def read_link_response(uart, timeout=2.0):
    """Read a LINK response line from the device.

    Returns the response text (without LINK prefix) or None if timeout.
    """
    start_time = time.time()
    response = b""

    while time.time() - start_time < timeout:
        if uart.in_waiting > 0:
            data = uart.read(uart.in_waiting)
            response += data
            # Check if we got a complete line
            if b"\n" in response:
                break
        else:
            time.sleep(0.05)

    text = response.decode("utf-8", errors="replace").strip()
    return text if text else None


def wait_for_link_ack(uart, timeout=2.0):
    """Wait for ACK or NACK response. Returns True for ACK, False otherwise."""
    response = read_link_response(uart, timeout)
    if response == "ACK":
        return True
    elif response == "NACK":
        return False
    else:
        return False


def send_net_command(uart, command, params=None, encoder=None, wait_for_ack=True):
    """Send a command to the NET chip."""
    if command not in net_command_map:
        print(f"[ERROR] Unknown command: {command}")
        return False

    cmd_info = net_command_map[command]
    command_id = cmd_info["id"]
    num_params = cmd_info["num_params"]

    # Build payload based on encoder type
    payload = bytes()

    if encoder == "language" and params:
        payload = params[0].encode("utf-8")

    elif encoder == "namespace" and params:
        payload = encode_namespace(params[0])

    elif encoder == "ai_namespaces" and params:
        # params should be [query_ns, audio_ns, cmd_ns]
        payload = (
            encode_namespace(params[0])
            + encode_namespace(params[1])
            + encode_namespace(params[2])
        )

    elif params:
        # Default encoding: length-prefixed strings if multiple params
        for param in params:
            if num_params > 1:
                payload += len(param).to_bytes(4, byteorder="little")
            payload += param.encode("utf-8")

    # Build the full TLV packet
    Header_Bytes = 5  # 1 type + 4 length
    to_whom_len = Header_Bytes + len(payload)
    command_len = len(payload)

    data = []
    # MGMT - T (bypass to NET)
    data += bypass_map["net"].to_bytes(1, byteorder="little")
    # MGMT - L
    data += to_whom_len.to_bytes(4, byteorder="little")
    # NET - T (command ID)
    data += command_id.to_bytes(1, byteorder="little")
    # NET - L
    data += command_len.to_bytes(4, byteorder="little")
    # NET - V (payload)
    data += payload

    uart.write(bytes(data))

    if wait_for_ack:
        return wait_for_link_ack(uart, timeout=2.0)
    return True


def read_response(uart, timeout=2.0):
    """Read response data from the device (after sending a get command)."""
    return read_link_response(uart, timeout) or ""


def print_separator(title):
    """Print a section separator."""
    print()
    print("=" * 60)
    print(f"  {title}")
    print("=" * 60)


def set_all_config(uart):
    """Set all NET config parameters to known test values."""
    print_separator("SETTING NET CONFIG PARAMETERS")

    results = {}

    # 1. Set SSID (name and password)
    print(f"\n[1] Setting SSID: {TEST_VALUES['ssid_name']} / {TEST_VALUES['ssid_password']}")
    result = send_net_command(
        uart,
        "set_ssid",
        params=[TEST_VALUES["ssid_name"], TEST_VALUES["ssid_password"]],
    )
    results["set_ssid"] = result
    print(f"    Result: {'OK' if result else 'FAILED'}")

    # 2. Set MOQ URL
    print(f"\n[2] Setting MOQ URL: {TEST_VALUES['moq_url']}")
    result = send_net_command(uart, "set_moq_url", params=[TEST_VALUES["moq_url"]])
    results["set_moq_url"] = result
    print(f"    Result: {'OK' if result else 'FAILED'}")

    # 3. Set Language
    print(f"\n[3] Setting Language: {TEST_VALUES['language']}")
    result = send_net_command(
        uart, "set_language", params=[TEST_VALUES["language"]], encoder="language"
    )
    results["set_language"] = result
    print(f"    Result: {'OK' if result else 'FAILED'}")

    # 4. Set Channel namespace
    print(f"\n[4] Setting Channel: {TEST_VALUES['channel_namespace']}")
    result = send_net_command(
        uart,
        "set_channel",
        params=[TEST_VALUES["channel_namespace"]],
        encoder="namespace",
    )
    results["set_channel"] = result
    print(f"    Result: {'OK' if result else 'FAILED'}")

    # 5. Set AI namespaces
    print(f"\n[5] Setting AI namespaces:")
    print(f"    Query:  {TEST_VALUES['ai_query_namespace']}")
    print(f"    Audio:  {TEST_VALUES['ai_audio_namespace']}")
    print(f"    Cmd:    {TEST_VALUES['ai_cmd_namespace']}")
    result = send_net_command(
        uart,
        "set_ai",
        params=[
            TEST_VALUES["ai_query_namespace"],
            TEST_VALUES["ai_audio_namespace"],
            TEST_VALUES["ai_cmd_namespace"],
        ],
        encoder="ai_namespaces",
    )
    results["set_ai"] = result
    print(f"    Result: {'OK' if result else 'FAILED'}")

    # Summary
    print_separator("SET RESULTS SUMMARY")
    all_passed = all(results.values())
    for cmd, result in results.items():
        status = "PASS" if result else "FAIL"
        print(f"  {cmd}: {status}")
    print()
    print(f"  Overall: {'ALL PASSED' if all_passed else 'SOME FAILED'}")

    return all_passed


def verify_response(name, response, expected):
    """Verify a response matches expected value. Returns True if match."""
    if response is None:
        print(f"    Response: None (timeout)")
        print(f"    Expected: {expected}")
        print(f"    Status: FAIL (no response)")
        return False

    # For string comparisons
    if isinstance(expected, str):
        match = response == expected
        print(f"    Response: {response}")
        print(f"    Expected: {expected}")
        print(f"    Status: {'PASS' if match else 'FAIL'}")
        return match

    # For list comparisons (namespaces returned as JSON)
    if isinstance(expected, list):
        try:
            parsed = json.loads(response)
            match = parsed == expected
            print(f"    Response: {response}")
            print(f"    Expected: {json.dumps(expected)}")
            print(f"    Status: {'PASS' if match else 'FAIL'}")
            return match
        except json.JSONDecodeError:
            print(f"    Response: {response}")
            print(f"    Expected: {json.dumps(expected)}")
            print(f"    Status: FAIL (invalid JSON)")
            return False

    return False


def get_all_config(uart):
    """Read all NET config parameters and verify them."""
    print_separator("READING NET CONFIG PARAMETERS")

    results = {}

    print("\n[1] Getting SSID names...")
    send_net_command(uart, "get_ssid_names", wait_for_ack=False)
    response = read_response(uart)
    # SSID response is comma-separated list, check if our test SSID is included
    ssid_pass = False
    if response:
        ssid_list = response.split(",")
        ssid_pass = TEST_VALUES["ssid_name"] in ssid_list
        print(f"    Response: {response}")
        print(f"    Expected (in list): {TEST_VALUES['ssid_name']}")
        print(f"    Status: {'PASS' if ssid_pass else 'FAIL'}")
    else:
        print(f"    Response: None (timeout)")
        print(f"    Status: FAIL")
    results["ssid_name"] = ssid_pass

    print("\n[2] Getting SSID passwords...")
    send_net_command(uart, "get_ssid_passwords", wait_for_ack=False)
    response = read_response(uart)
    # Password response is comma-separated list, check if our test password is included
    password_pass = False
    if response:
        password_list = response.split(",")
        password_pass = TEST_VALUES["ssid_password"] in password_list
        print(f"    Response: {response}")
        print(f"    Expected (in list): {TEST_VALUES['ssid_password']}")
        print(f"    Status: {'PASS' if password_pass else 'FAIL'}")
    else:
        print(f"    Response: None (timeout)")
        print(f"    Status: FAIL")
    results["ssid_password"] = password_pass

    print("\n[3] Getting MOQ URL...")
    send_net_command(uart, "get_moq_url", wait_for_ack=False)
    response = read_response(uart)
    results["moq_url"] = verify_response("moq_url", response, TEST_VALUES["moq_url"])

    print("\n[4] Getting Language...")
    send_net_command(uart, "get_language", wait_for_ack=False)
    response = read_response(uart)
    results["language"] = verify_response("language", response, TEST_VALUES["language"])

    print("\n[5] Getting Channel namespace...")
    send_net_command(uart, "get_channel", wait_for_ack=False)
    response = read_response(uart)
    results["channel"] = verify_response("channel", response, TEST_VALUES["channel_namespace"])

    print("\n[6] Getting AI namespaces...")
    send_net_command(uart, "get_ai", wait_for_ack=False)
    response = read_response(uart)
    # AI response is JSON: {"query":[...],"audio":[...],"cmd":[...]}
    ai_pass = False
    if response:
        try:
            ai_data = json.loads(response)
            query_match = ai_data.get("query") == TEST_VALUES["ai_query_namespace"]
            audio_match = ai_data.get("audio") == TEST_VALUES["ai_audio_namespace"]
            cmd_match = ai_data.get("cmd") == TEST_VALUES["ai_cmd_namespace"]
            ai_pass = query_match and audio_match and cmd_match
            print(f"    Response: {response}")
            print(f"    Query:  {'PASS' if query_match else 'FAIL'} (expected {TEST_VALUES['ai_query_namespace']})")
            print(f"    Audio:  {'PASS' if audio_match else 'FAIL'} (expected {TEST_VALUES['ai_audio_namespace']})")
            print(f"    Cmd:    {'PASS' if cmd_match else 'FAIL'} (expected {TEST_VALUES['ai_cmd_namespace']})")
        except json.JSONDecodeError:
            print(f"    Response: {response}")
            print(f"    Status: FAIL (invalid JSON)")
    else:
        print(f"    Response: None (timeout)")
        print(f"    Status: FAIL (no response)")
    results["ai"] = ai_pass

    # Summary
    print_separator("GET RESULTS SUMMARY")
    all_passed = all(results.values())
    for name, passed in results.items():
        print(f"  {name}: {'PASS' if passed else 'FAIL'}")
    print()
    print(f"  Overall: {'ALL PASSED' if all_passed else 'SOME FAILED'}")

    return all_passed


def signal_handler(sig, frame):
    print("\n\nInterrupted. Exiting...")
    sys.exit(1)


def main():
    parser = argparse.ArgumentParser(
        description="Test NET config parameter NVS persistence"
    )
    parser.add_argument("--port", "-p", help="Serial port (auto-detect if not specified)")
    parser.add_argument(
        "--set", action="store_true", help="Set all config parameters to test values"
    )
    parser.add_argument(
        "--get", action="store_true", help="Read all config parameters"
    )
    parser.add_argument(
        "--reset", action="store_true", help="Reset device after operations"
    )
    args = parser.parse_args()

    if not args.set and not args.get:
        print("Please specify --set and/or --get")
        print()
        print("Typical usage for NVS persistence testing:")
        print("  1. Run: python test_nvs_persistence.py --set")
        print("  2. Power cycle the device (unplug and replug)")
        print("  3. Run: python test_nvs_persistence.py --get")
        print("  4. Verify the read values match what was set")
        return

    signal.signal(signal.SIGINT, signal_handler)

    uart_args = {
        "baudrate": 115200,
        "bytesize": serial.EIGHTBITS,
        "parity": serial.PARITY_NONE,
        "stopbits": serial.STOPBITS_ONE,
        "timeout": 0.1,
    }

    port = args.port
    if not port:
        port = SelectHactarPort(uart_args)

    if not port:
        print("Error: No port selected")
        return

    print(f"Opening port: {port}")
    uart = serial.Serial(port, **uart_args)

    # Disable logs for cleaner output
    print("Disabling NET logs...")
    send_net_command(uart, "disable_logs")
    time.sleep(0.5)

    if args.set:
        set_all_config(uart)
        time.sleep(0.5)

    if args.get:
        get_all_config(uart)

    if args.reset:
        print("\nResetting device...")
        from hactar_commands import command_map
        uart.write(command_map["reset"])
        time.sleep(0.5)

    uart.close()
    print("\nDone!")


if __name__ == "__main__":
    main()
