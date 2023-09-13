import serial


BGY = "\033[1;30m"
BR = "\033[1;31m"
BG = "\033[1;32m"
BY = "\033[1;33m"
BB = "\033[1;34m"
BM = "\033[1;35m"
BC = "\033[1;36m"
BW = "\033[1;37m"

NGY = "\033[0;30m"
NR = "\033[0;31m"
NG = "\033[0;32m"
NY = "\033[0;33m"
NB = "\033[0;34m"
NM = "\033[0;35m"
NC = "\033[0;36m"
NW = "\033[0;37m"

READY = 0x80


def SendUploadSelectionCommand(uart: serial.Serial, command: str):
    send_data = [ch for ch in bytes(command, "UTF-8")]
    res = WriteBytesWaitForACK(uart, bytes(send_data), 5)
    if res == -1:
        raise Exception("Failed to move device into upload mode")

    if (command == "ui_upload"):
        # Change to parity even
        print(f"Update uart to parity: {BB}EVEN{NW}")
        uart.parity = serial.PARITY_EVEN

        # Wait for a response
        res = WaitForBytes(uart, 1)
        if (res != READY):
            raise Exception("NO REPLY received after activating ui upload")

        print(f"Activating UI Upload Mode: {BG}SUCCESS{NW}")
    elif (command == "net_upload"):
        # Change to parity none
        print(f"Update uart to parity: {BR}NONE{NW}")
        uart.parity = serial.PARITY_NONE

        # Wait for a response
        res = WaitForBytes(uart, 1)
        if (res != READY):
            raise Exception("NO REPLY received after activating net upload")


def WriteReadBytes(uart: serial.Serial, write_buff: bytes,
                   read_num: int, retry_num=5):
    while (retry_num > 0):
        retry_num -= 1
        uart.write(write_buff)

        reply = WaitForBytesExcept(uart, read_num)

        if (len(reply) < 1):
            continue

        return reply


def WriteByte(uart: serial.Serial, _byte: int,
              compliment: bool = True):
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
