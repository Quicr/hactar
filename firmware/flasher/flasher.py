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
from ansi_colours import BB, BG, BR, BW, BY, NW

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
            s.write(HELLO)

            resp = s.read(len(HELLO_RES))

            s.close()

            if (resp == HELLO_RES):
                print(f"Device on port {BY}{port}{NW} {BG}is{NW} a Hactar!")
                result.append(port)
            else:
                print(f"Device on port {BY}{port}{NW} {BR}not{NW} a Hactar!")
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
        else:
            ports.append(args.port)

        print(f"Uploading to {len(ports)} Hactar devices on ports: {ports}")

        num_attempts = 5
        i = 0
        for port in ports:
            programmed = False
            while not programmed and i < num_attempts:
                # programmed = True
                i += 1
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
                        programmed = ProgramHactarSTM(stm32_flasher,
                                                        args.chip,
                                                        args.binary_path, False)

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
                    print(f"{BR}[Error]{NW} {ex}, will try again")
                    uart.close()
                    time.sleep(3)
            # End while
    except Exception as ex:
        print(f"{BR}[Error]{NW} {ex}")


def RecoverFlashSelection(flasher, chip, recover):
    trying_to_select = True
    while trying_to_select:
        try:
            flasher.uart.close()
            flasher.uart.timeout = 2
            flasher.uart.parity = serial.PARITY_NONE
            # Timeout period for mgmt
            time.sleep(5)
            flasher.uart.open()
            uart_utils.FlashSelection(flasher.uart, chip)
            trying_to_select = False
        except Exception as ex:
            if not recover:
                raise ex


def RecoverableEraseMemory(flasher, sectors, chip, recover):
    finished_erasing = False
    while (not finished_erasing):
        try:
            finished_erasing = flasher.SendExtendedEraseMemory(
                sectors, False, True, True)
        except Exception as ex:
            if (not recover):
                raise ex
            print(ex)
            print(f"Erase: {BB}Recovery mode{NW}")
            RecoverFlashSelection(flasher, chip, recover)


def RecoverableFlashMemory(flasher, firmware, chip, recover):
    finished_writing = False
    while (not finished_writing):
        try:
            # TODO add a function for getting the start of the address?
            finished_writing = flasher.SendWriteMemory(
                firmware, flasher.chip_config["usr_start_addr"], recover)
        except Exception as ex:
            if (not recover):
                raise ex
            print(ex)
            print(f"Flashing: {BB}Recovery mode{NW}")
            RecoverFlashSelection(flasher, chip, recover)


def ProgramHactarSTM(flasher, chip, binary_path, recover):
    uart_utils.FlashSelection(flasher.uart, chip)

    # Get the firmware
    firmware = open(binary_path, "rb").read()

    # Get the size of memory that we need to erase
    sectors = flasher.GetSectorsForFirmware(len(firmware))

    RecoverableEraseMemory(flasher, sectors, chip, recover)

    RecoverableFlashMemory(flasher, firmware, chip, recover)

    if (chip == "mgmt"):
        flasher.SendGo(flasher.chip_config["usr_start_addr"])

    return True


main()
