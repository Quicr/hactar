import os
import serial
import threading
import random
import time
import signal
import sys

if (len(sys.argv) < 3):
    print("Error. Need port followed by baudrate")
    exit()

port = sys.argv[1]
baud = sys.argv[2]
dump = True
dump_file = "dump.txt"

uart = serial.Serial(
    port=port,
    baudrate=baud,
    bytesize=serial.EIGHTBITS,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    timeout=2
)

print(f"\033[92mOpened port: {port}, baudrate={baud}\033[0m")

running = True

def ReadSerial():
    has_received = False
    erase = '\x1b[1A\x1b[2K'
    while running:
        try:
            if uart.in_waiting:
                data = uart.readline().decode()
                if (dump):
                    with open(dump_file, "a") as my_file:
                        my_file.write(data);
                print("\r\033[0m" + erase + data, end="")
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

def Close():
    global rx_thread
    global uart

    rx_thread.join()
    uart.close()

    sys.exit(0)

def SignalHandler(sig, frame):
    global running
    running = False
    Close()

# Set up signal handler
signal.signal(signal.SIGINT, SignalHandler)
# signal.pause()

print("\033[1m\033[92mEnter a command:\033[0m")
while running:
    WriteCommand()

Close()
