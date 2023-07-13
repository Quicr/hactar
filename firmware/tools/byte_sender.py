# This is a testing program to determine if the stm32 mgmt chip could
# keep up with transmitting large quantities of bytes at once.
# Given a large enough buffer of 1024 rx and 2048 tx it can
# handle 1,000,000 bytes without error.


import serial
import threading
import random
import time

port = "COM11"
baud = 115200
chunk_size = 16384

uart = serial.Serial(
    port=port,
    baudrate=baud,
    bytesize=serial.EIGHTBITS,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE
)

running = True

num_recv = 0

send_data = []
recv_data = []


def ReadSerial():
    global num_recv
    global send_data
    global recv_data
    has_received = False
    while True:
        while uart.in_waiting:
            data = uart.read_all()
            # print(f"Recieved data {data}")
            num_recv += len(data)
            # print(data)
            recv_data.extend(data)
            print(f"Total received {num_recv}")
            has_received = True
        if has_received:

            # print(f"sent data {send_data}")
            # print(f"recv data {recv_data}")

            if num_recv == len(send_data):
                # print(f"Total received {num_recv}")
                # Compare the data
                is_good = True
                for i in range(num_recv):
                    if (send_data[i] != recv_data[i]):
                        is_good = False
                        print(f"send data{i}={send_data[i]} != recv data{i}={recv_data[i]}")
                        # break
                if (is_good):
                    print("Passed!")
                else:
                    print("Failed!")
            has_received = False


def WriteSerial():
    global running
    global num_recv
    global send_data
    global recv_data
    global chunk_size
    try:
        while True:
            user_input = input()
            num_recv = 0
            send_data = []
            recv_data = []
            value = 0
            for _ in range(int(user_input)):
                value = random.randint(1, 254)
                # value = (value + 1) % 256
                send_data.append(value)
            # send_data = b"Hello, world!"
            # uart.write(send_data)
            num_chunks = len(send_data) // chunk_size

            start = 0
            end = 0
            for _ in range(num_chunks):
                start = end
                end += chunk_size
                chunk = send_data[start:end]
                uart.write(bytes(chunk))
                time.sleep(0.5)

                # print(len(chunk))

            # Send remaining chunk
            chunk = send_data[end:]
            uart.write(bytes(chunk))


            # Send the whole data
            # uart.write(bytes(send_data))
    except KeyboardInterrupt:
        running = False

def WriteCommandSerial():
    global running
    global num_recv
    global send_data
    global recv_data
    global chunk_size
    try:
        while True:
            user_input = input("Enter a command\n> ")
            num_recv = 0
            send_data = []
            recv_data = []
            value = 0

            send_data = [ch for ch in bytes(user_input, "UTF-8")]
            send_data.append(0)

            # send_data = b"Hello, world!"
            # uart.write(send_data)
            num_chunks = len(send_data) // chunk_size

            start = 0
            end = 0
            for _ in range(num_chunks):
                start = end
                end += chunk_size
                chunk = send_data[start:end]
                uart.write(bytes(chunk))
                time.sleep(0.5)

                # print(len(chunk))

            # Send remaining chunk
            chunk = send_data[end:]
            print(chunk)
            uart.write(bytes(chunk))


            # Send the whole data
            # uart.write(bytes(send_data))
    except KeyboardInterrupt:
        running = False


rx_thread = threading.Thread(target=ReadSerial)
rx_thread.daemon = True
rx_thread.start()

WriteCommandSerial()

while running:
    pass

uart.close()
