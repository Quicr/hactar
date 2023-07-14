# This is a testing program to determine if the stm32 mgmt chip could
# keep up with transmitting large quantities of bytes at once.
# Given a large enough buffer of 1024 rx and 2048 tx it can
# handle 1,000,000 bytes without error.


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

running = True

def ReadSerial():
    has_received = False
    while True:
        if uart.in_waiting:
            data = uart.read_all()
            print(data.decode(), end="")

rx_thread = threading.Thread(target=ReadSerial)
rx_thread.daemon = True
rx_thread.start()

send_data = [ch for ch in bytes(command, "UTF-8")]
send_data.append(0)

print(f"Port: {port}, Baud: {baud}, Command: {command}" )
uart.write(bytes(send_data))

# WriteCommandSerial()

while running:
    pass

uart.close()
