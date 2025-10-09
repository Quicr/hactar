import sys
import os
import threading
import time
import signal
import numbers
import shlex

import serial
from hactar_scanning import HactarScanning, SelectHactarPort
from hactar_commands import (
    command_map,
    bypass_map,
    ui_command_map,
    net_command_map,
)

running = True


def SendConfiguration(uart, to_whom, command, input_message):
    print(input_message)
    usr_input = input("> ")

    split = shlex.split(usr_input.strip())

    chip_commands = net_command_map
    if to_whom == "ui":
        chip_commands = ui_command_map

    if not command in chip_commands:
        print(f"[ERROR] this shouldn't happen. subcommand {command} is unknown")

    num_params = chip_commands[command]["num_params"]
    command_id = chip_commands[command]["id"]

    if len(split) < num_params:
        print(f"[ERROR] Not enough parameters for command{command} expected {num_params} got {len(split)}")
        return

    if len(split) > num_params:
        print(f"[ERROR] Too many parameters for command {command} expected {num_params} got {len(split)}")
        return

    Header_Bytes = 5  # 1 type, 4 length

    # Create the length of the mgmt TLV and ui TLV
    to_whom_len = Header_Bytes
    command_len = 0

    for param in split[0:]:
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
    for param in split[0:]:
        # If there is > 1 params then we add the size of the param first
        if num_params > 1:
            data += len(param).to_bytes(4, byteorder="little")
            pass

        data += param.encode("utf-8")

    uart.write(bytes(data))

    # TODO Wait for an ok, but logging on ui and net needs to be changed
    # so that we can disable all logs during configuration


def SignalHandler(signal, frame):
    global running
    running = False
    print(f"Signal {signal}, in file {frame.f_code.co_filename} line {frame.f_lineno}")
    exit(-1)


def main(args):
    # Request user to give a uart port we want to configure
    uart_args = {
        "baudrate": 115200,
        "bytesize": serial.EIGHTBITS,
        "parity": serial.PARITY_NONE,
        "stopbits": serial.STOPBITS_ONE,
        "timeout": 0.01,
    }

    signal.signal(signal.SIGINT, SignalHandler)
    port = args.port
    if port == "" or port == None:
        port = SelectHactarPort(uart_args)

    if port == "" or port == None:
        print("Error. No port selected")
        return
    uart = serial.Serial(port, **uart_args)

    # TODO when we need it configure something on ui

    # Configure net ssid
    SendConfiguration(uart, "net", "set_ssid", "Enter ssid name and password ex. My_SSID P@ssw0rd123")
    SendConfiguration(uart, "net", "set_moq_url", "Enter moq url")

    while running:
        try:
            if uart.in_waiting:
                data = uart.readline().decode()
            else:
                data = None
                time.sleep(0.05)
                continue

            print(data, end="")
        except:
            pass


# TODO?
if __name__ == "__main__":
    pass
