import serial
import time
import sys

if (len(sys.argv) < 2):
    print("Error. Need port followed by command")
    exit()

baud = 115200

uart = serial.Serial(
    port=sys.argv[1],
    baudrate=baud,
    bytesize=serial.EIGHTBITS,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE
)

user_input = str(sys.argv[2])

send_data = [ch for ch in bytes(user_input, "UTF-8")]
send_data.append(0)

print(f"Sending command - {user_input}" )
uart.write(bytes(send_data))
uart.close()

time.sleep(2)
