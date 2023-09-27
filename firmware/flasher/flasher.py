import sys
import argparse
import serial
import stm32
import esp32
from ansi_colours import BB, BG, BR, BW, NW

# TODO serial port prober that will send a hello command to each com port
# until one replies with an ACK + details


def main():
    try:
        parser = argparse.ArgumentParser()

        parser.add_argument("-p", "--port",
                            help="COM/Serial Port that Hactar is on",
                            default="/dev/ttyUSB0",
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

        uart = serial.Serial(
            port=args.port,
            baudrate=args.baud,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=2
        )

        print(f"Opened port: {BB}{args.port}{NW} "
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


main()
