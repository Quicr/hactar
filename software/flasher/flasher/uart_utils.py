import serial
import time

from ansi_colours import BG, NW, BR, BB

ACK = 0x79
OK = 0x80
READY = 0x81
MAX_WAIT = 15


def WriteReadBytes(uart: serial.Serial, write_buff: bytes, read_num: int, retry_num=5):
    while retry_num > 0:
        retry_num -= 1
        uart.write(write_buff)

        reply = TryGetBytes(uart, read_num)

        if len(reply) < 1:
            continue

        return reply


def WriteByte(uart: serial.Serial, _byte: int, compliment: bool = True):
    data = [_byte]
    if compliment:
        data.append(_byte ^ 0xFF)
    uart.write(bytes(data))


def WriteByteWaitForACK(
    uart: serial.Serial, _bytes: int, retry_num: int = 5, compliment: bool = True
):
    data = [_bytes]
    if compliment:
        data.insert(0, _bytes ^ 0xFF)

    print("Write:", data)
    print("parity:", uart.parity)
    reply = 0
    while retry_num > 0:
        retry_num -= 1

        uart.write(bytes(data))

        reply = GetBytes(uart, 1)
        print("reply", reply)
        if reply == -1:
            uart.flush()
            uart.close()
            time.sleep(0.5)
            uart.open()
            time.sleep(0.5)
            continue

        if reply == ACK:
            return reply

    return reply


def WriteBytesWaitForACK(uart: serial.Serial, _bytes: bytes, retry_num: int = 5):
    """Assumes the data it receives is complete"""

    while retry_num > 0:
        retry_num -= 1
        print("Write:", _bytes)
        uart.write(_bytes)

        reply = GetBytes(uart, 1)

        print("reply", reply)
        if reply == -1:
            uart.close()
            time.sleep(0.5)
            uart.open()
            time.sleep(0.5)
            continue

        return reply

    return -1


def GetBytes(uart: serial.Serial, num_bytes: int):
    """Sits and waits for a response on the serial line.
    If no reply is received then return -1

    Note - Passing num_bytes <= 1 returns an int
        - Passing num_bytes >  1 returns an int array
    """
    rx_byte = uart.read(num_bytes)
    print("Get bytes:", rx_byte, len(rx_byte))
    if len(rx_byte) < 1:
        return -1

    if num_bytes <= 1:
        return int.from_bytes(rx_byte, "big")

    # Convert to ints
    return [x for x in rx_byte]


def TryGetBytes(uart: serial.Serial, num_bytes: int):
    """Sits and waits for a response on the serial line
    if no reply is received raise an exception.

    Note - Passing num_bytes <= 1 returns an int
        - Passing num_bytes >  1 returns an int array
    """
    rx_byte = uart.read(num_bytes)
    if len(rx_byte) < 1:
        raise Exception("Error. No reply")

    if num_bytes <= 1:
        return int.from_bytes(rx_byte, "big")

    # Convert to ints
    return [x for x in rx_byte]


def TryPattern(
    uart: serial.Serial, pattern: int, recv_bytes_cnt: int, num_retry: int = 5
):
    rx = 0
    for i in range(num_retry):
        rx = GetBytes(uart, recv_bytes_cnt)

        if rx == pattern:
            return

    raise Exception(f"Didn't receive pattern: {pattern} got {rx}")


def TryHandshake(
    uart: serial.Serial, pattern: int, recv_bytes_cnt: int, num_retry: int = 5
):
    rx = 0
    for i in range(num_retry):
        rx = GetBytes(uart, recv_bytes_cnt)

        if rx == pattern:
            WriteByte(uart, pattern, False)
            return

    raise Exception(f"Didn't receive handshake: {pattern} got {rx}")
