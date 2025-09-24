import sys
import os
import threading
import time
import signal
import numbers

# No support on windows
try:
    import readline
except Exception:
    readline = None

import serial
from hactar_scanning import HactarScanning, SelectHactarPort
from hactar_commands import (
    command_map,
    bypass_map,
    ui_command_map,
    net_command_map,
    hactar_command_completer,
    hactar_command_print_matches,
)


class Monitor:
    def __init__(self, port, uart_args, threaded_reading=True):
        self.uart = serial.Serial(port, **uart_args)
        print(f"\033[92mOpened port: {port}, baudrate={uart_args['baudrate']}\033[0m")

        self.running = True
        self.rx_thread = None

        # Start threading
        if threaded_reading:
            self.rx_thread = threading.Thread(target=self.ReadSerial, args=(True,))
            self.rx_thread.daemon = True
            self.rx_thread.start()

        self.WriteSerial()

    def ReadSerial(self, threaded=False):
        data = None
        while self.running and threaded:
            try:
                if self.uart.in_waiting:
                    data = self.uart.readline().decode()
                else:
                    data = None
                    time.sleep(0.05)
                    continue

                if readline and data != None and data != "":
                    saved_input = readline.get_line_buffer()
                    # return to start of line and clear current line
                    sys.stdout.write("\r\033[K")
                    # print log (adds newline)
                    if data[-1] != "\n":
                        data += "\n"
                    print(f"{data}", end="")
                    # reprint prompt + saved user text
                    sys.stdout.write(f"> {saved_input}")
                    sys.stdout.flush()
                else:
                    print(data)
            except:
                pass

    def WriteSerial(self):
        while self.running:
            usr_input = input("> ")
            if usr_input.lower() == "exit":
                self.running = False
            else:
                split = usr_input.split()
                if split[0].lower() in command_map or usr_input.lower() in command_map:
                    self.uart.write(command_map[usr_input.lower()])
                    continue

                if split[0].lower() in bypass_map:
                    self.ProcessBypassCommand(split)
                    continue

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

        if len(split) - 2 < num_params:
            print("[ERROR] Not enough parameters")
            return

        Header_Bytes = 5  # 1 type, 4 length
        # Create the length of the mgmt TLV and ui TLV

        to_whom_len = Header_Bytes
        command_len = 0
        # transmit the TLV

        collapsed_params = []
        for param in split[2:]:
            to_whom_len += len(param)
            command_len += len(param)
            collapsed_params += param.encode("utf-8")

        data = []
        data += bypass_map[to_whom].to_bytes(1, byteorder="little")

        data += to_whom_len.to_bytes(4, byteorder="little")

        data += command_id.to_bytes(1, byteorder="little")

        data += command_len.to_bytes(4, byteorder="little")

        data += collapsed_params

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

    readline.set_completer(hactar_command_completer)
    readline.set_completion_display_matches_hook(hactar_command_print_matches)
    readline.parse_and_bind("tab: complete")

    monitor = Monitor(port, uart_config)


# TODO
if __name__ == "__main__":
    pass
