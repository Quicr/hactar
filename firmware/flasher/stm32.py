import serial
import time
import functools
import os
import sys
import json
from types import SimpleNamespace
import uart_utils
from ansi_colours import BW, BC, BG, BR, BB, BY, BM, NW, NY

# https://www.st.com/resource/en/application_note/an3155-usart-protocol-used-in-the-stm32-bootloader-stmicroelectronics.pdf

# TODO Put the sector verify code somewhere
# TODO clean up full verify and write flash

class stm32_flasher:
    ACK = 0x79
    READY = 0x80
    NACK = 0x1F
    NO_REPLY = -1

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

    config_file: dict = None
    chip: dict = None

    def __init__(self, uart: serial.Serial):
        self.uart = uart

        config_path = f"{os.path.dirname(os.path.abspath(sys.argv[0]))}/"
        config_path += "stm32_configurations.json"

        if (not os.path.isfile(config_path)):
            raise Exception(
                "Failed to find configuration file in program directory")

        self.config_file = json.load(open(f"{config_path}"))

    def SetChipConfig(self, pid: int):
        print(f"Retrieving configurations for chip ID: {BC}{hex(pid)}{NW}")
        # Get the config from file returns an error if its missing
        if str(pid) not in self.config_file.keys():
            raise Exception(f"Chip ID: {pid}. Not in configuration file")

        self.chip = self.config_file[str(pid)]

    def CalculateChecksum(self, arr: [int]):
        return functools.reduce(lambda a, b: a ^ b, arr).to_bytes(1, "big")

    def GetSectorsForFirmware(self, data_len: int):
        """ Gets the sectors, starting from zero, that the firmware will fill.

        """
        sectors = 0
        total_sectors = len(self.chip["sectors"])
        while (data_len > 0):
            data_len -= self.chip["sectors"][sectors]["size"]
            sectors += 1

            if (sectors >= total_sectors):
                raise Exception("Not enough flash memory for binary")

        return [i for i in range(sectors)]

    def SendSync(self, retry_num: int = 5):
        """ Sends the sync byte 0x7F and waits for an self.ACK from the device.
        """

        reply = uart_utils.WriteByteWaitForACK(self.uart, self.Commands.sync,
                                               retry_num, False)

        # Check if ack or self.nack
        if (reply == self.NACK):
            print(f"Activating device: {BR}FAILED{NW}")
            raise Exception("Failed to Activate device")
        elif (reply == -1):
            print(f"Activating device: {BY}NO REPLY{NW}")
            raise Exception("Failed to Activate device")

        print(f"Activating device: {BG}SUCCESS{NW}")

    def SendGetID(self, retry_num: int = 5):
        """ Sends the GetID command and it's compliment.
            Command = 0x02
            Compliment = 0xFD = 0x02 ^ 0xFF

            returns pid
        """
        res = uart_utils.WriteByteWaitForACK(
            self.uart, self.Commands.get_id, 1)

        if res == self.NACK:
            raise Exception("NACK was received during GetID")
        elif res == -1:
            raise Exception("No reply was received during GetID")

        # Read the next byte which should be the num of bytes - 1

        num_bytes = uart_utils.WaitForBytesExcept(self.uart, 1)

        # NOTE there is a bug in the bootloader for stm, or there is a
        # inaccuracy in the an3155 datasheet that says that a single ACK
        # is sent after receiving the get_id command.
        # But in reality 90% of the time two ACK bytes are sent
        if (num_bytes == self.ACK):
            num_bytes = uart_utils.WaitForBytesExcept(self.uart, 1)
        elif (num_bytes == self.NACK):
            raise Exception("NACK was received while trying to get the "
                            "num bytes in GetID")

        # Get the pid which should be N+1 bytes
        pid_bytes = uart_utils.WaitForBytesExcept(self.uart, num_bytes+1)

        # Wait for an ack
        res = uart_utils.WaitForBytesExcept(self.uart, 1)
        if (res == self.NACK):
            raise Exception("A NACK was received at the end of GetID")
        elif res == -1:
            raise Exception("No reply was received during GetID")

        pid = int.from_bytes(bytes(pid_bytes), "big")
        print(f"Chip ID: {BC}{hex(pid)}{NW}")

        return pid

    def SendGet(self, retry_num: int = 5):
        """ Sends the Get command and it's compliment.
            Command = 0x00
            Compliment = 0xFF = 0x00 ^ 0xFF

            returns all available commands
        """
        reply = uart_utils.WriteByteWaitForACK(
            self.uart, self.Commands.get, retry_num)
        if (reply == self.NACK):
            raise Exception("NACK was received during Get Commands")

        # Read the next byte which should be the num of bytes - 1
        #  for some reason they minus off one even tho 2 bytes come in
        num_bytes = uart_utils.WaitForBytesExcept(self.uart, 1)

        bootloader_verison = uart_utils.WaitForBytesExcept(self.uart, 1)

        # Get all of the available commands
        recv_commands = uart_utils.WaitForBytesExcept(self.uart, num_bytes)

        # Wait for an ack
        reply = uart_utils.WaitForBytesExcept(self.uart, 1)

        # Parse the commands
        available_commands = []
        for key, val in self.Commands.__dict__.items():
            for cmd in recv_commands:
                if (val == cmd):
                    available_commands.append(key)

        if (reply == self.NACK):
            raise Exception("A self.NACK was received at the end of GetID")

        print(f"Bootloader version: {BM}{bootloader_verison}{NW}")

        print(f"{num_bytes} available commands: ", end="")
        for i, cmd in enumerate(available_commands):
            if (i == len(available_commands) - 1):
                print(cmd)
            else:
                print(cmd, end=", ")

        return available_commands

    def SendReadMemory(self, address: bytes,
                       num_bytes: int, retry_num: int = 5):
        """ Sends the Read Memory command and it's compliment.
            Command = 0x11
            Compliment = 0xEE = 0x11 ^ 0xFF

            returns [bytes] contents of memory [address:address + num_bytes]

        """
        reply = uart_utils.WriteByteWaitForACK(self.uart,
                                               self.Commands.read_memory,
                                               retry_num)
        if (reply == self.NACK):
            raise Exception("NACK was received during Read Memory command")

        memory_addr = address + self.CalculateChecksum(address)

        # Write the memory we want to read from and the checksum
        reply = uart_utils.WriteBytesWaitForACK(self.uart, memory_addr, 1)
        if (reply == self.NACK):
            raise Exception("NACK was received after sending memory address to"
                            " read")

        reply = uart_utils.WriteByteWaitForACK(self.uart, num_bytes-1, 1)
        if (reply == self.NACK):
            raise Exception("NACK was received after sending how many bytes to"
                            " read")

        recv_data = uart_utils.WaitForBytesExcept(self.uart, num_bytes)
        if type(recv_data) is int:
            recv_data = [recv_data]

        return recv_data

    def SendExtendedEraseMemory(self, sectors: [int],
                                special: bool,
                                fast_verify: bool = True,
                                retry_num: int = 5):
        """ Sends the Erase memory command and it's compliment.
            Command = 0x44
            Compliment = 0xBB = 0x44 ^ 0xFF

            returns self.ACK/self.NACK/0
        """
        reply = uart_utils.WriteByteWaitForACK(
            self.uart, self.Commands.extended_erase, 5)
        if (reply == self.NACK):
            raise Exception("NACK was received after sending erase command")
        elif (reply == self.NO_REPLY):
            raise Exception("No reply was received after sending erase"
                            " command")

        # Number of sectors starts at 0x00 0x00. So 0x00 0x00 means
        # delete 1 sector
        # An exception is thrown if the number is bigger than 2 bytes
        # TODO delete one sector at a time and report back with an ACK
        num_sectors = [(len(sectors) - 1).to_bytes(2, "big")]

        # Turn the sectors received into two byte elements
        byte_sectors = [x.to_bytes(2, "big") for x in sectors]

        # Join the num sectors and the byte sectors into one list
        data = b''.join(num_sectors + byte_sectors)

        # Calculate the checksum
        checksum = [self.CalculateChecksum(data)]

        # Join all the data together
        data = b''.join(num_sectors + byte_sectors + checksum)

        print(f"Erase: Sectors {NY}{sectors}{NW}")
        print(f"Erase: {BB}STARTED{NW}")

        # Give more time to reply with the deleted sectors
        self.uart.timeout = 30
        reply = uart_utils.WriteBytesWaitForACK(self.uart, data, 1)
        # Revert the change back to two seconds
        self.uart.timeout = 2
        if (reply == -1):
            raise Exception("Failed to erase, no reply received")
        elif (reply == self.NACK):
            raise Exception("Failed to erase")


        if (fast_verify):
            return self.FastEraseVerify(sectors)
        else:
            total_bytes = 0
            for sector in sectors:
                total_bytes += self.chip["sectors"][sector]["size"]
                print(total_bytes)

            data = bytes([255] * total_bytes)
            for verify_status in self.FullVerify(data):
                print(f"Verifying erase: {BG}{verify_status['percent']:.2f}"
                        f"{NW}% verified", end="\r")

                if (verify_status["failed"]):
                    print(f"\nVerifying Erase: {BR}Failed to verify address "
                          f"{hex(verify_status['addr'])}{NW}")
                    return False

            print(f"Verifying erase: {BG}100{NW}% verified")

        return True

        # mem_bytes_sz = 256
        # expected_mem = [255] * mem_bytes_sz
        # mem = [0] * mem_bytes_sz
        # read_count = 0
        # bytes_verified = 0
        # total_bytes_to_verify = 0
        # for sector_idx in sectors:
        #     total_bytes_to_verify += self.chip["sectors"][sector_idx]["size"]
        # percent_verified = int((bytes_verified / total_bytes_to_verify)*100)

        # for sector_idx in sectors:
        #     memory_address = self.chip["sectors"][sector_idx]["addr"]
        #     end_of_sector = (self.chip["sectors"][sector_idx]["addr"] +
        #         self.chip["sectors"][sector_idx]["size"])

        #     while (memory_address != end_of_sector):
        #         read_count = 0
        #         mem = [0] * mem_bytes_sz
        #         percent_verified = int(
        #             (bytes_verified / total_bytes_to_verify)*100)
        #         print(f"Verifying erase: {BG}{percent_verified:2}"
        #               f"{NW}% verified", end="\r")
        #         while (mem != expected_mem) and read_count != 10:
        #             mem = self.SendReadMemory(memory_address.to_bytes(
        #                 4, "big"),
        #                 mem_bytes_sz)
        #             read_count += 1
        #             if (mem != expected_mem):
        #                 print(f"Sector not verified {sector_idx} retry"
        #                       f" {read_count}")

        #         if (read_count == 10 and mem != expected_mem):
        #             print(f"Verifying: {BR}Failed to verify sector "
        #                   f"[{sector_idx}]{NW}")
        #             return False

        #         memory_address += mem_bytes_sz
        #         bytes_verified += mem_bytes_sz

        # # Don't actually need to do the math here
        # print(f"Verifying erase: {BG}100{NW}% verified")
        # print(f"Erase: {BG}COMPLETE{NW}")
        # return True

    def FullVerify(self, data:bytes):
        """ Takes a chunk of memory and verifies it sequentially from the start addr

        """
        Max_Num_Bytes = 256
        addr = self.chip["usr_start_addr"]
        data_addr = 0
        data_len = len(data)
        verify_status = {"addr": addr, "failed": 0, "percent": 0}

        while (data_addr < data_len):
            verify_status['addr'] = addr
            verify_status['percent'] = (data_addr / data_len) * 100
            yield verify_status

            chunk = data[data_addr:data_addr+Max_Num_Bytes]
            if (not self.FlashCompare(chunk, addr)):
                verify_status['failed'] = True
                break

            addr += len(chunk)
            data_addr += len(chunk)

        verify_status['percent'] = 100
        yield verify_status

    def FastEraseVerify(self, sectors:[int]):
        print(f"Erase Verify: {BB}BEGIN{NW}")
        Mem_Bytes_Sz = 256
        expected_mem = bytes([255] * Mem_Bytes_Sz)
        num_sectors = len(sectors)
        num_sectors_verified = 0

        for sector in sectors:
            # Print how much has been verified
            percent_verified = int(
                (num_sectors_verified / num_sectors)*100)
            print(f"Verifying erase: {BG}{percent_verified:2}"
                    f"{NW}% verified", end="\r")

            # Get the next address to verify
            addr = self.chip["sectors"][sector]["addr"]

            # Verify the flash at that memory location
            if not (self.FlashCompare(expected_mem, addr)):
                print(f"Verifying: {BR}Failed to verify sector "
                        f"[{sector}]{NW}")
                return False

            # Verified that sector, continue on
            num_sectors_verified += 1
        print(f"Verifying erase: {BG}100{NW}% verified")
        print(f"Erase: {BG}COMPLETE{NW}")

        return True


    def FlashCompare(self, chunk:bytes, addr:int):
        """ Compares a byte array to the flash at the provided addr.
            True if equal, False otherwise
        """
        # Get the flash chunk from the address of equal size to chunk
        Max_Attempts = 10
        read_count = 0
        mem = [0] * len(chunk)

        while (mem != chunk and read_count < Max_Attempts):
            mem = bytes(self.SendReadMemory(addr.to_bytes(4, "big"), len(chunk)))
            read_count += 1

        return chunk == mem

    def SendGo(self, address: int, num_retry: int = 1):
        """ Sends the Go command according to the passed in address
        """

        reply = uart_utils.WriteByteWaitForACK(self.uart,
            self.Commands.go, num_retry)

        if (reply == self.NACK):
            raise Exception("NACK received during Go command")
        elif (reply == self.NO_REPLY):
            raise Exception("NO REPLY during Go Command")

        # Convert the address to bytes
        addr_bytes = (address).to_bytes(4, "big")

        # Get the checksum for the address
        checksum = self.CalculateChecksum(addr_bytes)

        # Append the checksum to the address bytes
        addr_bytes += checksum

        # Send the data and get a reply
        reply = uart_utils.WriteBytesWaitForACK(self.uart,
                                                addr_bytes,
                                                num_retry)

        if (reply == self.NACK):
            raise Exception("NACK received after sending jump address")
        elif (reply == self.NO_REPLY):
            raise Exception("NO REPLY after sending jump address")

        print(f"Successfully jumped to address: {BB}{hex(address)}{BW}")

    def SendWriteMemory(self, data: bytes, address: int,
                        num_retry: int = 5):
        """ Sends the Write memory command and it's compliment.
            Then writes the address bytes and its checksum
            Then writes the entire data stream
            Command = 0x31
            Compliment = 0xCE = 0x31 ^ 0xFF

            returns self.ACK if successful
        """

        Max_Num_Bytes = 256

        # Send the address and the checksum
        addr = address
        file_addr = 0

        percent_flashed = 0

        total_bytes = len(data)
        self.uart.timeout = 5

        print(f"Write to Memory: {BB}STARTED{NW}")
        print(f"Address: {BW}{address:04x}{NW}")
        print(f"Byte Stream Size: {BW}{total_bytes}{NW}")

        while file_addr < total_bytes:
            percent_flashed = (file_addr / total_bytes) * 100
            print(f"Flashing: {BG}{percent_flashed:2.2f}{NW}%", end="\r")

            reply = uart_utils.WriteByteWaitForACK(
                self.uart, self.Commands.write_memory, 1)
            if (reply == self.NACK):
                raise Exception("NACK was received after sending write "
                                "command")
            elif (reply == -1):
                raise Exception("No reply was received after sending write "
                                "command")

            checksum = self.CalculateChecksum(addr.to_bytes(4, "big"))
            write_address_bytes = addr.to_bytes(4, "big") + checksum
            reply = uart_utils.WriteBytesWaitForACK(self.uart,
                                                    write_address_bytes, 1)

            if (reply == self.NACK):
                print("")
                for b in write_address_bytes:
                    print(b)
                raise Exception("NACK was received after sending write"
                                f"address bytes command for address"
                                f"{addr:04x}")
            elif (reply == -1):
                raise Exception("No reply was received after sending write"
                                f"address bytes command for address"
                                f"{addr:04x}")

            # Get the contents of the binary
            chunk = data[file_addr:file_addr+Max_Num_Bytes]
            # Chunk size before extra bytes are appended
            chunk_size = len(chunk)

            # Pad the chunk to be a multiple of 4 bytes
            while len(chunk) % 4 != 0:
                chunk += bytes([0xFF])

            # Front append the number of bytes to be received. 0 start
            chunk = (len(chunk) - 1).to_bytes(1, "big") + chunk

            # Add the checksum
            chunk = chunk + self.CalculateChecksum(chunk)

            # Write the chunk to the chip
            reply = uart_utils.WriteBytesWaitForACK(self.uart, chunk, 1)
            if (reply == self.NACK):
                raise Exception(f"NACK was received while writing to "
                                f"addr: {addr}")
            elif (reply == -1):
                raise Exception(f"No reply was received while writing to "
                                f"addr: {addr}")

            # Shift over by the amount of byte stream bytes were sent
            file_addr += chunk_size
            addr += chunk_size

        # Don't need to calculate it here it finished
        print(f"Flashing: {BG}100.00{NW}%")

        # Verify the written memory by reading the size of the file
        addr = address
        file_addr = 0

        while file_addr < total_bytes:
            percent_verified = (file_addr / total_bytes)*100
            print(f"Verifying write: {BG}{percent_verified:2.2f}{NW}"
                  f"% verified", end="\r")

            chunk = data[file_addr:file_addr+Max_Num_Bytes]
            mem = bytes(self.SendReadMemory(addr.to_bytes(4, "big"),
                        len(chunk)))

            if chunk != mem:
                raise Exception(
                    f"Failed to verify at memory address {hex(addr)}")

            addr += len(mem)
            file_addr += len(chunk)

        print(f"Verifying write: {BG}100.00{NW}% verified")
        print(f"Write: {BG}COMPLETE{NW}")

        return self.ACK

    def ProgramSTM(self, chip: str, binary_path: str):
        # Determine which chip will be flashed
        uart_utils.FlashSelection(self.uart, chip)

        self.SendSync(2)

        # Sometimes a small delay is required otherwise it won't
        # continue on correctly
        time.sleep(0.5)

        # Get the id of the chip
        uid = self.SendGetID(1)

        # Set the configuration based on the id
        self.SetChipConfig(uid)

        # Get all of the available commands
        # TODO use if needed.
        # self.SendGet()

        # Check if memory can be read.
        # mem = self.SendReadMemory(self.chip["usr_start_addr"].to_bytes(4, "big"), 256)
        # print(f"Reading memory: {BG}SUCCESSFUL{NW}")
        # print(f"Number of bytes read from memory: {BM}{len(mem)}\
        #     {NW}")

        # Get the firmware
        firmware = open(binary_path, "rb").read()

        # Get the size of memory that we need to erase
        sectors = self.GetSectorsForFirmware(len(firmware))

        self.SendExtendedEraseMemory(sectors, False)

        self.SendWriteMemory(firmware, self.chip["usr_start_addr"], 5)

        if (chip == "mgmt"):
            self.SendGo(self.chip["usr_start_addr"])

        return True
