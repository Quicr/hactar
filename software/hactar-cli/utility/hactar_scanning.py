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
            s.timeout = 0.5

            # Silence the chattering chips (I'M LOOKING AT YOU ESP32!)
            # Also read and ignore the ok
            s.write(command_map["disable logs"])
            s.read(3)

            # Send a message to the serial port
            # If it responds with I AM A HACTAR DEVICE
            # append it.
            s.write(command_map["who are you"])

            # Read and ignore the ok reponse and ignore it
            s.read(3)

            resp = s.read(len(HELLO_RES))

            s.write(command_map["default logging"])
            s.read(3)
            s.close()

            if resp == HELLO_RES:
                print(f"Device on port {BY}{port}{NW} {BG}is{NW} a Hactar!")
                result.append(port)
            else:
                print(f"Device on port {BY}{port}{NW} {BR}not{NW} a Hactar!")
        except (OSError, serial.SerialException):
            pass
    return result
