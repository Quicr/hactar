import serial
import sys
import time
import functools
from types import SimpleNamespace
# https://www.manualslib.com/download/1764455/St-An3155.html

ACK = 0x79
NACK = 0x1F

Commands = SimpleNamespace(**{
    "sync": 0x7F,
    "get": 0x00,
    "get_version_and_read_protection_status": 0x01,
    "get_id": 0x02,
    "read_memory": 0x11,
    "go": 0x21,
    "write_memory": 0x31,
    "erase": 0x43,
    "extended_erase": 0x44,
    "write_protect": 0x63,
    "write_unprotect": 0x73,
    "readout_protect": 0x82,
    "readout_unprotect": 0x92
})

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


def WriteReadBytes(uart: serial.Serial, write_buff: bytes, read_num: int,
                   retry_num=5):
    while (retry_num > 0):
        retry_num -= 1
        uart.write(write_buff)

        reply = WaitForBytesExcept(uart, read_num)

        if (len(reply) < 1):
            continue

        return reply


def WriteByteWaitForACK(uart: serial.Serial, _bytes: int,
                        retry_num: int = 5, compliment: bool = True):

    while (retry_num > 0):
        retry_num -= 1

        data = [_bytes]
        if (compliment):
            data.append(_bytes ^ 0xFF)
        uart.write(bytes(data))

        reply = WaitForBytes(uart, 1)

        if (reply == -1):
            continue

        return reply

    return -1


def WriteBytesWaitForACK(uart: serial.Serial, _bytes: bytes,
                         retry_num: int = 5):
    """ Assumes the data it receives is complete """

    while (retry_num > 0):
        retry_num -= 1

        uart.write(_bytes)

        reply = WaitForBytes(uart, 1)

        if (reply == -1):
            continue

        return reply

    return -1


def WaitForBytes(uart: serial.Serial, num_bytes: int):
    """ Sits and waits for a response on the serial line.
        If no reply is received then return -1

        Note - Passing num_bytes <= 1 returns an int
             - Passing num_bytes >  1 returns an int array
    """
    rx_byte = uart.read(num_bytes)
    if (len(rx_byte) < 1):
        return -1

    if (num_bytes <= 1):
        return int.from_bytes(rx_byte, "big")

    # Convert to ints
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
        return int.from_bytes(rx_byte, "big")

    # Convert to ints
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


def WriteSync(uart: serial.Serial, retry_num: int = 5):
    """ Sends the sync byte 0x7F and waits for an ACK from the device.
    """

    reply = WriteByteWaitForACK(uart, Commands.sync, retry_num, False)

    # Check if ack or nack
    if (reply == NACK):
        print(f"Activating device: {B_RED}FAILED{N_WHITE}")
        raise Exception("Failed to Activate device")
    elif (reply == -1):
        print(f"Activating device: {B_YELLOW}NO REPLY{N_WHITE}")
        raise Exception("Failed to Activate device")

    print(f"Activating device: {B_GREEN}SUCCESS{N_WHITE}")


def WriteGetID(uart: serial.Serial, retry_num: int = 5):
    """ Sends the GetID command and it's compliment.
        Command = 0x02
        Compliment = 0xFD = 0x02 ^ 0xFF

        returns pid
    """
    res = WriteByteWaitForACK(uart, Commands.get_id, 5)

    if res == NACK:
        raise Exception("NACK was received during GetID")

    # Read the next byte which should be the num of bytes - 1
    #  for some reason they minus off one even tho 2 bytes come in
    num_bytes = WaitForBytesExcept(uart, 1) + 1

    # Get the pid which should be 2 bytes
    pid = WaitForBytesExcept(uart, num_bytes)

    # Wait for an ack
    res = WaitForBytesExcept(uart, 1)

    if (res == NACK):
        raise Exception("A NACK was received at the end of GetID")

    h_pid = hex(int.from_bytes(bytes(pid), "big"))
    print(f"Chip ID: {B_CYAN}{h_pid}{N_WHITE}")

    return pid


def WriteGet(uart: serial.Serial, retry_num: int = 5):
    """ Sends the Get command and it's compliment.
        Command = 0x00
        Compliment = 0xFF = 0x00 ^ 0xFF

        returns all available commands
    """
    reply = WriteByteWaitForACK(uart, Commands.get, retry_num)
    if (reply == NACK):
        raise Exception("NACK was received during Get Commands")

    # Read the next byte which should be the num of bytes - 1
    #  for some reason they minus off one even tho 2 bytes come in
    num_bytes = WaitForBytesExcept(uart, 1)

    bootloader_verison = WaitForBytesExcept(uart, 1)

    # Get all of the available commands
    recv_commands = WaitForBytesExcept(uart, num_bytes)

    # Wait for an ack
    reply = WaitForBytesExcept(uart, 1)

    # Parse the commands
    available_commands = []
    for key, val in Commands.__dict__.items():
        for cmd in recv_commands:
            if (val == cmd):
                available_commands.append(key)

    if (reply == NACK):
        raise Exception("A NACK was received at the end of GetID")

    print(f"Bootloader version: {B_MAGENTA}{bootloader_verison}{N_WHITE}")

    print(f"{num_bytes} available commands: ", end="")
    for i, cmd in enumerate(available_commands):
        if (i == len(available_commands) - 1):
            print(cmd)
        else:
            print(cmd, end=", ")

    return available_commands


def WriteReadMemory(uart: serial.Serial, read_from: [int], num_bytes: int,
                    retry_num: int = 5):
    """ Sends the Read Memory command and it's compliment.
        Command = 0x11
        Compliment = 0xEE = 0x11 ^ 0xFF

        returns ACK/NACK/0
    """
    reply = WriteByteWaitForACK(uart, Commands.read_memory, retry_num)
    if (reply == NACK):
        raise Exception("NACK was received during Read Memory command")

    # Calculate the checksum
    checksum = functools.reduce(lambda a, b: a ^ b, read_from)

    memory_addr = read_from
    memory_addr.append(checksum)

    # Write the memory we want to read from and the checksum
    reply = WriteBytesWaitForACK(uart, memory_addr, 1)
    if (reply == NACK):
        raise Exception("NACK was received after sending memory address to \
                         read")

    reply = WriteByteWaitForACK(uart, num_bytes-1, 1)
    if (reply == NACK):
        raise Exception("NACK was received after sending how many bytes to \
                         read")

    recv_data = WaitForBytesExcept(uart, num_bytes)
    if type(recv_data) is int:
        recv_data = [recv_data]

    print(f"Reading memory: {B_GREEN}SUCCESSFUL{N_WHITE}")
    print(f"Number of bytes read from memory: {B_MAGENTA}{len(recv_data)}\
        {N_WHITE}")

    return recv_data


def WriteExtendedEraseMemory(uart: serial.Serial, sectors: [int],
                             special: bool, retry_num: int = 5):
    """ Sends the Erase memory command and it's compliment.
        Command = 0x11
        Compliment = 0xEE = 0x11 ^ 0xFF

        returns ACK/NACK/0
    """
    reply = WriteByteWaitForACK(uart, Commands.extended_erase, 5)
    if (reply == NACK):
        raise Exception("NACK was received after sending erase command")

    data = []

    # Number of sectors starts at 0x00 0x00. So 0x00 0x00 means delete 1 sector
    # An exception is thrown if the number is bigger than 2 bytes
    num_sectors = (len(sectors) - 1).to_bytes(2, "big")

    # Turn the sectors received into two byte elements
    byte_sectors = [x.to_bytes(2, "big") for x in sectors]

    # Join the num sectors and the byte sectors into one list
    data = b''.join([num_sectors] + byte_sectors)

    # byte_sectors = b''.join(byte_sectors)

    # Calculate the checksum
    checksum = functools.reduce(lambda a, b: a ^ b, data)
    print([num_sectors])
    print(byte_sectors)
    print([checksum])
    data = b''.join([num_sectors] + byte_sectors + [checksum.to_bytes(1, "big")])

    print(f"Erase: {B_BLUE}STARTED{N_WHITE}")

    reply = WriteBytesWaitForACK(uart, data, 1)
    print(reply)
    if (reply == -1):
        raise Exception("Failed to erase, no reply received")
    elif (reply == NACK):
        raise Exception("Failed to erase")

    mem_bytes_sz = 256
    expected_mem = [255] * mem_bytes_sz
    mem = [0] * mem_bytes_sz
    read_count = 0
    while (mem != expected_mem) and read_count != 5:
        mem = WriteReadMemory(uart, [0x08, 0x00, 0x00, 0x00], mem_bytes_sz)
        time.sleep(0.5)

    print(f"Erase: {B_CYAN}COMPLETE{N_WHITE}")
    return True


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

    print(f"Opened port: {B_GREEN}{port}{N_WHITE}, baudrate: {B_BLUE}{baud}{N_WHITE}")

    # Send the command to the mgmt chip that we want to program the UI chip
    SendUploadSelectionCommand(uart, "ui_upload")
    # # TODO wait for reply from mgmt chip

    WriteSync(uart)

    # Sometimes a small delay is required otherwise it won't continue on correctly

    WriteGetID(uart)

    WriteGet(uart)

    WriteReadMemory(uart, [0x08, 0x00, 0x00, 0x00], 1)

    sectors = [x for x in range(2)]
    WriteExtendedEraseMemory(uart,
                             sectors, False)

    # Need to wait before continuing on.
    time.sleep(2)


    # Close the uart
    uart.close()


print(f"{B_WHITE}Starting{N_WHITE}")
try:
    ProgramSTM()
except Exception as ex:
    print(f"{B_RED}{ex}{N_WHITE}")
