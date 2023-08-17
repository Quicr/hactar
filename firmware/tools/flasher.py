import os
import serial
import threading
import random
import time
import signal
import sys
# https://www.manualslib.com/download/1764455/St-An3155.html
ACK = 0x79
NACK = 0x1F

COMMANDS = {
    "sync": [ 0x7F ],
    "getid": [ 0x02, 0xFD ]
}

if (len(sys.argv) < 3):
    print("Error. Need port followed by baudrate")
    exit()

def ReadSerial(uart:serial.Serial):
    data = []
    while True:
        try:
            if uart.in_waiting:
                rx = uart.read(1)
                data.append(rx)
                print("read serial")
                print(int.from_bytes(rx, "little"))

                if ((int.from_bytes(rx, "little") == NACK or int.from_bytes(rx, "little") == NACK)and len(data) > 1):
                    # TODO fix, this isn't breaking out correctly
                    break
        except Exception as ex:
            print(ex)

def WriteCommand(uart:serial.Serial, data:bytes, retry=2):
    try:
        # send_data = [int(d, 16) for d in user_input.lower().split(' ')]
        uart.write(bytes(send_data))
    except Exception as ex:
        print(ex)

def SendUploadSelectionCommand(uart:serial.Serial, command:str):
    send_data = [ch for ch in bytes(command, "UTF-8")]
    uart.write(bytes(send_data))

def ProgramSTM():
    port = sys.argv[1]
    baud = sys.argv[2]

    uart = serial.Serial(
        port=port,
        baudrate=baud,
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE
    )

    print(f"\033[92mOpened port: {port}, baudrate={baud}\033[0m")

    # Send the command to the mgmt chip that we want to program the UI chip
    SendUploadSelectionCommand(uart, "ui_upload")
    time.sleep(2)

    # Get the id
    print(bytes(COMMANDS["sync"]))
    uart.write(bytes(COMMANDS["sync"]))
    time.sleep(2)
    print(bytes(COMMANDS["sync"]))
    uart.write(bytes(COMMANDS["sync"]))

    # Read the response
    ReadSerial(uart)

    # Close the uart
    uart.close()

print("Starting")
ProgramSTM()