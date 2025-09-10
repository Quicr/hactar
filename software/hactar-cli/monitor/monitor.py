import sys
import os
import threading
import time

# No support on windows
try:
    import readline
except Exception:
    readline = None

import serial
from hactar_scanning import HactarScanning
from hactar_commands import command_map, bypass_map


class Monitor:
    def __init__(self, uart):
        self.uart = uart
        self.running = True

    def ReadSerial():
        if readline:
            saved_input = readline.get_line_buffer()
            # return to start of line and clear current line
            sys.stdout.write("\r\033[K")
            # print log (adds newline)
            print(f"[LOG] {msg}")
            # reprint prompt + saved user text
            sys.stdout.write(f"{prompt}{saved}")
            sys.stdout.flush()
        else:
            print("log")

    def WriteSerial():
        pass


def main(args):
    uart_config = {
        "baudrate": 115200,
        "bytesize": serial.EIGHTBITS,
        "parity": serial.PARITY_NONE,
        "stopbits": serial.STOPBITS_ONE,
        "timeout": 2,
    }

    HactarScanning(uart_config)

    # TODO request a number

    monitor = Monitor()


if __name__ == "__main__":
    pass
