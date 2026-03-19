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
from hactar_commands import (bypass_map, command_map, hactar_command_completer,
                             hactar_command_print_matches, net_command_map,
                             ui_command_map, SUPPORTED_LANGUAGES, is_valid_language,
                             encode_namespace, Link_Sync_Word)
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
            if char == b"\n":
                return data.decode()

        # If we got here then that means we didn't get an endline
        data += b"\n"
        data = data.decode()

        return data

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

        # Encode the payload based on the encoder type
        payload = bytes()

        if encoder == "language":
            # Validate language
            lang = split[2]
            if not is_valid_language(lang):
                print(f"[ERROR] Invalid language '{lang}'. Supported: {', '.join(SUPPORTED_LANGUAGES)}")
                return
            payload = lang.encode("utf-8")

        elif encoder == "namespace":
            # Parse JSON array and encode as namespace
            try:
                ns_parts = json.loads(split[2])
                if not isinstance(ns_parts, list):
                    print("[ERROR] Namespace must be a JSON array of strings")
                    return
                payload = encode_namespace(ns_parts)
            except json.JSONDecodeError as e:
                print(f"[ERROR] Invalid JSON: {e}")
                return

        elif encoder == "ai_namespaces":
            # Parse 3 JSON arrays (query, audio_response, cmd_response)
            try:
                query_ns = json.loads(split[2])
                audio_ns = json.loads(split[3])
                cmd_ns = json.loads(split[4])

                if not all(isinstance(ns, list) for ns in [query_ns, audio_ns, cmd_ns]):
                    print("[ERROR] All AI namespaces must be JSON arrays of strings")
                    return

                payload = encode_namespace(query_ns) + encode_namespace(audio_ns) + encode_namespace(cmd_ns)
            except json.JSONDecodeError as e:
                print(f"[ERROR] Invalid JSON: {e}")
                return

        else:
            # Default encoding: length-prefixed strings if multiple params
            for param in split[2:]:
                if num_params > 1:
                    payload += len(param).to_bytes(4, byteorder="little")
                payload += param.encode("utf-8")

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
        # print(data);
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
