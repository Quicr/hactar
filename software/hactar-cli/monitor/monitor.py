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
from hactar_scanning import HactarScanning
from hactar_commands import command_map, bypass_map, ui_command_map, net_command_map


print(command_map["who are you"])


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

        self.uart.write(command_map["disable logs"])

        self.WriteSerial()

    def ReadSerial(self, threaded=False):
        data = None
        while self.running and threaded:
            time.sleep(0.05)

            if self.uart.in_waiting:
                data = self.uart.readline().decode()
            else:
                data = None
                continue

            if readline:
                saved_input = readline.get_line_buffer()
                # return to start of line and clear current line
                sys.stdout.write("\r\033[K")
                # print log (adds newline)
                print(f"{data}", end="")
                # reprint prompt + saved user text
                sys.stdout.write(f"> {saved_input}")
                sys.stdout.flush()
            else:
                print(data)

    def WriteSerial(self):
        while self.running:
            usr_input = input("> ").lower()
            if usr_input == "exit":
                self.running = False
            else:
                split = usr_input.split()
                print("split", split)
                if split[0] in command_map:
                    self.uart.write(command_map[usr_input])
                    continue

                if split[0] in bypass_map:
                    self.ProcessBypassCommand(split)
                    continue
                # for to_whom in bypass_map:
                #     if to_whom in usr_input:
                #         found = True
                #         self.ProcessBypassCommand(
                #             to_whom, usr_input[(len(to_whom) + 1) :]
                #         )
                #         break

                print("Unknown command " + usr_input)

    def ProcessBypassCommand(self, split):
        if len(split) < 2:
            print("[ERROR] Not enough parameters to determine sub command")
            return

        to_whom = split[0]
        command = split[1]

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

        # print(to_whom_len)
        # print(command_len)

        data = []
        data += bypass_map[to_whom].to_bytes(1, byteorder="little")
        # print("comamnd id as bytes", data)

        data += to_whom_len.to_bytes(4, byteorder="little")
        # print("command len as bytes", data)

        data += command_id.to_bytes(1, byteorder="little")
        # print(data)

        data += command_len.to_bytes(4, byteorder="little")
        # print(data)

        data += collapsed_params
        print(data)

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
        "timeout": 2,
    }

    signal.signal(signal.SIGINT, SignalHandler)
    port = args.port
    if port == "" or port == None:
        ports = HactarScanning(uart_config)

        if len(ports) == 0:
            print("No hactars found, exiting")
            return

        idx = -1
        while idx < 0 or idx >= len(ports):
            print(f"Hactars found: {len(ports)}")
            print(f"Select a port [0-{len(ports)-1}]")
            for i, p in enumerate(ports):
                print(f"{i}. {p}")

            idx = input("> ")
            if not idx.isdigit():
                print("Error: not a number entered")
                idx = -1
                continue

            idx = int(idx)

            if idx < 0 or idx >= len(ports):
                print("Invalid selection, try again")
                continue

        port = ports[idx]

    monitor = Monitor(port, uart_config)


if __name__ == "__main__":
    pass
