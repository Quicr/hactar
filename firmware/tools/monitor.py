import os
import serial
import threading
import random
import time

port = "/dev/ttyUSB0"
baud = 115200
command = "debug"

uart = serial.Serial(
    port=port,
    baudrate=baud,
    bytesize=serial.EIGHTBITS,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE
)

print(f"\033[92mOpened port: {port}, baudrate={baud}\033[0m")

running = True

def ReadSerial():
    has_received = False
    erase = '\x1b[1A\x1b[2K'
    while True:
        try:
            if uart.in_waiting:
                data = uart.readline()
                print("\r\033[0m" + erase + data.decode(), end="")
                print("\033[1m\033[92mEnter a command:\033[0m")
        except Exception as ex:
            print(ex)

rx_thread = threading.Thread(target=ReadSerial)
rx_thread.daemon = True
rx_thread.start()

def WriteCommand():
    global running
    try:
        user_input = input()
        if (user_input.lower() == "exit"):
            running = False
        else:
            send_data = [ch for ch in bytes(user_input.lower(), "UTF-8")]
            uart.write(bytes(send_data))

    except Exception as ex:
        print(ex)

print("\033[1m\033[92mEnter a command:\033[0m")
while running:
    WriteCommand()

uart.close()
