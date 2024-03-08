import argparse
import sys
import glob
import time
import serial
import serial.tools.list_ports
import uart_utils
import stm32
import hactar_stm32
import esp32
from ansi_colours import BB, BG, BR, BW, NW

HELLO = bytes("WHO ARE YOU?\0", "utf-8")
HELLO_RES = bytes("HELLO, I AM A HACTAR DEVICE\0", "utf-8")


def SerialPorts(uart_config):
    # Get all ports
    ports = []
    if sys.platform.startswith("win"):
        coms = list(serial.tools.list_ports.comports())
        ports = [port for (port, _, _) in coms]
    elif (sys.platform.startswith('linux') or
          sys.platform.startswith('cygwin')):
        ports = glob.glob('/dev/ttyUSB[0-9]*')
    elif sys.platform.startswith('darwin'):
        ports = glob.glob('/dev/cu.usbserial*')
    else:
        raise EnvironmentError("Unsupported platform")

    print(f"Ports available: {len(ports)} [{ports}]")
    result = []
    for port in ports:
        try:
            s = serial.Serial(**uart_config, port=port)
            # Send a message to the serial port
            # If it responds with I AM A HACTAR DEVICE
            # append it.
            s.write(HELLO)

            resp = s.read(len(HELLO_RES))

            s.close()

            if (resp == HELLO_RES):
                result.append(port)
        except (OSError, serial.SerialException):
            pass
    return result


def main():
    try:
        parser = argparse.ArgumentParser()

        parser.add_argument("-p", "--port",
                            help="COM/Serial Port that Hactar is on",
                            default="",
                            required=False)
        parser.add_argument("-b", "--baud", help="Baudrate to communicate at",
                            default=115200,
                            type=int,
                            required=False)
        parser.add_argument("-c", "--chip",
                            help="Chips that are to be flashed to. "
                                 "Available values: ui, net, mgmt. "
                                 "Multiple chips: ui+net, or ui+net+mgmt, etc",
                                 default="",
                                 required=True)
        parser.add_argument("-bin", "--binary_path",
                            help="Path to the binary",
                            default="",
                            required=True)

        args = parser.parse_args()

        uart = None

        uart_config = {
            "baudrate": args.baud,
            "bytesize": serial.EIGHTBITS,
            "parity": serial.PARITY_NONE,
            "stopbits": serial.STOPBITS_ONE,
            "timeout": 2
        }

        ports = []
        if (args.port == ""):
            # Try to find a hactar
            print("Searching for Hactar devices")
            ports = SerialPorts(uart_config)
            time.sleep(2)
        else:
            ports.append(args.port)

        print(f"Uploading to {len(ports)} Hactar devices on ports: {ports}")

        for port in ports:
            programmed = False
            while not programmed:
                programmed = True
                try:
                    uart = serial.Serial(
                        port=port,
                        **uart_config
                    )

                    print(f"Opened port: {BB}{port}{NW} "
                          f"baudrate: {BG}{args.baud}{NW}")

                    # TODO use oop inheritance
                    if ("mgmt" in args.chip):
                        print(f"{BW}Starting MGMT Upload{NW}")
                        stm32_flasher = stm32.stm32_flasher(uart)
                        programmed = stm32_flasher.ProgramHactarSTM(args.chip,
                                                                    args.binary_path)

                    if ("ui" in args.chip):
                        print(f"{BW}Starting UI Upload{NW}")
                        stm32_flasher = stm32.stm32_flasher(uart)
                        programmed = ProgramHactarSTM(stm32_flasher, args.chip,
                                                      args.binary_path, True)

                    if ("net" in args.chip):
                        print(f"{BW}Starting Net Upload{NW}")
                        esp32_flasher = esp32.esp32_flasher(uart)
                        programmed = esp32_flasher.ProgramESP(
                            args.binary_path)

                    print(f"Done Flashing {BR}GOODBYE{NW}")
                    uart.close()
                except Exception as ex:
                    print(f"{BR}[Error]{NW} {ex}")
                    uart.close()
            # End while
    except Exception as ex:
        print(f"{BR}[Error]{NW} {ex}")


def ProgramHactarSTM(flasher, chip, binary_path, recover):
    uart_utils.FlashSelection(flasher.uart, chip)

    # Get the firmware
    firmware = open(binary_path, "rb").read()

    # Get the size of memory that we need to erase
    sectors = flasher.GetSectorsForFirmware(len(firmware))

    flasher.SendExtendedEraseMemory(sectors, False)

    finished_writing = False
    in_recovery = False
    while (not finished_writing):
        try:
            print("Try to write")
            # TODO add a function for getting the start of the address?
            finished_writing = flasher.SendWriteMemory(
                firmware, flasher.chip_config["usr_start_addr"], in_recovery, 5)
        except Exception as ex:
            if (not recover):
                raise ex
            print(f"Flashing: {BB}Recovery mode{NW}")
            flasher.uart.close()
            flasher.uart.timeout = 5
            flasher.uart.parity = serial.PARITY_NONE
            time.sleep(4)
            flasher.uart.open()
            uart_utils.FlashSelection(flasher.uart, chip)
            in_recovery = True

    if (chip == "mgmt"):
        flasher.SendGo(flasher.chip_config["usr_start_addr"])

    return True


main()
