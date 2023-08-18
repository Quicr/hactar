import serial
import sys
import time
# https://www.manualslib.com/download/1764455/St-An3155.html
ACK = 0x79
NACK = 0x1F

Sync_Command = bytes([0x7F])
Getid_Commmand = bytes([0x02, 0x02 ^ 0xFF])

B_GREY = "\033[1;30m"
B_RED = "\033[1;31m"
B_GREEN = "\033[1;32m"
B_YELLOW = "\033[1;33m"
B_BLUE = "\033[1;34m"
B_MAGENTA = "\033[1;35m"
B_CYAN = "\033[1;36m"
B_WHITE = "\033[1;37m"

N_GREY = "\033[0;30m"
N_RED = "\033[0;31m"
N_GREEN = "\033[0;32m"
N_YELLOW = "\033[0;33m"
N_BLUE = "\033[0;34m"
N_MAGENTA = "\033[0;35m"
N_CYAN = "\033[0;36m"
N_WHITE = "\033[0;37m"

if (len(sys.argv) < 3):
    print("Error. Need port followed by baudrate")
    exit()


def ReadSerial(uart: serial.Serial, expected_bytes=1):
    data = []
    while True:
        print("read serial")
        try:
            rx = uart.read(1)
            data.append(rx)
            print("read serial")
            print(int.from_bytes(rx, "little"))

            # print(len(data))
            print(int.from_bytes(rx, "little") == NACK)
            print(int.from_bytes(rx, "little") == ACK)
            print(len(data))
            if ((int.from_bytes(rx, "little") == ACK or
                 int.from_bytes(rx, "little") == NACK) and len(data) >=
                    expected_bytes):
                # TODO fix, this isn't breaking out correctly
                print("return data")
                return data
        except Exception as ex:
            print(ex)


def WriteCommand(uart: serial.Serial, data: bytes, retry=2):
    uart.write(bytes(data))


def WaitForBytes(uart: serial.Serial, num_bytes: int):
    """ Sits and waits for a response on the serial line.
        If no reply is received then return an (int)0x00

        Note - Passing num_bytes <= 1 returns an int
             - Passing num_bytes >  1 returns an int array
    """
    rx_byte = uart.read(num_bytes)
    if (len(rx_byte) < 1):
        return 0

    if (num_bytes <= 1):
        return int.from_bytes(rx_byte, "little")

    return [x for x in rx_byte]


def WaitForBytesExcept(uart: serial.Serial, num_bytes: int):
    """ Sits and waits for a response on the serial line
        if no reply is received raise an exception.

        Note - Passing num_bytes <= 1 returns an int
             - Passing num_bytes >  1 returns an int array
    """
    rx_byte = uart.read(num_bytes)
    if (len(rx_byte) < 1):
        raise Exception("Error. No reply")

    if (num_bytes <= 1):
        return int.from_bytes(rx_byte, "little")

    return [x for x in rx_byte]


def SendUploadSelectionCommand(uart: serial.Serial, command: str):
    send_data = [ch for ch in bytes(command, "UTF-8")]
    uart.write(bytes(send_data))
    time.sleep(2)
    if (command == "ui_upload"):
        #  Change to parity even
        # TODO get a reply from the mgmt chip for changing to ui_mode/net_mode
        print(f"Activating UI Upload Mode: {B_GREEN}SUCCESS{N_WHITE}")
        print(f"Update uart to parity: {B_BLUE}EVEN{N_WHITE}")
        uart.close()
        uart.parity = serial.PARITY_EVEN
        uart.open()


def WriteSync(uart: serial.Serial, retry_num: int):
    """ Sends the sync byte 0x7F and waits for an ACK from the device.
    """
    while (retry_num > 0):
        retry_num -= 1
        uart.write(Sync_Command)

        rx_bytes = uart.read(1)
        rx_data = int.from_bytes(rx_bytes, "little")

        if (len(rx_bytes) < 1):
            continue

        # Check if ack or nack
        if (rx_data == ACK):
            print(f"Activating device: {B_GREEN}SUCCESS{N_WHITE}")
            return
        elif (rx_data == NACK):
            break

    print(f"Activating device: {B_RED}FAILED{N_WHITE}")
    raise Exception("Failed to Activate device")


def WriteGetID(uart: serial.Serial, retry_num: int = 5):
    while (retry_num > 0):
        retry_num -= 1
        # print(f"Write GetID {Getid_Commmand}")
        uart.write(Getid_Commmand)

        res = WaitForBytes(uart, 1)

        if (res == 0):
            continue

        # Received an ACK break out
        if res == ACK:
            break
        elif res == NACK:
            raise Exception("NACK was received during GetID")

    # Failed to get a reply
    if (retry_num == 0):
        raise Exception("Timeout occurred during GetID")

    # Read the next byte which should be the num of bytes - 1
    #  for some reason they minus off one even tho 2 bytes come in
    num_bytes = WaitForBytesExcept(uart, 1) + 1

    # Get the pid which should be 2 bytes
    pid = WaitForBytesExcept(uart, num_bytes)

    # Wait for an ack
    res = WaitForBytesExcept(uart, 1)

    if (res == NACK):
        raise Exception("A NACK was received at the end of GetID")

    return pid


def WriteRead(uart: serial.Serial, data: bytes, retry_num: int):
    # reply = []
    while (retry_num > 0):
        retry_num -= 1
        print(f"Send data: {data}")
        uart.write(bytes(data))

        # Listen
        rx = uart.read(1)
        print(rx)

        if (len(rx) < 1):
            continue

        # if ()


def ProgramSTM():
    port = sys.argv[1]
    baud = sys.argv[2]

    uart = serial.Serial(
        port=port,
        baudrate=baud,
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        timeout=2
    )

    print(f"\033[92mOpened port: {port}, baudrate={baud}\033[0m")

    # Send the command to the mgmt chip that we want to program the UI chip
    SendUploadSelectionCommand(uart, "ui_upload")
    # TODO wait for reply from mgmt chip

    # Read the response
    WriteSync(uart, 5)

    # lol nice f(g(h(b((x)))
    mc_id = hex(int.from_bytes(bytes(WriteGetID(uart, 5)), "big"))
    # mc_id = (WriteGetID(uart, 5))
    print(f"Chip ID: {B_CYAN}{mc_id}{N_WHITE}")
    # uart.write(0x02)
    # time.sleep(0.001)
    # uart.write(0xFD)
    # res = uart.read(1)
    # print(res)
    print("Done")

    # Close the uart
    uart.close()


print(f"{B_WHITE}Starting{N_WHITE}")
try:
    ProgramSTM()
except Exception as ex:
    print(f"{B_RED}{ex}{N_WHITE}")
