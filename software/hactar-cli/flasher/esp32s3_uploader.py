import serial
import json
import hashlib
import time

from esp32_slip_packet import ESP32SlipPacket
import uart_utils
from ansi_colours import NW, BG, BY, BW, BB

# https://docs.espressif.com/projects/esptool/en/latest/esp32s3/advanced-topics/serial-protocol.html
from uploader import Uploader
from hactar_commands import command_map


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

    # Flash pages are 4KB aligned
    Page_Size = 0x1000

    def __init__(self, chip: str, binary_path: str):
        super().__init__(chip)

        self.binary_path = binary_path

        flasher_args = json.load(open(f"{binary_path}/flasher_args.json"))

        self.binaries = []
        if "bootloader" in flasher_args:
            flasher_args["bootloader"]["name"] = "bootloader"
            self.binaries.append(flasher_args["bootloader"])
        if "partition-table" in flasher_args:
            flasher_args["partition-table"]["name"] = "partition-table"
            self.binaries.append(flasher_args["partition-table"])
        if "app" in flasher_args:
            flasher_args["app"]["name"] = "app"
            self.binaries.append(flasher_args["app"])

        self.binary_idx = 0
        self.data_idx = 0

    def FlashFirmware(self, uart: serial.Serial) -> bool:
        print(f"{BW}Starting Net Upload{NW}")

        self.FlashSelect(uart)

        self.Sync(uart)

        self.AttachSPI(uart)
        self.SetSPIParameters(uart)

        self.Flash(uart)

        return True

    def FlashSelect(self, uart: serial.Serial):
        uart.write(command_map["flash net"])
        print(f"Sent command to flash Net")

        uart.flush()

        self.TryPattern(uart, uart_utils.OK, 1, 5)
        print(f"Flash Net command: {BG}CONFIRMED{NW}")

        print(f"Update uart to parity: {BB}NONE{NW}")
        uart.parity = serial.PARITY_NONE

        self.TryPattern(uart, uart_utils.READY, 1, 5)
        print(f"Flash Net: {BB}READY{NW}")

        uart.flush()
        uart.reset_input_buffer()

        print(f"Activating NET Upload Mode: {BG}SUCCESS{NW}")

    def Sync(self, uart: serial.Serial):
        print(f"Syncing uart to ESP32S3")
        # For some reason sync sends 4 packets
        packet = ESP32SlipPacket(0x00, self.SYNC)

        packet.PushDataArray([0x07, 0x07, 0x012, 0x20], "big")
        packet.PushDataArray([0x55] * 32, "big")

        timeout = time.time() + 10

        num_replies = 0
        synced = False

        reply = self.WritePacketWaitForResponsePacket(uart, packet, self.SYNC, retry_num=10)

        num_sync_received = 0

        while True:
            if time.time() > timeout:
                raise Exception("Timed out on sync")

            if reply == -1:
                if not synced:
                    print(f"Activating device: {BY}NO REPLY{NW}")
                    raise Exception("Failed to Activate device")

                # We are synced and we got a no reply, which means we read all of the syncs
                break

            if reply.GetCommand() != self.SYNC:
                print(f"Got a different command. Expected {self.SYNC} Got {reply.GetCommand()}")
                raise Exception("Failed to Activate device")

            if reply.GetCommand() == self.SYNC:
                synced = True
                num_sync_received += 1
                timeout = time.time() + 10

            reply = self.WaitForResponsePacket(uart, self.SYNC)

            print(f"Sync packets recevied: {num_sync_received}")

        print(f"Syncing uart to ESP32S3: {BG}SUCCESS{NW}")

    def AttachSPI(self, uart: serial.Serial):
        packet = ESP32SlipPacket(0, self.SPI_ATTACH)
        packet.PushDataArray([0] * 8)

        reply = self.WritePacketWaitForResponsePacket(uart, packet, self.SPI_ATTACH)

        if reply.data[-1] == 1 or reply.GetCommand() != self.SPI_ATTACH:
            raise Exception(f"Error occurred in attach spi. Reply dump: {reply}")

        print("Attached SPI")

    def SetSPIParameters(self, uart: serial.Serial):
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

        reply = self.WritePacketWaitForResponsePacket(uart, packet, self.SPI_SET_PARAMS)

        if reply.data[-1] == 1 or reply.GetCommand() != self.SPI_SET_PARAMS:
            print(f"Error occurred in spi set params. Reply dump: {reply}")

        print("SPI parameters set!")

    def WritePacketWaitForResponsePacket(
        self,
        uart: serial.Serial,
        packet: ESP32SlipPacket,
        packet_type: int = -1,
        checksum: bool = False,
        retry_num: int = 5,
    ):
        while retry_num > 0:
            retry_num -= 1
            self.WritePacket(uart, packet, checksum)

            reply = self.WaitForResponsePacket(uart, packet_type)

            if reply == -1:
                continue

            return reply

        return -1

    def Flash(self, uart: serial.Serial):
        while self.binary_idx < len(self.binaries):
            binary = self.binaries[self.binary_idx]

            # Determine our current page number
            page = self.data_idx // self.Page_Size

            self.data_idx = page * self.Page_Size

            # Get the firmware binary
            data = open(f"{self.binary_path}/{binary['file']}", "rb").read()
            binary_size = len(data)
            starting_offset = int(binary["offset"], 16)

            # Get the total number of bytes we'll be sending
            send_size = binary_size - self.data_idx

            # Get the offset of where we will be writing to
            offset = starting_offset + self.data_idx

            # Determine the number of packets we'll be sending
            num_blocks = (send_size - 1) // self.Block_Size

            print(f"Flashing: {BY}{binary['name']}{NW}, send_size: {hex(send_size)}, start_addr: {hex(offset)}")

            self.StartFlash(uart, send_size, num_blocks, offset)
            self.WriteFlash(uart, binary["file"], data, num_blocks)
            self.FlashMD5(uart, data, starting_offset, binary_size)

            # Reset our data ptr
            self.data_idx = 0

            self.binary_idx += 1

        # End flashing and reboot
        self.EndFlash(uart)

    def StartFlash(self, uart: serial.Serial, size: int, num_blocks: int, offset: int):
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

        reply = self.WritePacketWaitForResponsePacket(uart, packet, self.FLASH_BEGIN)

        # Check for error
        if reply.data[-1] == 1 and reply.GetCommand() == self.FLASH_BEGIN:
            print(reply)
            raise Exception("Error occurred when starting flashing")

    def WriteFlash(self, uart: serial.Serial, file: str, data: bytes, num_blocks: int):
        packet_idx = 0
        current_idx = self.data_idx
        binary_size = len(data)

        print(f"Flashing: {BG}00.00{NW}%", end="\r")

        while current_idx < binary_size:
            # Create a flashing slip packet with the direction and command
            bin_packet = ESP32SlipPacket(0, self.FLASH_DATA)

            # Push on the data size which will be the block size
            bin_packet.PushData(self.Block_Size, 4)

            # Push the current bin_packet num aka sequence number
            bin_packet.PushData(packet_idx, 4)

            # Then some zeroes (32bit x 2 of zeros)
            bin_packet.PushData(0, 4)
            bin_packet.PushData(0, 4)

            data_bytes = data[current_idx : current_idx + self.Block_Size]

            if len(data_bytes) < self.Block_Size:
                # Pad the data to fit the block size
                data_bytes += bytes([0xFF] * (self.Block_Size - len(data_bytes)))

            # Push it all into the packet
            bin_packet.PushDataArray(data_bytes, "big")

            # Write the packet and wait for a reply
            reply = self.WritePacketWaitForResponsePacket(uart, bin_packet, self.FLASH_DATA, checksum=True)

            if reply == self.NO_REPLY or reply.GetCommand() != self.FLASH_DATA:
                print("")
                print(reply)
                if reply != self.NO_REPLY:
                    print(reply.GetCommand())
                print(f"Error occurred when writing address {current_idx}" f" of {file}")
                raise Exception("Error. Failed to write")

            print(f"Flashing: {BG}{(current_idx/binary_size) * 100:2.2f}{NW}%", end="\r")

            self.data_idx = current_idx

            # Move the file pointer
            current_idx += self.Block_Size

            # Increment the sequence number
            packet_idx += 1

        print(f"Flashing: {BG}100.00{NW}%")

    def EndFlash(self, uart: serial.Serial):
        packet = ESP32SlipPacket(0, self.FLASH_END)

        packet.PushData(0, 4)

        reply = self.WritePacketWaitForResponsePacket(uart, packet, self.FLASH_END)

        if reply == self.NO_REPLY or reply.GetCommand() != self.FLASH_END:
            print("Failed to restart board")

        print(f"Flashing: {BG}COMPLETE{NW}")

    def FlashMD5(self, uart: serial.Serial, data: bytes, address: int, size: int):
        packet = ESP32SlipPacket(0, self.SPI_FLASH_MD5)
        packet.PushData(address, 4)
        packet.PushData(size, 4)
        packet.PushData(0, 4)
        packet.PushData(0, 4)

        reply = self.WritePacketWaitForResponsePacket(uart, packet, self.SPI_FLASH_MD5)

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

    def WritePacket(self, uart: serial.Serial, packet: ESP32SlipPacket, checksum: bool = False):
        data = packet.SLIPEncode(checksum)
        uart.write(data)

    def WaitForResponsePacket(self, uart: serial.Serial, packet_type: int = -1):
        Timeout_Duration_s = 3

        rx_byte = bytes(1)
        in_bytes = []

        packet = None

        packet = ESP32SlipPacket()

        timeout = time.time() + Timeout_Duration_s

        # Read until we hit a end byte, otherwise, timeout
        while rx_byte[0] != ESP32SlipPacket.END:
            if time.time() > timeout:
                print("Timed out waiting for a start byte")
                return -1

            rx_byte = uart.read(1)

            if len(rx_byte) < 1:
                return -1

        # We have the start byte, append it to our bytes
        in_bytes.append(rx_byte)

        rx_byte = bytes(1)
        while rx_byte[0] != ESP32SlipPacket.END:
            if time.time() > timeout:
                return -1

            rx_byte = uart.read(1)

            if len(rx_byte) < 1:
                print("Timed out waiting for a start byte")

                return -1

            in_bytes.append(rx_byte)

            # Since this could in theory have double the size
            # from the encoding we'll only break if we are bigger than
            # double our max packet size
            if len(in_bytes) > self.Block_Size * 2:
                print("In bytes is too large")
                return -1

        # Packet is done, convert it
        packet.FromBytes(in_bytes)

        if packet.Length() > self.Block_Size:
            print("Packet too large")
            return -1

        return packet
