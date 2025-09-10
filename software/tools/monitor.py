import os
import serial
import threading
import random
import time
import glob
import signal
import sys

import argparse

running = False
uart = None
rx_thread = None

dump_file = None

command_map = {
    "version": bytes([0, 0, 0]),
    "who are you": bytes([1, 0, 0]),
    "hard reset": bytes([2, 0, 0]),
    "reset": bytes([3, 0, 0]),
    "reset ui": bytes([4, 0, 0]),
    "reset net": bytes([5, 0, 0]),
    "flash ui": bytes([6, 0, 0]),
    "flash net": bytes([7, 0, 0]),
    "enable logs": bytes([8, 0, 0]),
    "enable ui logs": bytes([9, 0, 0]),
    "enable net logs": bytes([10, 0, 0]),
    "disable logs": bytes([11, 0, 0]),
    "disable ui logs": bytes([12, 0, 0]),
    "disable net logs": bytes([13, 0, 0]),
    "default logging": bytes([14, 0, 0]),
}


def FindHactar(uart_config):
    HELLO_RES = bytes("HELLO, I AM A HACTAR DEVICE", "utf-8")

    # Get all ports
    ports = []
    if sys.platform.startswith("win"):
        coms = list(serial.tools.list_ports.comports())
        ports = [port for (port, _, _) in coms]
    elif sys.platform.startswith("linux") or sys.platform.startswith("cygwin"):
        ports = glob.glob("/dev/ttyUSB[0-9]*")
    elif sys.platform.startswith("darwin"):
        ports = glob.glob("/dev/cu.usbserial*")
    else:
        raise EnvironmentError("Unsupported platform")
    ports.sort()

    print(f"Ports available: {len(ports)} [{ports}]")
    result = []
    for port in ports:
        try:
            s = serial.Serial(**uart_config, port=port)
            s.timeout = 0.5

            # Silence the chattering chips (I'M LOOKING AT YOU ESP32!)
            s.write(command_map["disable logs"])
            ok = s.read(3)

            # Send a message to the serial port
            # If it responds with I AM A HACTAR DEVICE
            # append it.
            s.write(command_map["who are you"])

            # Read the ok reponse and ignore it
            ok = s.read(3)

            resp = s.read(len(HELLO_RES))

            s.write(command_map["default logging"])
            s.close()

            if resp == HELLO_RES:
                print(f"Device on port {port} is a Hactar!")
                result.append(port)
            else:
                print(f"Device on port {port} not a Hactar!")
        except (OSError, serial.SerialException):
            pass
    return result


def ReadSerial():
    global uart
    global dump_file
    has_received = False
    erase = "\x1b[1A\x1b[2K"
    while running:
        try:
            if uart.in_waiting:
                data = uart.readline().decode()
                if dump_file:
                    dump_file.write(data)
                # print(data)
                print("\r\033[0m" + erase + data, end="")
                print("\033[1m\033[92mEnter a command:\033[0m")
            else:
                time.sleep(0.05)
        except Exception as ex:
            print(ex)


def WriteCommand():
    global running
    try:
        user_input = input()
        if user_input.lower() == "exit":
            running = False
        else:
            if user_input in command_map:
                print(command_map[user_input])
                uart.write(command_map[user_input])
            # uart.write(bytes([0, 0, 0]))
            # send_data = [ch for ch in bytes(user_input, "UTF-8")]
            # uart.write(bytes(send_data))

    except Exception as ex:
        print(ex)


def Close():
    global rx_thread
    global uart

    rx_thread.join()
    uart.close()

    sys.exit(0)


def SignalHandler(sig, frame):
    global running
    running = False
    Close()


def main():
    try:
        global rx_thread
        global running
        global uart
        global dump_file

        parser = argparse.ArgumentParser()

        parser.add_argument(
            "-p",
            "--port",
            help="COM/Serial port that Hactar is on, leave blank to search for Hactars",
            default="",
            required=False,
        )

        parser.add_argument(
            "-b",
            "--baud",
            help="Baudrate to communicate at",
            default=115200,
            type=int,
            required=False,
        )

        parser.add_argument(
            "--dump_file",
            help="Dump file of incoming logs",
            default=None,
            type=str,
            required=False,
        )

        args = parser.parse_args()

        uart_args = {
            "baudrate": args.baud,
            "bytesize": serial.EIGHTBITS,
            "parity": serial.PARITY_NONE,
            "stopbits": serial.STOPBITS_ONE,
            "timeout": 0.5,
        }

        if args.dump_file:
            dump_file = open(args.dump_file, "a")

        port = args.port
        if port == "":
            ports = FindHactar(uart_args)

            if len(ports) == 0:
                raise Exception("No hactars found")

            idx = -1
            while idx < 0 or idx >= len(ports):
                try:
                    print(f"Hactars found: {len(ports)}")
                    print(f"Select a port [0-{len(ports)-1}]")
                    for i, p in enumerate(ports):
                        print(f"{i}. {p}")
                    idx = int(input("> "))

                    if idx < 0 or idx >= len(ports):
                        print("Invalid selection, try again")

                except:
                    print("Invalid selection, try again")

            port = ports[idx]

        uart = serial.Serial(port=port, **uart_args)

        print(f"\033[92mOpened port: {port}, baudrate={args.baud}\033[0m")

        running = True

        rx_thread = threading.Thread(target=ReadSerial)
        rx_thread.daemon = True
        rx_thread.start()

        # Set up signal handler
        signal.signal(signal.SIGINT, SignalHandler)

        print("\033[1m\033[92mEnter a command:\033[0m")
        while running:
            WriteCommand()

        Close()

    except Exception as ex:
        print(f"[ERROR] {ex}")


if __name__ == "__main__":
    main()
