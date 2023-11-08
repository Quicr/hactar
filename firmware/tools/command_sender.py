import serial
import time
import sys

if (len(sys.argv) < 4):
    print("Error. Need port followed by command")
    exit()

port = sys.argv[1]
baud = sys.argv[2]
command = str(sys.argv[3])

uart = serial.Serial(
    port=port,
    baudrate=baud,
    bytesize=serial.EIGHTBITS,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE
)

send_data = [ch for ch in bytes(command, "UTF-8")]

print(f"Port: {port}, Baud: {baud}, Command: {command}" )
uart.write(bytes(send_data))
uart.close()

time.sleep(1)
