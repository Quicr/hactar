import socket
import threading
import os
import queue
from datetime import datetime

global running
global sock
global port
global msg_queue
global threads
global connections

class InPacket:
    def __init__(self):
        self.Clear();

    def Clear(self):
        self.type = 0;
        self.length = 0;
        self.data = [];

running = True
sock = socket.socket()
port = 60777
msg_queue = queue.Queue()
threads = []
connections = []

# begin server
def BeginServer():
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(("0.0.0.0", port))
    sock.listen(1)

def RunThreads():
    con_thread = threading.Thread(target=HandleConnection)
    con_thread.daemon = True
    con_thread.start()
    threads.append(con_thread)

    # Send main thread to the console
    Console()

def HandleConnection():
    global running
    while running:
        client, addr = sock.accept()
        now = datetime.now().strftime("%H:%M:%S")
        print(str(addr[0]) + ":", str(addr[1]), "connected at", now)

        client_thread = threading.Thread(target=HandleClient,
                                         args=(client, addr))
        client_thread.daemon = True
        client_thread.start()
        connections.append(client)

def HandleClient(client:socket.socket, addr):
    global running
    receive_size = 1
    data = []
    msg_length = None
    while running:
        try:
            # Get bytes 1 at a time until we get three bytes
            msg = client.recv(receive_size)

            if msg:
                for byte in msg:
                    if (msg_length and len(data) == msg_length):
                        break
                    data.append(byte)

                # The first three bytes contain the type and length
                if (len(data) >= 3) and not msg_length:
                    # Now that we have at least the first three bytes we can
                    #  determine how many bytes we expect to come in and then
                    #  respond to the client

                    # Since this is an echo server we are going to generally
                    # ignore the type

                    # Get the first 6 bits
                    msg_type = ((data[0] & 252) >> 2)

                    # Get the first 2 bits of the first byte and 6 bits from
                    # the next
                    msg_id = ((data[0] & 3) << 6) + ((data[1] & 252) >> 2)

                    # Get 10 bits from the next bytes
                    msg_length = ((data[1] & 3) << 6) + data[2] + 3

                    # We want to receive the size of the message
                    receive_size = msg_length

                    print(f"Message type: {msg_type}, id: {msg_id}, length: {msg_length}")

            if (msg_length and len(data) >= msg_length):
                print("Reached end of message")
                data[2] += 6

                data.insert(3, 114)
                data.insert(3, 101)
                data.insert(3, 118)
                data.insert(3, 114)
                data.insert(3, 101)
                data.insert(3, 115)
                # Replace the message with one from the server
                print("Echo back to the client: ", end=" ")
                for num in data:
                    # print("{0:#0{1}x}".format(num,4), end=" ")
                    if num >= 65 and num <= 122:
                        print(chr(num), end=" ")
                    else:
                        print(num, end=" ")
                print("\n")
                client.send(bytes(data))
                data.clear()
                msg_length = None
                connections.remove(client)
                client.close()
                return
        except Exception as err:
            print("Client disconnected:", err)
            for num in data:
                print("{0:#0{1}x}".format(num,4), end=" ")
            print("")
            # print(bytes(data).hex())
            data.clear()
            connections.remove(client)
            client.close()
            return

# Packet format
# 0 1 2 3 4 5 6 7 8 9 1 2 3 4 5 6 ... L
# [    T    ] [        L        ] [ D ]

# For now we only need to worry about one packet
# 8 bit length = 255 max bytes
def BuildPacket(msg:str):
    # Raw packet
    # Start with the type which is 1 byte
    packet = "2"

    # Followed by the data itself
    packet += msg

    # Size of the packet is 1 byte
    # Then the size of the total packet
    packet = str(len(packet)) + packet

    return packet

def Console():
    global running
    while running:
        user_in = input()
        print(user_in)
        if user_in == "exit":
            running = False

if __name__ == "__main__":
    try:
        BeginServer()
        RunThreads()
    except(KeyboardInterrupt, SystemExit):
        os._exit(0)
