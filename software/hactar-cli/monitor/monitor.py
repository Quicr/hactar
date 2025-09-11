import sys
import os
import threading
import time
import signal

# No support on windows
try:
    import readline
except Exception:
    readline = None

import serial
from hactar_scanning import HactarScanning
from hactar_commands import command_map, bypass_map


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
                if usr_input in command_map:
                    self.uart.write(command_map[usr_input])
                    continue

                found = False
                for to_whom in bypass_map:
                    if to_whom in usr_input:
                        print(to_whom)
                        print("yay")
                        found = True

                if found:
                    continue
                print("Unknown command " + usr_input)

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

            idx = int(input("> "))
            if idx < 0 or idx >= len(ports):
                print("Invalid selection, try again")

        port = ports[idx]

    monitor = Monitor(port, uart_config)


if __name__ == "__main__":
    pass
