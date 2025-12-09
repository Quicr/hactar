import sys
import os
import threading
import time
import signal
import numbers
import shlex

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
                    data = self.uart.readline()
                    data = data.decode()
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

        if len(split) - 2 < num_params:
            print(f"[ERROR] Not enough parameters for command{command} expected {num_params} got {len(split)-2}")
            return

        if len(split) - 2 > num_params:
            print(f"[ERROR] Too many parameters for command {command} expected {num_params} got {len(split)-2}")
            return

        Header_Bytes = 5  # 1 type, 4 length

        # Create the length of the mgmt TLV and ui TLV
        to_whom_len = Header_Bytes
        command_len = 0

        for param in split[2:]:
            # Get the total sizes before we can continue
            to_whom_len += len(param)
            command_len += len(param)

            if num_params > 1:
                # Less than two params, we don't need to add data lens for each param
                to_whom_len += 4
                command_len += 4

        data = []
        # MGMT - T
        data += bypass_map[to_whom].to_bytes(1, byteorder="little")

        # MGMT - L
        data += to_whom_len.to_bytes(4, byteorder="little")

        # MGMT - V and also UI/NET - T
        data += command_id.to_bytes(1, byteorder="little")

        # UI/NET - L
        data += command_len.to_bytes(4, byteorder="little")

        # UI/NET - V
        for param in split[2:]:
            # If there is > 1 params then we add the size of the param first
            if num_params > 1:
                data += len(param).to_bytes(4, byteorder="little")
                pass

            data += param.encode("utf-8")

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
    readline.parse_and_bind("tab: complete")

    monitor = Monitor(port, uart_config)


# TODO
if __name__ == "__main__":
    pass
