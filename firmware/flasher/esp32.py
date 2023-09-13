import serial
# import time
# import functools

from esp32_slip_packet import esp32_slip_packet

# https://docs.espressif.com/projects/esptool/en/latest/esp32s3/advanced-topics/serial-protocol.html


class esp32_flasher:

    SYNC = 0x08

    READY = 0x80
    NO_REPLY = -1

    def __init__(self, uart: serial.Serial):
        self.uart = uart

    def WriteRead_Bytes(self, data: bytes, read_num: int, retry_num: int = 5):

        while (retry_num > 0):
            retry_num -= 1
            self.uart.write(data)

            reply = self.WaitForBytesExcept(read_num)

            if (len(reply) < 1):
                continue

            return reply

    def Sync(self):
        packet = esp32_slip_packet(0x00, self.SYNC)
        packet.PushDataArray([0x07, 0x07, 0x012, 0x20])
        packet.PushDataArray([0x55] * 32)
