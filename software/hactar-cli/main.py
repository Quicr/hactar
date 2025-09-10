#!/usr/bin/env python3
import argparse
import sys
import os

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "flasher"))
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "monitor"))
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "utility"))
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "vendor"))

# from flasher import main

from monitor import main as monitor_main

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        prog="hactar", description="Firmware flashing and serial monitoring tool"
    )

    subparsers = parser.add_subparsers(dest="command", required=True)

    # Flasher command
    flash_parser = subparsers.add_parser("flash", help="Flash firmware to device")
    flash_parser.add_argument("file", help="Path to firmware file")
    flash_parser.add_argument("-p", "--port", help="Serial port", required=True)

    # Monitor command
    monitor_parser = subparsers.add_parser("monitor", help="Open serial monitor")
    monitor_parser.add_argument("-p", "--port", help="Serial port", required=False)
    monitor_parser.add_argument(
        "-b", "--baud", type=int, default=115200, help="Baud rate (default: 115200)"
    )

    args = parser.parse_args()

    if args.command == "flash":
        pass
        # sys.exit(flasher_main(args))
    elif args.command == "monitor":
        sys.exit(monitor_main(args))
    else:
        parser.print_help()
        sys.exit(1)
