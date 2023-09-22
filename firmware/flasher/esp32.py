import serial
import time
import json

from esp32_slip_packet import esp32_slip_packet
import uart_utils
from ansi_colours import NW, NY, BG, BY

# https://docs.espressif.com/projects/esptool/en/latest/esp32s3/advanced-topics/serial-protocol.html


class esp32_flasher:

    SYNC = 0x08
    FLASH_BEGIN = 0x02
    FLASH_DATA = 0x03
    FLASH_END = 0x04

    READY = 0x80
    NO_REPLY = -1

    # Block size (16k)
    Block_Size = 0x4000

    def __init__(self, uart: serial.Serial):
        self.uart = uart

    def WritePacketWaitForResponsePacket(self, packet: esp32_slip_packet,
                                         packet_type: int = -1,
                                         retry_num: int = 5):
        while (retry_num > 0):
            retry_num -= 1
            self.WritePacket(packet)

            reply = self.WaitForResponsePacket(packet_type)

            if (reply == -1):
                continue

            return reply

        return -1

    def WritePacket(self, packet: esp32_slip_packet):
        data = packet.SLIPEncode()
        self.uart.write(data)

    def WaitForResponsePacket(self, packet_type: int = -1):
        rx_byte = bytes(1)
        in_bytes = []

        packet = None
        packets = []

        # Loop until we get no reply, or we get the packet type we want
        while True:
            packet = esp32_slip_packet()
            # Wait for start byte
            while rx_byte[0] != esp32_slip_packet.END:
                rx_byte = self.uart.read(1)
                if (len(rx_byte) < 1):
                    if len(packets) > 0:
                        return packets
                    else:
                        return -1

            # Append the end byte
            in_bytes.append(rx_byte)

            # Reset rx_byte
            rx_byte = bytes(1)
            while rx_byte[0] != esp32_slip_packet.END:
                rx_byte = self.uart.read(1)
                if (len(rx_byte) < 1):
                    if len(packets) > 0:
                        return packets
                    else:
                        return -1
                in_bytes.append(rx_byte)

            packet.FromBytes(in_bytes)
            if (packet.Get(1, 1) == packet_type):
                print("break out")
                break
            elif (packet_type == -1):
                print("push packet")
                packets.append(packet)

        return packet

    def Flash(self, build_path: str):
        flasher_args = json.load(open(f"{build_path}/flasher_args.json"))

        binaries = []
        if ("bootloader" in flasher_args):
            binaries.append(flasher_args["bootloader"])
        # if ("partition-table" in flasher_args):
        #     binaries.append(flasher_args["partition-table"])
        # if ("app" in flasher_args):
        #     binaries.append(flasher_args["app"])

        for binary in binaries:
            data = open(f"{build_path}/{binary['file']}", "rb").read()
            size = len(data)
            offset = int(binary["offset"], 16)

            self.StartFlash(size, offset)
            self.WriteFlash(data)
            self.EndFlash(data)

    def StartFlash(self, size: int, offset: int):
        # Get the num blocks
        num_blocks = (size + self.Block_Size - 1) // self.Block_Size

        # Get the size to erase
        size_to_erase = size

        packet = esp32_slip_packet(0x00, self.FLASH_BEGIN)

        # data = [x for x in size_to_erase.to_bytes(4, "big")]
        # data += [x for x in num_blocks.to_bytes(4, "big")]
        # data += [x for x in self.Block_Size.to_bytes(4, "big")]
        # data += [x for x in offset.to_bytes(4, "big")]
        # packet.PushDataArray(data, "little")


        packet.PushData(size_to_erase, 4)
        packet.PushData(num_blocks, 4)

        packet.PushData(self.Block_Size, 4)


        packet.PushData(offset, 4)


        # packet.PushData(0, 4)
        print(packet.data_length)
        print(packet.data)

        print(packet.ToEncodedBytes())

        reply = self.WritePacketWaitForResponsePacket(packet, self.FLASH_BEGIN)
        print(reply.ToEncodedBytes())

    def WriteFlash(self, data):
        pass

    def EndFlash(self, data):
        pass

    def Sync(self):
        packet = esp32_slip_packet(0x00, self.SYNC)

        packet.PushDataArray([0x07, 0x07, 0x012, 0x20], "big")
        packet.PushDataArray([0x55] * 32, "big")

        reply = self.WritePacketWaitForResponsePacket(packet, self.SYNC, 5)

        # We got a packet, so we are pleased with that
        if (reply == -1):
            print(f"Activating device: {BY}NO REPLY{NW}")
            raise Exception("Failed to Activate device")
        # reply = self.WaitForResponsePacket()
        # time.sleep(1)
        # reply = self.WaitForResponsePacket()

        if (type(reply) is list):
            for r in reply:
                print(r.ToEncodedBytes())
        else:
            print(reply.ToEncodedBytes())

        print(f"Activating device: {BG}SUCCESS{NW}")

    def ProgramESP(self, build_path: str):
        uart_utils.SendUploadSelectionCommand(self.uart, "net_upload")

        # time.sleep(2)
        self.Sync()

        # TODO might be a few more steps where I need to first configure
        # the spi flash configurations before I start sending any data
        # I also need to figure out why they are flashing so much
        # data to memory

        self.Flash(build_path)
