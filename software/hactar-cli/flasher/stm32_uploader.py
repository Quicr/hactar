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
from uploader import Uploader
from hactar_commands import command_map

# TODO fundamental issue with checking if we are synced.
# If we fail, we need to invalid the sync and do it again.


class STM32Uploader(Uploader):
    ACK = 0x79
    NACK = 0x1F
    NO_REPLY = -1

    Commands = SimpleNamespace(
        **{
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
            "readout_unprotect": 0x92,
        }
    )

    config_file: dict = None
    chip_config: dict = None

    def __init__(self, chip: str, binary_path: str):
        super().__init__(chip)

        self.Reset()

        self.binary_path = binary_path

        config_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "stm32_configurations.json")

        if not os.path.isfile(config_path):
            raise Exception(f"Failed to find configuration ${config_path} in program directory")

        self.configs_file = json.load(open(f"{config_path}"))

    def Reset(self):
        self.synced = False
        self.uid = None
        self.chip_config_config = None
        self.available_commands = None  # TODO
        self.use_exception = True

        self.erase_started = False
        self.sectors_deleted = []
        self.sector_idx = 0

        self.write_address = 0
        self.data_idx = 0

        self.last_verify_addr = 0
        self.last_verify_data_addr = 0

    def FlashFirmware(self, uart: serial.Serial) -> bool:
        self.synced = False
        self.uid = None

        # Open the binary first
        binary = open(self.binary_path, "rb").read()

        self.FlashSelect(uart)

        self.sectors = self.GetSectorsForFirmware(uart, len(binary))

        self.SendExtendedEraseMemory(uart, self.sectors, False, True, True)

        address = self.chip_config["usr_start_addr"] + self.write_address
        data_offset = self.data_idx
        self.SendWriteMemory(uart, binary, data_offset, address)

        # UI chip is controlled by mgmt chip, so sending a go only matters on the mgmt chip.
        if self.chip == "mgmt":
            self.SendGo(uart, self.chip_config["usr_start_addr"])

        return True

    def FlashSelect(self, uart):
        if self.chip == "mgmt":
            uart.parity = serial.PARITY_EVEN
            print(f"Updated uart to parity: {BB}EVEN{NW}")
            print(f"User, put Hactar into bootloader mode!!")
            input("Press enter once it is done...")
            uart.reset_input_buffer()
        elif self.chip == "ui":
            uart.write(command_map["flash ui"])
            print(f"Sent command to flash UI")

            uart.flush()

            self.TryPattern(uart, uart_utils.OK, 1, 5)
            print(f"Flash UI command: {BG}CONFIRMED{NW}")

            print(f"Update uart to parity: {BB}EVEN{NW}")
            uart.parity = serial.PARITY_EVEN

            self.TryPattern(uart, uart_utils.READY, 1, 5)
            print(f"Flash UI: {BB}READY{NW}")

            uart.flush()
            uart.reset_input_buffer()

            print(f"Activating UI Upload Mode: {BG}SUCCESS{NW}")

            time.sleep(1)

    def SetChipConfig(self, pid: int):
        print(f"Retrieving configurations for chip ID: {BC}{hex(pid)}{NW}")
        # Get the config from file returns an error if its missing
        if str(pid) not in self.configs_file.keys():
            raise Exception(f"Chip ID: {pid}. Not in configuration file")

        self.chip_config = self.configs_file[str(pid)]

    def CalculateChecksum(self, arr: [int]):
        return functools.reduce(lambda a, b: a ^ b, arr).to_bytes(1, "big")

    def GetSectorsForFirmware(self, uart, data_len: int):
        """Gets the sectors, starting from zero, that the firmware will fill."""
        # Attempt to get the chip uid
        self.CheckInit(uart)

        if self.chip_config == None:
            raise Exception("Chip configuration has not been set")

        sectors = 0
        total_sectors = len(self.chip_config["sectors"])
        while data_len > 0:
            data_len -= self.chip_config["sectors"][sectors]["size"]
            sectors += 1

            if sectors >= total_sectors:
                raise Exception("Not enough flash memory for binary")

        return [i for i in range(sectors)]

    def HandleReply(self, reply_res, caller, exception_str: str, output_suc=False):
        if reply_res == self.NACK:
            print(f"{caller}: {BR}FAILED{NW}")
            self.synced = False
            if self.use_exception:
                raise Exception(f"{exception_str} - type: NACK")
            else:
                return False
        elif reply_res == self.NO_REPLY:
            print(f"{caller}: {BY}NO REPLY{NW}")
            self.synced = False
            if self.use_exception:
                raise Exception(f"{exception_str} - type: NO REPLY")
            else:
                return False

        if output_suc:
            print(f"{caller}: {BG}SUCCESSFUL{NW}")
        return True

    def SendSync(self, uart: serial.Serial, retry_num: int = 5):
        """Sends the sync byte 0x7F and waits for an self.ACK from the device."""

        # reply = uart_utils.WriteByteWaitForACK(uart, self.Commands.sync, retry_num, False)

        data = [self.Commands.sync]

        while retry_num > 0:
            retry_num -= 1

            uart.write(bytes(data))

            reply = uart_utils.GetBytes(uart, 1)

            if reply == -1:
                continue

            if reply == uart_utils.ACK:
                break

        self.synced = self.HandleReply(reply, "Sync", "Failed to activate device", True)

        return self.synced

    def CheckSync(self, uart: serial.Serial):
        if self.synced:
            return True

        return self.SendSync(uart)

    def CheckUid(self, uart: serial.Serial):
        if self.uid != None:
            return True

        return self.SendGetID(uart)

    def CheckInit(self, uart: serial.Serial):
        if not self.CheckSync(uart):
            return False

        if not self.CheckUid(uart):
            return False

        return True

    def SendGetID(self, uart: serial.Serial, retry_num: int = 5):
        """Sends the GetID command and it's compliment.
        Command = 0x02
        Compliment = 0xFD = 0x02 ^ 0xFF

        returns pid
        """

        self.CheckSync(uart)

        res = uart_utils.WriteByteWaitForACK(uart, self.Commands.get_id, 5)

        self.HandleReply(res, "Get ID command", "ACK was not received")

        # Get the number of incoming bytes
        num_bytes = uart_utils.TryGetBytes(uart, 1)

        # Get the pid which should be N+1 bytes
        pid_bytes = uart_utils.TryGetBytes(uart, num_bytes + 1)

        # Wait for an ack
        res = uart_utils.TryGetBytes(uart, 1)
        self.HandleReply(res, "GetID PID ACK", "Failed to get PID", False)

        self.uid = int.from_bytes(bytes(pid_bytes), "big")
        print(f"Chip ID: {BC}{hex(self.uid)}{NW}")

        self.SetChipConfig(self.uid)

        return True

    def SendGet(self, uart: serial.Serial, retry_num: int = 5):
        """Sends the Get command and it's compliment.
        Command = 0x00
        Compliment = 0xFF = 0x00 ^ 0xFF

        returns all available commands
        """
        self.CheckInit(uart)

        reply = uart_utils.WriteByteWaitForACK(uart, self.Commands.get, retry_num)

        self.HandleReply(
            reply,
            "Get Commands",
            "Failed to retrieve commands available to this chip",
            False,
        )
        # Read the next byte which should be the num of bytes - 1
        #  for some reason they minus off one even tho 2 bytes come in
        num_bytes = uart_utils.TryGetBytes(uart, 1)

        bootloader_verison = uart_utils.TryGetBytes(uart, 1)

        # Get all of the available commands
        recv_commands = uart_utils.TryGetBytes(uart, num_bytes)

        # Wait for an ack
        reply = uart_utils.TryGetBytes(uart, 1)

        # Parse the commands
        available_commands = []
        for key, val in self.Commands.__dict__.items():
            for cmd in recv_commands:
                if val == cmd:
                    available_commands.append(key)

        self.HandleReply(reply, "Get Commands Receive", "Failed to get available commands", False)

        print(f"Bootloader version: {BM}{bootloader_verison}{NW}")

        print(f"{num_bytes} available commands: ", end="")
        for i, cmd in enumerate(available_commands):
            if i == len(available_commands) - 1:
                print(cmd)
            else:
                print(cmd, end=", ")

        return available_commands

    def SendReadMemory(self, uart: serial.Serial, address: bytes, num_bytes: int, retry_num: int = 5):
        """Sends the Read Memory command and it's compliment.
        Command = 0x11
        Compliment = 0xEE = 0x11 ^ 0xFF

        returns [bytes] contents of memory [address:address + num_bytes]

        """
        self.CheckInit(uart)

        reply = uart_utils.WriteByteWaitForACK(uart, self.Commands.read_memory, retry_num)

        self.HandleReply(reply, "Read memory command", "Failed to read memory command", False)

        memory_addr = address + self.CalculateChecksum(address)

        # Write the memory we want to read from and the checksum
        reply = uart_utils.WriteBytesWaitForACK(uart, memory_addr, 1)

        self.HandleReply(
            reply,
            "Read memory set address",
            "Failed to set address for read memory command",
            False,
        )

        reply = uart_utils.WriteByteWaitForACK(uart, num_bytes - 1, 1)
        self.HandleReply(
            reply,
            "Read memory num bytes available",
            "Failed to read number of bytes for memory command",
            False,
        )

        recv_data = uart_utils.TryGetBytes(uart, num_bytes)
        if type(recv_data) is int:
            recv_data = [recv_data]

        return recv_data

    def SendExtendedEraseMemory(
        self, uart: serial.Serial, sectors_to_delete: [int], special: bool, fast_verify: bool, recover: bool
    ):
        """Sends the Erase memory command and it's compliment.
        Command = 0x44
        Compliment = 0xBB = 0x44 ^ 0xFF

        returns true
        """
        if len(sectors_to_delete) < 1:
            raise Exception("Cannot delete zero sectors")

        if self.sectors_deleted == sectors_to_delete:
            print("Already deleted sectors, skipping")
            return True

        self.CheckInit(uart)

        print(f"Erase: {BB}STARTED{NW}")
        print(f"Erase  {BB}SECTORS{NW} {NY}{sectors_to_delete}{NW}")
        print(f"Erased {BB}SECTORS{NW} {NY}{self.sectors_deleted}{NW}", end="")

        while self.sectors_deleted != sectors_to_delete:
            sector = sectors_to_delete[self.sector_idx]

            reply = uart_utils.WriteByteWaitForACK(uart, self.Commands.extended_erase, 1)

            self.HandleReply(reply, "\nExtended Erase", "Extended erase failed")

            # Number of sectors starts at 0x00 0x00. So 0x00 0x00 means
            # delete 1 sector
            # An exception is thrown if the number is bigger than 2 bytes
            num_sectors = [int(0).to_bytes(2, "big")]

            # Turn the sectors received into two byte elements
            sector_in_bytes = [sector.to_bytes(2, "big")]

            # Join the num sectors and the byte sectors into one list
            data = b"".join(num_sectors + sector_in_bytes)

            # Calculate the checksum
            checksum = [self.CalculateChecksum(data)]

            # Join all the data together
            data = b"".join(num_sectors + sector_in_bytes + checksum)

            print(f"\rErased {BB}SECTORS{NW} {NY}{self.sectors_deleted}{NW}", end="")

            # Give more time to reply with the deleted sectors
            uart.timeout = 5
            reply = uart_utils.WriteBytesWaitForACK(uart, data, 1)
            # Revert the change back to two seconds
            uart.timeout = 2
            self.HandleReply(reply, "\nErase memory", "Failed to Erase", False)

            # Update the number of deleted sectors and remove the deleted sector
            self.sectors_deleted.append(sector)

            self.sector_idx += 1

        print(f"\rErased {BB}SECTORS{NW} {NY}{self.sectors_deleted}{NW}")

        # TODO verify at the same time
        if fast_verify:
            return self.FastEraseVerify(uart, self.sectors_deleted)
        else:
            total_bytes = 0
            for sector in self.sectors_deleted:
                total_bytes += self.chip_config["sectors"][sector]["size"]
                print(total_bytes)

            data = bytes([255] * total_bytes)
            for verify_status in self.FullVerify(uart, data):
                print(
                    f"Verifying erase: {BG}{verify_status['percent']:.2f}" f"{NW}% verified",
                    end="\r",
                )

                if verify_status["failed"]:
                    print(f"\nVerifying Erase: {BR}Failed to verify address " f"{hex(verify_status['addr'])}{NW}")
                    return False

            print(f"Verifying erase: {BG}100{NW}% verified")

        return True

    def FullVerify(self, uart: serial.Serial, data: bytes):
        """Takes a chunk of memory and verifies it sequentially from the start addr"""
        # TODO recoverable
        self.CheckInit(uart)

        Max_Num_Bytes = 256
        addr = self.chip_config["usr_start_addr"]
        data_addr = 0
        data_len = len(data)
        verify_status = {"addr": addr, "failed": 0, "percent": 0}

        while data_addr < data_len:
            verify_status["addr"] = addr
            verify_status["percent"] = (data_addr / data_len) * 100
            yield verify_status

            chunk = data[data_addr : data_addr + Max_Num_Bytes]
            if not self.FlashCompare(uart, chunk, addr):
                verify_status["failed"] = True
                break

            addr += len(chunk)
            data_addr += len(chunk)

        verify_status["percent"] = 100
        yield verify_status

    def FastEraseVerify(self, uart: serial.Serial, sectors: [int]):
        # TODO recoverable
        self.CheckInit(uart)
        print(f"Erase Verify: {BB}BEGIN{NW}")
        Mem_Bytes_Sz = 256
        expected_mem = bytes([255] * Mem_Bytes_Sz)
        num_sectors = len(sectors)
        num_sectors_verified = 0

        for sector in sectors:
            # Print how much has been verified
            percent_verified = int((num_sectors_verified / num_sectors) * 100)
            print(f"Verifying erase: {BG}{percent_verified:2}" f"{NW}% verified", end="\r")

            # Get the next address to verify
            addr = self.chip_config["sectors"][sector]["addr"]
            # Verify the flash at that memory location
            if not (self.FlashCompare(uart, expected_mem, addr)):
                print(f"Verifying: {BR}Failed to verify sector " f"[{sector}]{NW}")
                return False

            # Verified that sector, continue on
            num_sectors_verified += 1
        print(f"Verifying erase: {BG}100{NW}% verified")
        print(f"Erase: {BG}COMPLETE{NW}")

        return True

    def FlashCompare(self, uart: serial.Serial, chunk: bytes, addr: int):
        """Compares a byte array to the flash at the provided addr.
        True if equal, False otherwise
        """
        self.CheckInit(uart)

        # Get the flash chunk from the address of equal size to chunk
        Max_Attempts = 10
        read_count = 0
        mem = [0] * len(chunk)

        while mem != chunk and read_count < Max_Attempts:
            mem = bytes(self.SendReadMemory(uart, addr.to_bytes(4, "big"), len(chunk)))
            read_count += 1

        return chunk == mem

    def SendGo(self, uart: serial.Serial, address: int, num_retry: int = 1):
        """Sends the Go command according to the passed in address"""
        self.CheckInit(uart)

        reply = uart_utils.WriteByteWaitForACK(uart, self.Commands.go, num_retry)

        self.HandleReply(reply, "Go Command", "Failed to send Go Command")

        # Convert the address to bytes
        addr_bytes = (address).to_bytes(4, "big")

        # Get the checksum for the address
        checksum = self.CalculateChecksum(addr_bytes)

        # Append the checksum to the address bytes
        addr_bytes += checksum

        # Send the data and get a reply
        reply = uart_utils.WriteBytesWaitForACK(uart, addr_bytes, num_retry)

        self.HandleReply(
            reply,
            f"Jump to address {BB}{hex(address)}{BW}",
            f"Failed to jump to address {hex(address)}",
            True,
        )

    def SendWriteMemory(self, uart: serial.Serial, data: bytes, data_offset: int, address: int):
        """Sends the Write memory command and it's compliment.
        Then writes the address bytes and its checksum
        Then writes the entire data stream
        Command = 0x31
        Compliment = 0xCE = 0x31 ^ 0xFF

        returns true if successful
        """

        uart.timeout = 1

        Max_Num_Bytes = 256
        total_bytes = len(data)

        current_idx = data_offset
        current_write_addr = address

        print(f"Write to Memory: {BB}STARTED{NW}")
        print(f"Address: {BW}{current_write_addr:04x}{NW}")
        print(f"Byte Stream Size: {BW}{total_bytes}{NW}")

        # Either a exception or a positive return will break out
        self.CheckInit(uart)

        while current_idx < total_bytes:
            percent_flashed = (current_idx / total_bytes) * 100
            print(f"\rFlashing: {BG}{percent_flashed:2.2f}{NW}%", end="")

            reply = uart_utils.WriteByteWaitForACK(uart, self.Commands.write_memory, 1)
            self.HandleReply(reply, "Write Command", "Failed to send Write command", False)

            # Send the address and the checksum
            checksum = self.CalculateChecksum(current_write_addr.to_bytes(4, "big"))
            write_address_bytes = current_write_addr.to_bytes(4, "big") + checksum
            reply = uart_utils.WriteBytesWaitForACK(uart, write_address_bytes, 1)

            self.HandleReply(
                reply,
                "\nWrite address bytes",
                "Failed to write the address bytes to the chip",
            )

            # Get the contents of the binary
            chunk = data[current_idx : current_idx + Max_Num_Bytes]

            # Get the chunk size before padding
            chunk_size = len(chunk)

            # Pad the chunk to be a multiple of 4 bytes
            while len(chunk) % 4 != 0:
                chunk += bytes([0xFF])

            # Front append the number of bytes to be received. 0 start
            chunk = (len(chunk) - 1).to_bytes(1, "big") + chunk

            # Add the checksum
            chunk = chunk + self.CalculateChecksum(chunk)

            # Write the chunk to the chip
            reply = uart_utils.WriteBytesWaitForACK(uart, chunk, 1)
            self.HandleReply(
                reply,
                f"\nWrite to address {hex(current_write_addr)}",
                f"Failed to write to address {hex(current_write_addr)}",
            )

            self.data_idx = current_idx
            self.write_address = current_write_addr

            # Shift over by the amount of byte stream bytes were sent
            current_idx += chunk_size
            current_write_addr += chunk_size

        # Don't need to calculate it here it finished
        print(f"\rFlashing: {BG}100.00{NW}%")

        # TODO verify function?
        while self.last_verify_data_addr < total_bytes:
            percent_verified = (self.last_verify_data_addr / total_bytes) * 100
            print(
                f"\rVerifying write: {BG}{percent_verified:2.2f}{NW}" f"% verified",
                end="",
            )

            chunk = data[self.last_verify_data_addr : self.last_verify_data_addr + Max_Num_Bytes]
            mem = bytes(self.SendReadMemory(self.last_verify_addr.to_bytes(4, "big"), len(chunk)))

            if chunk != mem:
                raise Exception(f"\nFailed to verify at memory address {hex(self.last_verify_addr)}")

            self.last_verify_addr += len(mem)
            self.last_verify_data_addr += len(chunk)

        print(f"\rVerifying write: {BG}100.00{NW}% verified")
        print(f"Write: {BG}COMPLETE{NW}")

        return True
