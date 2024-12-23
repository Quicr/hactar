import socket
import time

# Server address and port
server_address = ('127.0.0.1', 12345)

# Create a TCP socket
sender_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sender_socket.connect(server_address)
print(f"Sender client connected to {server_address}")

# Message to send
message = "Hello from the Sender Client!"
count = 0

try:
    while True:
        # Send the message to the server
        print(f"Sending message: {message} {count}")
        packet = (message + " " + str(count)).encode()
        sender_socket.send(packet)
        count += 1

        # Wait for a while before sending the next message
        time.sleep(0.01)

except KeyboardInterrupt:
    print("\nSender client shutting down...")
finally:
    sender_socket.close()
