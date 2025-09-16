import argparse
import sys
import os
import glob
import time
from ansi_colours import BB, BG, BR, BW, BY, NW

import uart_utils
import stm32_uploader
import esp32s3_uploader

import serial
import serial.tools.list_ports
from hactar_scanning import HactarScanning
from hactar_commands import command_map


# TODO only allow .bin files


def main(args):
    try:
        if not args.use_external_flasher and args.binary_path == "":
            parser.error(
                "A binary path must be provided if the flasher is not being used to get hactar chips into bootloder modes."
            )

        uart = None

        uart_config = {
            "baudrate": 115200,
            "bytesize": serial.EIGHTBITS,
            "parity": serial.PARITY_NONE,
            "stopbits": serial.STOPBITS_ONE,
            "timeout": 2,
        }

        ports = []
        if args.port == "":
            # Try to find a hactar
            print("Searching for Hactar devices")
            ports = HactarScanning(uart_config)
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

                    print(f"Opened port: {BB}{port}{NW} " f"baudrate: {BG}{115200}{NW}")

                    uart.write(command_map["disable logs"])
                    while True:
                        byte = uart.read(1)
                        print(byte)

                        if byte == bytes(0):
                            break

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


# TODO
if __name__ == "__main__":
    pass
