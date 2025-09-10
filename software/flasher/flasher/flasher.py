import argparse
import sys
import os
import glob
import time
from ansi_colours import BB, BG, BR, BW, BY, NW

# Get pyserial from our local copy
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "vendor"))
import uart_utils
import stm32_uploader
import esp32s3_uploader

import serial
import serial.tools.list_ports

# TODO only allow .bin files

# TODO put this into a command file
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
}


def main():
    try:
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
            "-c",
            "--chip",
            help="Chips that are to be flashed to. "
            "Available values: ui, net, mgmt. "
            "Multiple chips: ui+net, or ui+net+mgmt, etc",
            default="",
            required=True,
        )
        parser.add_argument(
            "-bin",
            "--binary_path",
            help="Path to the binary",
            default="",
            required=False,
        )
        parser.add_argument(
            "-e",
            "--use_external_flasher",
            help="Gets hactar into flashing mode and then exits so a 3rd party flasher can be used",
            default=False,
            required=False,
        )

        args = parser.parse_args()

        if not args.use_external_flasher and args.binary_path == "":
            parser.error(
                "A binary path must be provided if the flasher is not being used to get hactar chips into bootloder modes."
            )

        uart = None

        uart_config = {
            "baudrate": args.baud,
            "bytesize": serial.EIGHTBITS,
            "parity": serial.PARITY_NONE,
            "stopbits": serial.STOPBITS_ONE,
            "timeout": 2,
        }

        ports = []
        if args.port == "":
            # Try to find a hactar
            print("Searching for Hactar devices")
            ports = SerialPorts(uart_config)
        else:
            ports.append(args.port)

        print(f"Uploading to {len(ports)} Hactar devices on ports: {ports}")

        num_attempts = 5
        i = 0
        uploader = None
        for port in ports:
            flashed = False
            while not flashed and i < num_attempts:
                i += 1
                try:
                    uart = serial.Serial(port=port, **uart_config)

                    print(f"Opened port: {BB}{port}{NW} " f"baudrate: {BG}{args.baud}{NW}")

                    uploader = UploaderFactory(uart, args.chip)

                    if args.use_external_flasher:
                        uploader.FlashSelect()
                        flashed = True
                    else:
                        flashed = uploader.FlashFirmware(args.binary_path)

                    print(f"Done Flashing {BR}GOODBYE{NW}")
                    uart.close()
                except Exception as ex:
                    print(f"{BR}[Error]{NW} {ex}, will try again")
                    uart.close()
                    time.sleep(12)
            # End while
    except Exception as ex:
        print(f"{BR}[Error]{NW} {ex}")


def SerialPorts(uart_config):
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
            # Send a message to the serial port
            # If it responds with I AM A HACTAR DEVICE
            # append it.
            s.write(command_map["who are you"])

            # Read the ok reponse and ignore it
            ok = s.read(3)

            resp = s.read(len(HELLO_RES))

            s.close()

            if resp == HELLO_RES:
                print(f"Device on port {BY}{port}{NW} {BG}is{NW} a Hactar!")
                result.append(port)
            else:
                print(f"Device on port {BY}{port}{NW} {BR}not{NW} a Hactar!")
        except (OSError, serial.SerialException):
            pass
    return result


def UploaderFactory(uart: serial.Serial, chip: str):
    chip = chip.lower()
    if chip == "mgmt" or chip == "ui":
        return stm32_uploader.STM32Uploader(uart, chip)
    elif chip == "net":
        return esp32s3_uploader.ESP32S3Uploader(uart, chip)
    else:
        print(f"Unsupported option for chip: {chip}")
        exit()


def RecoverFlashSelection(flasher, chip, recover):
    trying_to_select = True
    while trying_to_select:
        try:
            flasher.uart.timeout = 2
            flasher.uart.parity = serial.PARITY_NONE
            # Timeout period for mgmt
            time.sleep(5)
            FlashSelection(flasher.uart)
            trying_to_select = False
        except Exception as ex:
            if not recover:
                raise ex


def RecoverableEraseMemory(flasher, sectors, chip, recover):
    finished_erasing = False
    while not finished_erasing:
        try:
            finished_erasing = flasher.SendExtendedEraseMemory(sectors, False, True, True)
        except Exception as ex:
            if not recover:
                raise ex
            print(ex)
            print(f"Erase: {BB}Recovery mode{NW}")
            time.sleep(3)
            RecoverFlashSelection(flasher, chip, recover)


def RecoverableFlashMemory(flasher, firmware, chip, recover):
    finished_writing = False
    while not finished_writing:
        try:
            # TODO add a function for getting the start of the address?
            finished_writing = flasher.SendWriteMemory(firmware, flasher.chip_config["usr_start_addr"], recover)
        except Exception as ex:
            if not recover:
                raise ex
            print(ex)
            print(f"Flashing: {BB}Recovery mode{NW}")
            time.sleep(3)
            RecoverFlashSelection(flasher, chip, recover)
