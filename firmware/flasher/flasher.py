import argparse
import sys
import glob
import time
import serial
import stm32
import esp32
from ansi_colours import BB, BG, BR, BW, NW

HELLO = bytes("WHO ARE YOU?\0", "utf-8")
HELLO_RES = bytes("HELLO, I AM A HACTAR DEVICE\0", "utf-8")


def SerialPorts(uart_config):
    # Get all ports
    ports = []
    if sys.platform.startswith("win"):
        ports = [f'COM{i}' for i in range(1, 256)]
    elif (sys.platform.startswith('linux') or
          sys.platform.startswith('cygwin')):
        ports = glob.glob('/dev/ttyUSB[0-9]*')
    elif sys.platform.startswith('darwin'):
        ports = glob.glob('/dev/cu.usbserial*')
    else:
        raise EnvironmentError("Unsupported platform")

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
        parser.add_argument("--mgmt_binary_path",
                            help="Path to the mgmt binary",
                            default="")
        parser.add_argument("--ui_binary_path",
                            help="Path to where the ui binary",
                            default="")
        parser.add_argument("--net_build_path",
                            help="Path to where the net binaries can be found",
                            default="")

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
            try:
                uart = serial.Serial(
                    port=port,
                    **uart_config
                )

                print(f"Opened port: {BB}{port}{NW} "
                      f"baudrate: {BG}{args.baud}{NW}")

                if ("mgmt" in args.chip and args.mgmt_binary_path != ""):
                    print(f"{BW}Starting UI Upload{NW}")
                    stm32_flasher = stm32.stm32_flasher(uart)
                    stm32_flasher.ProgramSTM(args.mgmt_binary_path)

                if ("ui" in args.chip and args.ui_binary_path != ""):
                    print(f"{BW}Starting UI Upload{NW}")
                    stm32_flasher = stm32.stm32_flasher(uart)
                    stm32_flasher.ProgramSTM(args.ui_binary_path)

                if ("net" in args.chip and args.net_build_path != ""):
                    print(f"{BW}Starting Net Upload{NW}")
                    esp32_flasher = esp32.esp32_flasher(uart)
                    esp32_flasher.ProgramESP(args.net_build_path)

                print(f"Done flashing {BR}GOODBYE{NW}")

                uart.close()
            except Exception as ex:
                print(f"{BR}{ex}{NW}")
                uart.close()
    except Exception as ex:
        print(f"{BR}{ex}{NW}")


main()
