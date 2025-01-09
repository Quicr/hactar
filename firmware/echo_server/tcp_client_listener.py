import socket

# Server address and port
server_address = ('127.0.0.1', 12345)

# Create a TCP socket
listener_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
listener_socket.connect(server_address)
print(f"Listener client connected to {server_address}")

try:
    while True:
        # Receive data from the server
        data = listener_socket.recv(1024)  # Buffer size 1024 bytes
        if data:
            print(f"Received message: {data.decode()}")

except KeyboardInterrupt:
    print("\nListener client shutting down...")
finally:
    listener_socket.close()
