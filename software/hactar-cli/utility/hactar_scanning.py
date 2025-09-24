from hactar_commands import command_map
from ansi_codes import *
import serial
import serial.tools.list_ports
import sys
import glob


def HactarScanning(uart_config):
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
            s.timeout = 0.1

            # Silence the chattering chips (I'M LOOKING AT YOU ESP32!)
            # Also read and ignore the ok
            s.write(command_map["disable logs"])

            while True:
                byte = s.read(1)

                if byte == bytes(0):
                    break

            # Send a message to the serial port
            # If it responds with I AM A HACTAR DEVICE
            # append it.
            s.write(command_map["who are you"])

            # Read and ignore the ok reponse and ignore it
            # 3 for ok\n
            resp = s.read(3 + len(HELLO_RES))
            # Skip the Ok\n
            resp = resp[3:]

            s.write(command_map["default logging"])
            s.close()

            if resp == HELLO_RES:
                print(f"Device on port {BY}{port}{NW} {BG}is{NW} a Hactar!")
                result.append(port)
            else:
                print(f"Device on port {BY}{port}{NW} {BR}not{NW} a Hactar!")
        except (OSError, serial.SerialException):
            pass
    return result


def SelectHactarPort(uart_config):
    ports = HactarScanning(uart_config)

    if len(ports) == 0:
        print("No hactars found, exiting")
        return

    idx = -1
    while idx < 0 or idx >= len(ports):
        print(f"Hactars found: {len(ports)}")
        print(f"Select a port [0-{len(ports)-1}]")
        for i, p in enumerate(ports):
            print(f"{i}. {p}")

        idx = input("> ").strip()
        if not idx.isdigit():
            print("Error: not a number entered")
            idx = -1
            continue

        idx = int(idx)

        if idx < 0 or idx >= len(ports):
            print("Invalid selection, try again")
            continue

    return ports[idx]


def SilentLogs(uart):
    uart.write(command_map["disable logs"])

    # Scan bytes until we get no reply meaning the disable logs completed
    while True:
        byte = s.read(1)

        if byte == bytes(0):
            break
