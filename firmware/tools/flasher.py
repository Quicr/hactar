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

# Depending on the sector, we can add the bytes together iteratively.
User_Sector_Start_Address = 0x08000000

Sectors = [
    SimpleNamespace(**{"size": 0x4000, "addr": 0x08000000}),
    SimpleNamespace(**{"size": 0x0400, "addr": 0x08004000}),
    SimpleNamespace(**{"size": 0x0400, "addr": 0x08008000}),
    SimpleNamespace(**{"size": 0x0400, "addr": 0x0800C000}),
    SimpleNamespace(**{"size": 0x010000, "addr": 0x08010000}),
    SimpleNamespace(**{"size": 0x020000, "addr": 0x08020000}),
    SimpleNamespace(**{"size": 0x020000, "addr": 0x08040000}),
    SimpleNamespace(**{"size": 0x020000, "addr": 0x08060000}),
    SimpleNamespace(**{"size": 0x020000, "addr": 0x08080000}),
    SimpleNamespace(**{"size": 0x020000, "addr": 0x080A0000}),
    SimpleNamespace(**{"size": 0x020000, "addr": 0x080C0000}),
    SimpleNamespace(**{"size": 0x020000, "addr": 0x080E0000}),
]

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

# address = start_addr
# in_file = open("test.txt", "rb")
# firmware = in_file.read()
# file_addr = 0
# chunk = firmware[0:256]

# chunk_pad = 4 - (len(chunk) % 4)
# if (chunk_pad > 0):
#     chunk = chunk + bytes([0xFF] * chunk_pad)

# # Add the number of bytes to be received. 0 start
# chunk = (len(chunk) - 1).to_bytes(1, "big") + chunk

# print(chunk)
# print(len(chunk))
# exit()

if (len(sys.argv) < 3):
    print("Error. Need port followed by baudrate")
    exit()


def CalculateChecksum(arr: [int]):
    return functools.reduce(lambda a, b: a ^ b, arr).to_bytes(1, "big")


def WriteReadBytes(uart: serial.Serial, write_buff: bytes, read_num: int,
                   retry_num=5):
    while (retry_num > 0):
        retry_num -= 1
        uart.write(write_buff)

        reply = WaitForBytesExcept(uart, read_num)

        if (len(reply) < 1):
            continue

        return reply


def WriteByte(uart: serial.Serial, _byte: int, compliment: bool = True):
    data = [_byte]
    if (compliment):
        data.append(_byte ^ 0xFF)
    uart.write(bytes(data))


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
        time.sleep(0.1)


def SendSync(uart: serial.Serial, retry_num: int = 5):
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


def SendGetID(uart: serial.Serial, retry_num: int = 5):
    """ Sends the GetID command and it's compliment.
        Command = 0x02
        Compliment = 0xFD = 0x02 ^ 0xFF

        returns pid
    """
    res = WriteByteWaitForACK(uart, Commands.get_id, 1)

    if res == NACK:
        raise Exception("NACK was received during GetID")
    elif res == -1:
        raise Exception("No reply was received during GetID")

    # Read the next byte which should be the num of bytes - 1

    num_bytes = WaitForBytesExcept(uart, 1)

    # NOTE there is a bug in the bootloader for stm, or there is a
    # inaccuracy in the an3155 datasheet that says that a single ACK
    # is sent after receiving the get_id command.
    # But in reality 90% of the time two ACK bytes are sent
    if (num_bytes == ACK):
        num_bytes = WaitForBytesExcept(uart, 1)
    elif (num_bytes == NACK):
        raise Exception("NACK was received while trying to get the num bytes "
                        "in GetID")

    # Get the pid which should be N+1 bytes
    pid = WaitForBytesExcept(uart, num_bytes+1)

    # Wait for an ack
    res = WaitForBytesExcept(uart, 1)
    if (res == NACK):
        raise Exception("A NACK was received at the end of GetID")
    elif res == -1:
        raise Exception("No reply was received during GetID")

    h_pid = hex(int.from_bytes(bytes(pid), "big"))
    print(f"Chip ID: {B_CYAN}{h_pid}{N_WHITE}")

    return pid


def SendGet(uart: serial.Serial, retry_num: int = 5):
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


def SendReadMemory(uart: serial.Serial, address: bytes, num_bytes: int,
                   retry_num: int = 5):
    """ Sends the Read Memory command and it's compliment.
        Command = 0x11
        Compliment = 0xEE = 0x11 ^ 0xFF

        returns [bytes] contents of memory [address:address + num_bytes]

    """
    reply = WriteByteWaitForACK(uart, Commands.read_memory, retry_num)
    if (reply == NACK):
        raise Exception("NACK was received during Read Memory command")

    memory_addr = address + CalculateChecksum(address)

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

    return recv_data


def SendExtendedEraseMemory(uart: serial.Serial, sectors: [int],
                            special: bool, retry_num: int = 5):
    """ Sends the Erase memory command and it's compliment.
        Command = 0x44
        Compliment = 0xBB = 0x44 ^ 0xFF

        returns ACK/NACK/0
    """
    reply = WriteByteWaitForACK(uart, Commands.extended_erase, 5)
    if (reply == NACK):
        raise Exception("NACK was received after sending erase command")
    elif (reply == -1):
        raise Exception("No reply was received after sending erase command")

    # TODO error check sectors?

    # Number of sectors starts at 0x00 0x00. So 0x00 0x00 means delete 1 sector
    # An exception is thrown if the number is bigger than 2 bytes
    num_sectors = [(len(sectors) - 1).to_bytes(2, "big")]

    # Turn the sectors received into two byte elements
    byte_sectors = [x.to_bytes(2, "big") for x in sectors]

    # Join the num sectors and the byte sectors into one list
    data = b''.join(num_sectors + byte_sectors)

    # Calculate the checksum
    checksum = [CalculateChecksum(data)]

    # Join all the data together
    data = b''.join(num_sectors + byte_sectors + checksum)

    print(f"Erase: Sectors {N_YELLOW}{sectors}{N_WHITE}")
    print(f"Erase: {B_BLUE}STARTED{N_WHITE}")

    # Give more time to reply with the deleted sectors
    uart.timeout = 10
    reply = WriteBytesWaitForACK(uart, data, 1)
    # Revert the change back to two seconds
    uart.timeout = 2
    if (reply == -1):
        raise Exception("Failed to erase, no reply received")
    elif (reply == NACK):
        raise Exception("Failed to erase")

    mem_bytes_sz = 256
    expected_mem = [255] * mem_bytes_sz
    mem = [0] * mem_bytes_sz
    read_count = 0
    bytes_verified = 0
    total_bytes_to_verify = 0
    for sector in sectors:
        total_bytes_to_verify += Sectors[sector].size
    percent_verified = int((bytes_verified / total_bytes_to_verify)*100)

    for sector in sectors:
        memory_address = Sectors[sector].addr
        end_of_sector = Sectors[sector].addr + Sectors[sector].size
        read_count = 0
        mem = [0] * mem_bytes_sz
        while (memory_address != end_of_sector):
            percent_verified = int(
                (bytes_verified / total_bytes_to_verify)*100)
            print(f"Verifying erase: {B_GREEN}{percent_verified:2}{N_WHITE}"
                  f"% verified", end="\r")
            while (mem != expected_mem) and read_count != 5:
                mem = SendReadMemory(uart, memory_address.to_bytes(4, "big"),
                                     mem_bytes_sz)
                read_count += 1
                time.sleep(0.5)

            if (read_count == 5 and mem != expected_mem):
                print(f"Verifying: {B_RED}Failed to verify sector [{sector}]\
                    {N_WHITE}")
                return False

            memory_address += mem_bytes_sz
            bytes_verified += mem_bytes_sz
            time.sleep(0.001)

    # Don't actually need to do the math here
    print(f"Verifying erase: {B_GREEN}100{N_WHITE}% verified")
    print(f"Erase: {B_GREEN}COMPLETE{N_WHITE}")
    return True


def SendWriteMemory(uart: serial.Serial, data: bytes, address: int,
                    num_retry: int = 5):
    """ Sends the Write memory command and it's compliment.
        Then writes the address bytes and its checksum
        Then writes the entire data stream
        Command = 0x31
        Compliment = 0xCE = 0x31 ^ 0xFF

        returns ACK if successful
    """

    Max_Num_Bytes = 256

    # Send the address and the checksum
    addr = address
    file_addr = 0

    percent_flashed = 0

    total_bytes = len(data)

    print(f"Write to Memory: {B_BLUE}STARTED{N_WHITE}")
    print(f"Address: {B_WHITE}{address:04x}{N_WHITE}")
    print(f"Byte Stream Size: {B_WHITE}{total_bytes}{N_WHITE}")

    while file_addr < total_bytes:
        percent_flashed = int((file_addr / total_bytes) * 100)

        print(f"Flashing: {B_GREEN}{percent_flashed:2}{N_WHITE}%", end="\r")
        reply = WriteByteWaitForACK(uart, Commands.write_memory, 5)
        if (reply == NACK):
            raise Exception("NACK was received after sending write command")
        elif (reply == -1):
            raise Exception("No reply was received after sending write "
                            "command")

        checksum = CalculateChecksum(addr.to_bytes(4, "big"))
        write_address_bytes = addr.to_bytes(4, "big") + checksum
        reply = WriteBytesWaitForACK(uart, write_address_bytes, 1)
        if (reply == NACK):
            raise Exception("NACK was received after sending write command")
        elif (reply == -1):
            raise Exception("No reply was received after sending write"
                            "command")

        # Get the contents of the binary
        chunk = data[file_addr:file_addr+Max_Num_Bytes]
        # Chunk size before extra bytes are appended
        chunk_size = len(chunk)

        # Pad the chunk to be a multiple of 4 bytes
        if (len(chunk) % 4 != 0):
            chunk = chunk + bytes([0xFF] * (4 - (len(chunk) % 4)))

        # Front append the number of bytes to be received. 0 start
        chunk = (len(chunk) - 1).to_bytes(1, "big") + chunk

        # Add the checksum
        chunk = chunk + CalculateChecksum(chunk)

        # Write the chunk to the chip
        reply = WriteBytesWaitForACK(uart, chunk, 1)
        if (reply == NACK):
            raise Exception(f"NACK was received while writing to "
                            f"addr: {addr}")
        elif (reply == -1):
            raise Exception(f"No reply was received while writing to "
                            f"addr: {addr}")

        # Shift over by the amount of byte stream bytes were sent
        file_addr += chunk_size
        addr += chunk_size

    # Don't need to calculate it here it finished
    print(f"Flashing: {B_GREEN}100{N_WHITE}%")

    # Verify the written memory by reading the size of the file
    addr = address
    file_addr = 0

    while file_addr < total_bytes:
        percent_verified = int(
            (file_addr / total_bytes)*100)
        print(f"Verifying write: {B_GREEN}{percent_verified:2}{N_WHITE}"
              f"% verified", end="\r")

        chunk = data[file_addr:file_addr+Max_Num_Bytes]
        mem = bytes(SendReadMemory(uart, addr.to_bytes(4, "big"), len(chunk)))

        if chunk != mem:
            raise Exception(f"Failed to verify at memory address {addr}")

        addr += len(mem)
        file_addr += len(chunk)

    print(f"Verifying write: {B_GREEN}100{N_WHITE}% verified")
    print(f"Write: {B_GREEN}COMPLETE{N_WHITE}")

    return ACK


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

    print(f"Opened port: {B_GREEN}{port}{N_WHITE}")
    print(f"baudrate: {B_BLUE}{baud}{N_WHITE}")

    # Send the command to the mgmt chip that we want to program the UI chip
    SendUploadSelectionCommand(uart, "ui_upload")
    # # TODO wait for reply from mgmt chip

    SendSync(uart)
    # Sometimes a small delay is required otherwise it won't
    # continue on correctly

    SendGetID(uart, 1)

    SendGet(uart)

    mem = SendReadMemory(uart, bytes([0x08, 0x00, 0x00, 0x00]), 256)
    print(f"Reading memory: {B_GREEN}SUCCESSFUL{N_WHITE}")
    print(f"Number of bytes read from memory: {B_MAGENTA}{len(mem)}\
        {N_WHITE}")

    time.sleep(1)

    # TODO get the size of our binary file and erase the
    # sectors that it would fit into
    sectors = [x for x in range(6)]
    SendExtendedEraseMemory(uart, sectors, False)

    firmware = open("../ui/build/ui.bin", "rb").read()
    SendWriteMemory(uart, firmware, User_Sector_Start_Address, 1)

    # Need to wait before continuing on.
    time.sleep(2)

    # Close the uart
    uart.close()


print(f"{B_WHITE}Starting{N_WHITE}")
try:
    ProgramSTM()
except Exception as ex:
    print(f"{B_RED}{ex}{N_WHITE}")
