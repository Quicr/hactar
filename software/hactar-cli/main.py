#!/usr/bin/env python3
import argparse
import sys
import os

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "flasher"))
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "monitor"))
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "configurator"))
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "utility"))
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "vendor"))


from flasher import main as flasher_main
from monitor import main as monitor_main
from configurator import main as configurator_main

if __name__ == "__main__":
    parser = argparse.ArgumentParser(prog="hactar-cli", description="Firmware flashing and serial monitoring tool")

    subparsers = parser.add_subparsers(dest="command", required=True)

    # Flasher command
    flash_parser = subparsers.add_parser("flash", help="Flash firmware to device")
    flash_parser.add_argument(
        "-p",
        "--port",
        help="COM/Serial port that Hactar is on, leave blank to search for Hactars",
        default="",
        required=False,
    )
    flash_parser.add_argument(
        "-b",
        "--baud",
        help="Baudrate to communicate at",
        default=115200,
        type=int,
        required=False,
    )
    flash_parser.add_argument(
        "-c",
        "--chip",
        help="Chips that are to be flashed to. "
        "Available values: ui, net, mgmt. "
        "Multiple chips: ui+net, or ui+net+mgmt, etc",
        default="",
        required=True,
    )
    flash_parser.add_argument(
        "-bin",
        "--binary_path",
        help="Path to the binary",
        default="",
        required=False,
    )
    flash_parser.add_argument(
        "-e",
        "--use_external_flasher",
        help="Gets hactar into flashing mode and then exits so a 3rd party flasher can be used",
        default=False,
        required=False,
    )

    # Monitor command
    monitor_parser = subparsers.add_parser("monitor", help="Open serial monitor")
    monitor_parser.add_argument("-p", "--port", help="Serial port", required=False, default="")
    monitor_parser.add_argument(
        "-b",
        "--baud",
        help="Baudrate to communicate at",
        default=115200,
        type=int,
        required=False,
    )

    # Configurator command
    configurator_parser = subparsers.add_parser("configurator", help="Open serial monitor")
    configurator_parser.add_argument("-p", "--port", help="Serial port", required=False, default="")
    configurator_parser.add_argument(
        "-b",
        "--baud",
        help="Baudrate to communicate at",
        default=115200,
        type=int,
        required=False,
    )

    args = parser.parse_args()

    if args.command == "flash":
        sys.exit(flasher_main(args))
    elif args.command == "monitor":
        sys.exit(monitor_main(args))
    elif args.command == "configurator":
        sys.exit(configurator_main(args))
    else:
        parser.print_help()
        sys.exit(1)
