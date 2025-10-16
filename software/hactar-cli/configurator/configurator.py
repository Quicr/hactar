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
    hactar_get_ack,
    command_map,
    bypass_map,
    ui_command_map,
    net_command_map,
    Reply_Ack,
    Reply_Nack,
)

running = True
uart = None


def SendCommand(uart, to_whom, command, message="", wait_for_ack=True):
    got_ack = False

    while not got_ack:
        if message != "":
            print(message)

        chip_commands = {}
        if to_whom == "ui":
            chip_commands = ui_command_map
        elif to_whom == "net":
            chip_commands = net_command_map
        elif to_whom == "mgmt":
            if command in command_map:
                uart.write(command_map[command])
                return
            else:
                print(f"ERROR, {command} does not exist in command_map")
                return

        if not command in chip_commands:
            print(f"[ERROR] this shouldn't happen. subcommand {command} is unknown")

        num_params = chip_commands[command]["num_params"]
        command_id = chip_commands[command]["id"]

        split = []
        if num_params > 0:
            usr_input = input("> ")
            split = shlex.split(usr_input.strip())

        if len(split) < num_params:
            print(f"[ERROR] Not enough parameters for command{command} expected {num_params} got {len(split)}")
            continue

        if len(split) > num_params:
            print(f"[ERROR] Too many parameters for command {command} expected {num_params} got {len(split)}")
            continue

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

        attempts = 0
        if wait_for_ack:
            while running and wait_for_ack:
                got_ack = hactar_get_ack(uart, 5)

                if not got_ack:
                    exit(-1)
        else:
            got_ack = True


def SignalHandler(signal, frame):
    global running
    global uart

    running = False

    # Reset the hactar before leaving
    if uart:
        SendCommand(uart, "mgmt", "reset", "Resetting hactar...", wait_for_ack=False)

    print(f"Signal {signal}, in file {frame.f_code.co_filename} line {frame.f_lineno}")
    exit(-1)


def main(args):
    global uart

    # Request user to give a uart port we want to configure
    uart_args = {
        "baudrate": 115200,
        "bytesize": serial.EIGHTBITS,
        "parity": serial.PARITY_NONE,
        "stopbits": serial.STOPBITS_ONE,
        "timeout": 0.1,
    }

    signal.signal(signal.SIGINT, SignalHandler)
    port = args.port
    if port == "" or port == None:
        port = SelectHactarPort(uart_args)

    if port == "" or port == None:
        print("Error. No port selected")
        return
    uart = serial.Serial(port, **uart_args)

    # Silence all logs on UI and Net
    SendCommand(uart, "ui", "disable_logs", "silencing ui logs")
    SendCommand(uart, "net", "disable_logs", "silencing net logs")

    # TODO when we need it configure something on ui
    # SendCommand(uart, "ui", "set_sframe", "Enter sframe key, 16 bytes minimum")

    # Configure net ssid
    SendCommand(uart, "net", "set_ssid", "Enter ssid name and password ex. My_SSID P@ssw0rd123")

    # NOTE! Leave this for last, as the net chip still crashes when
    # destroying a moq session
    SendCommand(uart, "net", "set_moq_url", "Enter moq url")

    SendCommand(uart, "mgmt", "reset", "Resetting hactar...", wait_for_ack=False)
    time.sleep(1)
    print("Goodbye!")


# TODO?
if __name__ == "__main__":
    pass
