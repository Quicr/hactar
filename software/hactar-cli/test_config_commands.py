#!/usr/bin/env python3
"""Test script to verify configuration commands work properly.

This script tests the set/get commands for language, channel, and AI namespaces.
It requires a connected Hactar device.

Usage:
    python test_config_commands.py [--port PORT]
"""

import argparse
import json
import struct
import sys
import time

import serial

# Add utility to path
sys.path.insert(0, "utility")
from hactar_commands import (
    Link_Sync_Word,
    Response_Ack,
    Response_Nack,
    Response_Data,
    bypass_map,
    net_command_map,
    encode_command_payload,
)
from hactar_scanning import SelectHactarPort, ResetDevice


class ConfigTester:
    def __init__(self, port: str, baudrate: int = 115200):
        self.uart = serial.Serial(
            port,
            baudrate=baudrate,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=2.0,
        )
        ResetDevice(self.uart, True)
        time.sleep(0.5)
        self.passed = 0
        self.failed = 0

    def close(self):
        self.uart.close()

    def send_command(self, command: str, params: list[str] = None) -> tuple[int, bytes]:
        """Send a NET command and return (response_type, response_data)."""
        params = params or []
        cmd_info = net_command_map[command]
        command_id = cmd_info["id"]
        encoder = cmd_info.get("encoder", None)

        payload, error = encode_command_payload(encoder, params)
        if error:
            raise ValueError(f"Encoding error: {error}")

        # Build TLV packet for NET
        Header_Bytes = 6 + len(Link_Sync_Word)  # 2 type, 4 length
        to_whom_len = Header_Bytes + len(payload)
        command_len = len(payload)

        data = bytearray()
        # Sync word
        data += Link_Sync_Word
        # MGMT - T (bypass to NET)
        data += bypass_map["net"].to_bytes(2, byteorder="little")
        # MGMT - L
        data += to_whom_len.to_bytes(4, byteorder="little")
        # Inner sync word + command
        data += Link_Sync_Word
        data += command_id.to_bytes(2, byteorder="little")
        # Command - L
        data += command_len.to_bytes(4, byteorder="little")
        # Command - V (payload)
        data += payload

        # Clear any pending data
        self.uart.reset_input_buffer()

        # Send
        self.uart.write(bytes(data))

        # Read response - look for LINK sync word
        response_data = bytes()
        start_time = time.time()
        buffer = bytes()

        while time.time() - start_time < 2.0:
            if self.uart.in_waiting:
                buffer += self.uart.read(self.uart.in_waiting)

                # Look for LINK sync word in buffer
                sync_pos = buffer.find(Link_Sync_Word)
                if sync_pos >= 0:
                    # Need at least sync(4) + type(2) + length(4) = 10 bytes
                    if len(buffer) >= sync_pos + 10:
                        header_start = sync_pos + 4
                        msg_type, msg_len = struct.unpack(
                            "<HI", buffer[header_start : header_start + 6]
                        )

                        # Wait for full payload
                        total_needed = sync_pos + 10 + msg_len
                        while len(buffer) < total_needed and time.time() - start_time < 2.0:
                            if self.uart.in_waiting:
                                buffer += self.uart.read(self.uart.in_waiting)
                            else:
                                time.sleep(0.01)

                        if len(buffer) >= total_needed:
                            payload_start = header_start + 6
                            response_data = buffer[payload_start : payload_start + msg_len]
                            return msg_type, response_data
            else:
                time.sleep(0.01)

        return None, bytes()

    def test_language(self) -> bool:
        """Test set_language and get_language commands."""
        print("\n--- Testing Language Commands ---")

        test_lang = "es-ES"

        # Set language
        print(f"  Setting language to '{test_lang}'...")
        resp_type, _ = self.send_command("set_language", [test_lang])
        if resp_type != Response_Ack:
            print(f"  FAIL: set_language returned {resp_type}, expected ACK ({Response_Ack})")
            return False
        print("  set_language: ACK received")

        # Get language
        print("  Getting language...")
        resp_type, resp_data = self.send_command("get_language")
        if resp_type != Response_Data:
            print(f"  FAIL: get_language returned {resp_type}, expected DATA ({Response_Data})")
            return False

        received_lang = resp_data.decode("utf-8")
        if received_lang != test_lang:
            print(f"  FAIL: Expected '{test_lang}', got '{received_lang}'")
            return False

        print(f"  get_language: '{received_lang}' - PASS")
        return True

    def test_channel(self) -> bool:
        """Test set_channel and get_channel commands."""
        print("\n--- Testing Channel Commands ---")

        test_ns = ["moq://relay.example.com", "org/test", "channel/demo", "ptt"]
        test_json = json.dumps(test_ns)

        # Set channel
        print(f"  Setting channel namespace...")
        resp_type, _ = self.send_command("set_channel", [test_json])
        if resp_type != Response_Ack:
            print(f"  FAIL: set_channel returned {resp_type}, expected ACK ({Response_Ack})")
            return False
        print("  set_channel: ACK received")

        # Get channel
        print("  Getting channel namespace...")
        resp_type, resp_data = self.send_command("get_channel")
        if resp_type != Response_Data:
            print(f"  FAIL: get_channel returned {resp_type}, expected DATA ({Response_Data})")
            return False

        received_json = resp_data.decode("utf-8")
        try:
            received_ns = json.loads(received_json)
            if received_ns != test_ns:
                print(f"  FAIL: Expected {test_ns}, got {received_ns}")
                return False
        except json.JSONDecodeError as e:
            print(f"  FAIL: Invalid JSON response: {e}")
            return False

        print(f"  get_channel: {received_ns} - PASS")
        return True

    def test_ai(self) -> bool:
        """Test set_ai and get_ai commands."""
        print("\n--- Testing AI Commands ---")

        query_ns = ["moq://ai.example.com", "ai/query"]
        audio_ns = ["moq://ai.example.com", "ai/audio"]
        cmd_ns = ["moq://ai.example.com", "ai/cmd"]

        ai_config = {"query": query_ns, "audio": audio_ns, "cmd": cmd_ns}
        test_json = json.dumps(ai_config)

        # Set AI
        print(f"  Setting AI namespaces...")
        resp_type, _ = self.send_command("set_ai", [test_json])
        if resp_type != Response_Ack:
            print(f"  FAIL: set_ai returned {resp_type}, expected ACK ({Response_Ack})")
            return False
        print("  set_ai: ACK received")

        # Get AI
        print("  Getting AI namespaces...")
        resp_type, resp_data = self.send_command("get_ai")
        if resp_type != Response_Data:
            print(f"  FAIL: get_ai returned {resp_type}, expected DATA ({Response_Data})")
            return False

        received_json = resp_data.decode("utf-8")
        try:
            received_config = json.loads(received_json)
            if received_config.get("query") != query_ns:
                print(f"  FAIL: query mismatch: {received_config.get('query')} != {query_ns}")
                return False
            if received_config.get("audio") != audio_ns:
                print(f"  FAIL: audio mismatch: {received_config.get('audio')} != {audio_ns}")
                return False
            if received_config.get("cmd") != cmd_ns:
                print(f"  FAIL: cmd mismatch: {received_config.get('cmd')} != {cmd_ns}")
                return False
        except json.JSONDecodeError as e:
            print(f"  FAIL: Invalid JSON response: {e}")
            return False

        print(f"  get_ai: {received_config} - PASS")
        return True

    def test_invalid_language(self) -> bool:
        """Test that invalid language is rejected."""
        print("\n--- Testing Invalid Language Rejection ---")

        # The validation happens client-side, so we test the encoder
        from hactar_commands import encode_command_payload

        _, error = encode_command_payload("language", ["invalid-lang"])
        if error is None:
            print("  FAIL: Invalid language should have been rejected")
            return False

        print(f"  Invalid language correctly rejected: {error} - PASS")
        return True

    def test_invalid_json(self) -> bool:
        """Test that invalid JSON is rejected."""
        print("\n--- Testing Invalid JSON Rejection ---")

        from hactar_commands import encode_command_payload

        _, error = encode_command_payload("json", ["not valid json{"])
        if error is None:
            print("  FAIL: Invalid JSON should have been rejected")
            return False

        print(f"  Invalid JSON correctly rejected: {error} - PASS")
        return True

    def run_all_tests(self) -> bool:
        """Run all tests and return overall success."""
        tests = [
            ("Language", self.test_language),
            ("Channel", self.test_channel),
            ("AI", self.test_ai),
            ("Invalid Language", self.test_invalid_language),
            ("Invalid JSON", self.test_invalid_json),
        ]

        print("=" * 50)
        print("Configuration Commands Test Suite")
        print("=" * 50)

        for name, test_func in tests:
            try:
                if test_func():
                    self.passed += 1
                else:
                    self.failed += 1
            except Exception as e:
                print(f"  EXCEPTION in {name}: {e}")
                self.failed += 1

        print("\n" + "=" * 50)
        print(f"Results: {self.passed} passed, {self.failed} failed")
        print("=" * 50)

        return self.failed == 0


def main():
    parser = argparse.ArgumentParser(description="Test Hactar configuration commands")
    parser.add_argument("--port", "-p", help="Serial port (auto-detect if not specified)")
    args = parser.parse_args()

    uart_config = {
        "baudrate": 115200,
        "bytesize": serial.EIGHTBITS,
        "parity": serial.PARITY_NONE,
        "stopbits": serial.STOPBITS_ONE,
        "timeout": 0.1,
    }

    port = args.port
    if not port:
        port = SelectHactarPort(uart_config)

    if not port:
        print("Error: No port selected")
        sys.exit(1)

    print(f"Using port: {port}")

    tester = ConfigTester(port)
    try:
        success = tester.run_all_tests()
        sys.exit(0 if success else 1)
    finally:
        tester.close()


if __name__ == "__main__":
    main()
