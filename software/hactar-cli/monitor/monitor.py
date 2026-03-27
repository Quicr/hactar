import numbers
import os
import shlex
import signal
import sys
import threading
import time

# No support on windows
try:
    import readline
except Exception:
    readline = None

import json

import serial
import struct
from hactar_commands import (bypass_map, command_map, hactar_command_completer,
                             hactar_command_print_matches, net_command_map,
                             ui_command_map, encode_command_payload, Link_Sync_Word,
                             Response_Ack, Response_Error, is_data_response,
                             RESPONSE_TYPE_NAMES)
from hactar_scanning import HactarScanning, ResetDevice, SelectHactarPort


class Monitor:
    def __init__(self, port, uart_args, threaded_reading=True):
        self.uart = serial.Serial(port, **uart_args)
        ResetDevice(self.uart, True)
        print(f"\033[92mOpened port: {port}, baudrate={uart_args['baudrate']}\033[0m")

        self.running = True
        self.rx_thread = None

        # Start threading
        if threaded_reading:
            self.rx_thread = threading.Thread(target=self.ReadSerial, args=(True,))
            self.rx_thread.daemon = True
            self.rx_thread.start()

        self.WriteSerial()

    def GetLine(self):
        if self.uart.in_waiting == 0:
            return ""

        data = bytes()
        while self.uart.in_waiting:
            char = self.uart.read()
            if char == b"\x82":
                # print("Got an ack!")
                return ""
            if char == b"\x83":
                # print("Got a nack!")
                return ""

            data += char

            # Check for TLV sync word
            if data[-4:] == Link_Sync_Word:
                return self.ParseTLVResponse(data[:-4])

            if char == b"\n":
                return data.decode()

        # If we got here then that means we didn't get an endline
        data += b"\n"
        data = data.decode()

        return data

    def ParseTLVResponse(self, prefix_data):
        """Parse a TLV response packet after sync word detected."""
        # Print any prefix data (logs before the response)
        if prefix_data:
            try:
                sys.stdout.write(f"\r\033[K{prefix_data.decode()}")
                sys.stdout.flush()
            except Exception:
                pass

        # Read header: type (2 bytes) + length (4 bytes)
        header = self.uart.read(6)
        if len(header) < 6:
            return "[TLV] Incomplete header\n"

        msg_type, msg_len = struct.unpack("<HI", header)

        # Read payload
        payload = bytes()
        if msg_len > 0:
            payload = self.uart.read(msg_len)

        # Format response based on type
        if msg_type == Response_Ack:
            return "\033[92m[ACK]\033[0m\n"
        elif msg_type == Response_Error:
            return "\033[91m[ERROR]\033[0m\n"
        elif is_data_response(msg_type):
            type_name = RESPONSE_TYPE_NAMES.get(msg_type, f"0x{msg_type:04x}")
            # Small payloads (1-2 bytes) are likely numeric, show as hex
            if len(payload) <= 2:
                return f"\033[94m[{type_name}]\033[0m {payload.hex()}\n"
            try:
                return f"\033[94m[{type_name}]\033[0m {payload.decode('utf-8')}\n"
            except Exception:
                return f"\033[94m[{type_name}]\033[0m {payload.hex()}\n"
        else:
            return f"[TLV type=0x{msg_type:04x} len={msg_len}] {payload.hex()}\n"

    def ReadSerial(self, threaded=False):
        while self.running and threaded:
            try:
                data = self.GetLine()

                if data == "":
                    time.sleep(0.05)
                    continue

                sys.stdout.write(f"\r\033[K{data}")
                sys.stdout.flush()

                current_input = readline.get_line_buffer()
                if current_input and "\n" in current_input:
                    current_input = ""

                sys.stdout.write(f"\r\033[K> {current_input}")
                sys.stdout.flush()
            except Exception as ex:
                print(ex)

    def WriteSerial(self):
        while self.running:
            usr_input = input("> ")
            if len(usr_input) == 0:
                continue
            elif usr_input.lower() == "exit":
                self.running = False
            else:
                split = shlex.split(usr_input.strip())
                if usr_input.lower() in command_map:
                    self.uart.write(command_map[usr_input.lower()])
                elif split[0].lower() in command_map:
                    self.uart.write(command_map[split[0].lower()])
                elif split[0].lower() in bypass_map:
                    self.ProcessBypassCommand(split)
                else:
                    print("Unknown command " + usr_input)

    def ProcessBypassCommand(self, split):
        if len(split) < 2:
            print("[ERROR] Not enough parameters to determine sub command")
            return

        to_whom = split[0].lower()
        command = split[1].lower()

        if to_whom == "ui":
            chip_commands = ui_command_map
        else:
            chip_commands = net_command_map

        if not (command in chip_commands):
            print(f"[ERROR] subcommand {command} is unknown")
            return

        num_params = chip_commands[command]["num_params"]
        command_id = chip_commands[command]["id"]
        encoder = chip_commands[command].get("encoder", None)

        if len(split) - 2 < num_params:
            print(
                f"[ERROR] Not enough parameters for command{command} expected {num_params} got {len(split)-2}"
            )
            return

        if len(split) - 2 > num_params:
            print(
                f"[ERROR] Too many parameters for command {command} expected {num_params} got {len(split)-2}"
            )
            return

        # Encode the payload using shared function
        payload, error = encode_command_payload(encoder, split[2:])
        if error:
            print(f"[ERROR] {error}")
            return

        Header_Bytes = 6 + len(Link_Sync_Word)  # 2 type, 4 length
        to_whom_len = Header_Bytes + len(payload)
        command_len = len(payload)

        data = []
        # Sync word 
        data += Link_Sync_Word;
       
        # MGMT - T
        data += bypass_map[to_whom].to_bytes(2, byteorder="little")

        # MGMT - L
        data += to_whom_len.to_bytes(4, byteorder="little")

        # MGMT - V and also UI/NET - STLV
        data += Link_Sync_Word
        data += command_id.to_bytes(2, byteorder="little")

        # UI/NET - L
        data += command_len.to_bytes(4, byteorder="little")

        # UI/NET - V (payload)
        data += payload

        # transmit the TLV
        self.uart.write(bytes(data))

    def Close(self):
        self.running = False
        if self.rx_thread != None:
            self.rx_thread.join()
        self.uart.close()


def SignalHandler(signal, frame):
    print(f"Signal {signal}, in file {frame.f_code.co_filename} line {frame.f_lineno}")
    exit(-1)


def main(args):
    uart_config = {
        "baudrate": 115200,
        "bytesize": serial.EIGHTBITS,
        "parity": serial.PARITY_NONE,
        "stopbits": serial.STOPBITS_ONE,
        "timeout": 0.01,
    }

    signal.signal(signal.SIGINT, SignalHandler)
    port = args.port
    if port == "" or port == None:
        port = SelectHactarPort(uart_config)

    if port == "" or port == None:
        print("Error. No port selected")
        return

    readline.set_completer(hactar_command_completer)
    readline.set_completion_display_matches_hook(hactar_command_print_matches)

    if "libedit" in (readline.__doc__ or ""):
        readline.parse_and_bind("bind ^I rl_complete")
    else:
        readline.parse_and_bind("tab: complete")

    monitor = Monitor(port, uart_config)


# TODO
if __name__ == "__main__":
    pass
