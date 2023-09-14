import serial
import time
# import functools

from esp32_slip_packet import esp32_slip_packet
import uart_utils

# https://docs.espressif.com/projects/esptool/en/latest/esp32s3/advanced-topics/serial-protocol.html


class esp32_flasher:

    SYNC = 0x08

    READY = 0x80
    NO_REPLY = -1

    def __init__(self, uart: serial.Serial):
        self.uart = uart

    def WritePacketWaitForResponsePacket(self, packet: esp32_slip_packet,
                                         retry_num: int = 5):
        while (retry_num > 0):
            retry_num -= 1
            self.WritePacket(packet)

            reply = self.WaitForResponsePacket()

            print(reply)
            if (reply == -1):
                continue

            return reply

        return -1

    def WritePacket(self, packet: esp32_slip_packet):
        print("encode")
        data = packet.SLIPEncode()
        self.uart.write(data)

    def WaitForResponsePacket(self):
        rx_byte = bytes(1)
        in_bytes = []

        # Wait for start byte
        while rx_byte[0] != esp32_slip_packet.END:
            rx_byte = self.uart.read(1)[0]
            if (len(rx_byte) < 1):
                return -1

            if (rx_byte[0] == esp32_slip_packet.END):
                in_bytes.append(rx_byte)
                break

        rx_byte = bytes(1)
        while rx_byte[0] != esp32_slip_packet.END:
            rx_byte = self.uart.read(1)
            if (len(rx_byte) < 1):
                return -1
            in_bytes.append(rx_byte)

        print(in_bytes)
        packet = esp32_slip_packet()
        packet.FromBytes(in_bytes)
        return packet

    def Sync(self):
        packet = esp32_slip_packet(0x00, self.SYNC)
        packet.PushDataArray([0x07, 0x07, 0x012, 0x20])
        packet.PushDataArray([0x55] * 32)

        self.WritePacketWaitForResponsePacket(packet, 5)

        self

    def ProgramESP(self):
        uart_utils.SendUploadSelectionCommand(self.uart, "net_upload")

        self.Sync()
