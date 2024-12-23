import socket
import threading

# Server configuration
server_address = ('192.168.50.20', 12345)  # Server's IP address and port
clients = []  # List to hold client sockets

# Function to handle communication with each client
def handle_client(client_socket):
    try:
        while True:
            # Receive data from the client
            data = client_socket.recv(1024)  # Buffer size 1024 bytes
            if not data:
                break  # Connection closed by the client

            print(f"Received message: {len(data)}")

            # Send data to all other connected clients
            for client in clients:
                if client != client_socket:  # Don't send back to the sender
                    client.send(data)
                    print(f"Forwarded message to {client.getpeername()}")
    except Exception as e:
        print(f"Error with client {client_socket.getpeername()}: {e}")
    finally:
        # Remove the client from the list and close the socket
        clients.remove(client_socket)
        client_socket.close()

# Create a TCP/IP socket
server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_socket.bind(server_address)
server_socket.listen(5)  # Maximum number of queued connections
print(f"Server listening on {server_address}")

try:
    while True:
        # Wait for a connection
        client_socket, client_address = server_socket.accept()
        print(f"New client connected: {client_address}")

        # Add the new client to the list
        clients.append(client_socket)

        # Start a new thread to handle the client
        client_thread = threading.Thread(target=handle_client, args=(client_socket,))
        client_thread.daemon = True  # Ensure threads are killed when the main program exits
        client_thread.start()
except KeyboardInterrupt:
    print("\nServer shutting down...")
finally:
    server_socket.close()
