import serial
import json
import hashlib
import time

from esp32_slip_packet import ESP32SlipPacket
import uart_utils
from ansi_colours import NW, BG, BY, BW, BB

# https://docs.espressif.com/projects/esptool/en/latest/esp32s3/advanced-topics/serial-protocol.html
from uploader import Uploader


class ESP32S3Uploader(Uploader):

    # TODO move into slip packet
    SYNC = 0x08
    FLASH_BEGIN = 0x02
    FLASH_DATA = 0x03
    FLASH_END = 0x04
    SPI_SET_PARAMS = 0x0B
    SPI_ATTACH = 0x0D
    SPI_FLASH_MD5 = 0x13

    READY = 0x80
    NO_REPLY = -1

    # Block size (1k)
    Block_Size = 0x400

    def __init__(self, uart: serial.Serial, chip: str):
        super().__init__(uart, chip)

    def FlashSelect(self):
        send_data = bytes([7, 0, 0])
        self.uart.write(send_data)
        print(f"Sent command to flash Net")

        self.uart.flush()

        self.TryPattern(uart_utils.OK, 1, 5)
        print(f"Flash UI command: {BG}CONFIRMED{NW}")

        print(f"Update uart to parity: {BB}EVEN{NW}")
        self.uart.parity = serial.PARITY_NONE

        self.TryPattern(uart_utils.READY, 1, 5)
        print(f"Flash UI: {BB}READY{NW}")

        self.uart.flush()
        self.uart.reset_input_buffer()

        print(f"Activating NET Upload Mode: {BG}SUCCESS{NW}")

    def FlashFirmware(self, binary_path: str) -> bool:
        print(f"{BW}Starting Net Upload{NW}")

        self.FlashSelect()

        self.Sync()

        self.AttachSPI()
        self.SetSPIParameters()

        self.Flash(binary_path)

        return True

    def WritePacketWaitForResponsePacket(
        self,
        packet: ESP32SlipPacket,
        packet_type: int = -1,
        checksum: bool = False,
        retry_num: int = 5,
    ):
        while retry_num > 0:
            retry_num -= 1
            self.WritePacket(packet, checksum)

            reply = self.WaitForResponsePacket(packet_type)

            if reply == -1:
                continue

            return reply

        return -1

    def WritePacket(self, packet: ESP32SlipPacket, checksum: bool = False):
        data = packet.SLIPEncode(checksum)
        self.uart.write(data)

    def WaitForResponsePacket(self, packet_type: int = -1):
        rx_byte = bytes(1)
        in_bytes = []

        packet = None
        packets = []

        # Loop until we get no reply, or we get the packet type we want
        while True:
            packet = ESP32SlipPacket()
            # Wait for start byte
            while rx_byte[0] != ESP32SlipPacket.END:
                rx_byte = self.uart.read(1)
                if len(rx_byte) < 1:
                    if len(packets) > 0:
                        return packets
                    else:
                        return -1

            # Append the end byte
            in_bytes.append(rx_byte)

            # Reset rx_byte
            rx_byte = bytes(1)
            while rx_byte[0] != ESP32SlipPacket.END:
                rx_byte = self.uart.read(1)
                if len(rx_byte) < 1:
                    if len(packets) > 0:
                        return packets
                    else:
                        return -1
                in_bytes.append(rx_byte)

            packet.FromBytes(in_bytes)
            if packet.Get(1, 1) == packet_type:
                break
            elif packet_type == -1:
                packets.append(packet)

        return packet

    def Flash(self, build_path: str):
        flasher_args = json.load(open(f"{build_path}/flasher_args.json"))

        binaries = []
        if "bootloader" in flasher_args:
            flasher_args["bootloader"]["name"] = "bootloader"
            binaries.append(flasher_args["bootloader"])
        if "partition-table" in flasher_args:
            flasher_args["partition-table"]["name"] = "partition-table"
            binaries.append(flasher_args["partition-table"])
        if "app" in flasher_args:
            flasher_args["app"]["name"] = "app"
            binaries.append(flasher_args["app"])

        for binary in binaries:
            data = open(f"{build_path}/{binary['file']}", "rb").read()
            size = len(data)
            offset = int(binary["offset"], 16)
            num_blocks = (size + self.Block_Size - 1) // self.Block_Size

            print(f"Flashing: {BY}{binary['name']}{NW}, size: {hex(size)}, " f"start_addr: {hex(offset)}")

            self.StartFlash(size, num_blocks, offset)
            self.WriteFlash(binary["file"], data, num_blocks)
            self.FlashMD5(data, offset, size)

        self.EndFlash()

    def StartFlash(self, size: int, num_blocks: int, offset: int):
        packet = ESP32SlipPacket(0x00, self.FLASH_BEGIN)

        # Size to erase
        packet.PushData(size, 4)

        # Number of incoming packets (blocks)
        packet.PushData(num_blocks, 4)

        # How big each packet will be
        packet.PushData(self.Block_Size, 4)

        # Where to begin writing for the incoming data
        packet.PushData(offset, 4)

        # Just some zeroes
        packet.PushData(0, 4)

        reply = self.WritePacketWaitForResponsePacket(packet, self.FLASH_BEGIN)

        # Check for error
        if reply.data[-1] == 1:
            print(reply)
            raise Exception("Error occurred when starting flashing")

    def WriteFlash(self, file: str, data: bytes, num_blocks: int):
        data_ptr = 0
        size = len(data)
        packet_idx = 0

        print(f"Flashing: {BG}00.00{NW}%", end="\r")

        while data_ptr < size:
            bin_packet = ESP32SlipPacket(0, self.FLASH_DATA)

            # Push on the data size which will be the block size
            bin_packet.PushData(self.Block_Size, 4)
            # Push the current bin_packet num aka sequence number
            bin_packet.PushData(packet_idx, 4)

            # Then some zeroes (32bit x 2 of zeros)
            bin_packet.PushData(0, 4)
            bin_packet.PushData(0, 4)

            data_bytes = data[data_ptr : data_ptr + self.Block_Size]

            if len(data_bytes) < self.Block_Size:
                # Pad the data to fit the block size
                data_bytes += bytes([0xFF] * (self.Block_Size - len(data_bytes)))

            # Push it all into the packet
            bin_packet.PushDataArray(data_bytes, "big")

            # Write the packet and wait for a reply
            reply = self.WritePacketWaitForResponsePacket(bin_packet, self.FLASH_DATA, checksum=True)

            if reply.GetCommand() != self.FLASH_DATA:
                print(reply)
                print(f"Error occurred when writing address {data_ptr}" f" of {file}")
                raise Exception("Error. Failed to write")

            print(f"Flashing: {BG}{(data_ptr/size) * 100:2.2f}{NW}%", end="\r")

            # Move the file pointer
            data_ptr += self.Block_Size

            # Increment the sequence number
            packet_idx += 1

        print(f"Flashing: {BG}100.00{NW}%")

    def EndFlash(self):
        packet = ESP32SlipPacket(0, self.FLASH_END)

        packet.PushData(0x1, 4)

        reply = self.WritePacketWaitForResponsePacket(packet, self.FLASH_END)

        if reply == self.NO_REPLY or reply.GetCommand() != self.FLASH_END:
            print("Failed to restart board")

        print(f"Flashing: {BG}COMPLETE{NW}")

    def FlashMD5(self, data: bytes, address: int, size: int):
        packet = ESP32SlipPacket(0, self.SPI_FLASH_MD5)
        packet.PushData(address, 4)
        packet.PushData(size, 4)
        packet.PushData(0, 4)
        packet.PushData(0, 4)

        reply = self.WritePacketWaitForResponsePacket(packet, self.SPI_FLASH_MD5)

        if reply.data[-1] == 1:
            print(f"Error occurred during flash. Reply dump: {reply}")
            raise Exception("Error")

        # Get the MD5 from the response
        res_md5 = reply.GetBytes(12, 36)
        res_md5.reverse()

        # Get the MD5
        # Calculate the MD5 for this data
        md5_hash = hashlib.md5(data).hexdigest()

        loc_md5 = []

        for h in md5_hash:
            loc_md5.append(ord(h))

        for i in range(len(loc_md5)):
            if res_md5[i] != loc_md5[i]:
                raise Exception("MD5 hashes did not match." f"\n\rReceived: {res_md5}" f"\n\rCalculated: {loc_md5}")

    def AttachSPI(self):
        packet = ESP32SlipPacket(0, self.SPI_ATTACH)
        packet.PushDataArray([0] * 8)

        reply = self.WritePacketWaitForResponsePacket(packet, self.SPI_ATTACH)

        if reply.data[-1] == 1:
            raise Exception(f"Error occurred in attach spi. Reply dump: {reply}")

    def SetSPIParameters(self):
        # This is all hardcoded...

        packet = ESP32SlipPacket(0, self.SPI_SET_PARAMS)
        # ID
        packet.PushData(0, 4)
        # total size (4MB)
        packet.PushData(0x400000, 4)
        # esp32s3 block size
        packet.PushData(64 * 1024, 4)
        # esp32s3 sector size
        packet.PushData(4 * 1024, 4)
        # esp32s3 Page size
        packet.PushData(256, 4)
        # status mask
        packet.PushData(0xFFFF, 4)

        reply = self.WritePacketWaitForResponsePacket(packet, self.SPI_SET_PARAMS)

        if reply.data[-1] == 1:
            print(f"Error occurred in spi set params. Reply dump: {reply}")

    def Sync(self):
        packet = ESP32SlipPacket(0x00, self.SYNC)

        packet.PushDataArray([0x07, 0x07, 0x012, 0x20], "big")
        packet.PushDataArray([0x55] * 32, "big")

        reply = self.WritePacketWaitForResponsePacket(packet, self.SYNC, retry_num=5)

        # We got a packet, so we are pleased with that
        if reply == -1:
            print(f"Activating device: {BY}NO REPLY{NW}")
            raise Exception("Failed to Activate device")

        print(f"Activating device: {BG}SUCCESS{NW}")
