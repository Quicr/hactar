import serial
import time
import functools
from types import SimpleNamespace
# https://www.manualslib.com/download/1764455/St-An3155.html


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

    # Depending on the sector, we can add the bytes together iteratively.
    User_Sector_Start_Address = 0x08000000

    Sectors = [
        SimpleNamespace(**{"size": 0x004000,   "addr": 0x08000000}),
        SimpleNamespace(**{"size": 0x004000,   "addr": 0x08004000}),
        SimpleNamespace(**{"size": 0x004000,   "addr": 0x08008000}),
        SimpleNamespace(**{"size": 0x004000,   "addr": 0x0800C000}),
        SimpleNamespace(**{"size": 0x010000, "addr": 0x08010000}),
        SimpleNamespace(**{"size": 0x020000, "addr": 0x08020000}),
        SimpleNamespace(**{"size": 0x020000, "addr": 0x08040000}),
        SimpleNamespace(**{"size": 0x020000, "addr": 0x08060000}),
        SimpleNamespace(**{"size": 0x020000, "addr": 0x08080000}),
        SimpleNamespace(**{"size": 0x020000, "addr": 0x080A0000}),
        SimpleNamespace(**{"size": 0x020000, "addr": 0x080C0000}),
        SimpleNamespace(**{"size": 0x020000, "addr": 0x080E0000}),
    ]

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

    def __init__(self, uart: serial.Serial):
        self.uart = uart

    def CalculateChecksum(self, arr: [int]):
        return functools.reduce(lambda a, b: a ^ b, arr).to_bytes(1, "big")

    def WriteReadBytes(self, write_buff: bytes,
                       read_num: int, retry_num=5):
        while (retry_num > 0):
            retry_num -= 1
            self.uart.write(write_buff)

            reply = self.WaitForBytesExcept(read_num)

            if (len(reply) < 1):
                continue

            return reply

    def WriteByte(self, _byte: int,
                  compliment: bool = True):
        data = [_byte]
        if (compliment):
            data.append(_byte ^ 0xFF)
        self.uart.write(bytes(data))

    def WriteByteWaitForACK(self, _bytes: int,
                            retry_num: int = 5, compliment: bool = True):
        while (retry_num > 0):
            retry_num -= 1

            data = [_bytes]
            if (compliment):
                data.append(_bytes ^ 0xFF)
            self.uart.write(bytes(data))

            reply = self.WaitForBytes(1)

            if (reply == -1):
                continue

            return reply

        return -1

    def WriteBytesWaitForACK(self, _bytes: bytes,
                             retry_num: int = 5):
        """ Assumes the data it receives is complete """

        while (retry_num > 0):
            retry_num -= 1

            self.uart.write(_bytes)

            reply = self.WaitForBytes(1)

            if (reply == -1):
                continue

            return reply

        return -1

    def WaitForBytes(self, num_bytes: int):
        """ Sits and waits for a response on the serial line.
            If no reply is received then return -1

            Note - Passing num_bytes <= 1 returns an int
                - Passing num_bytes >  1 returns an int array
        """
        rx_byte = self.uart.read(num_bytes)
        if (len(rx_byte) < 1):
            return -1

        if (num_bytes <= 1):
            return int.from_bytes(rx_byte, "big")

        # Convert to ints
        return [x for x in rx_byte]

    def WaitForBytesExcept(self, num_bytes: int):
        """ Sits and waits for a response on the serial line
            if no reply is received raise an exception.

            Note - Passing num_bytes <= 1 returns an int
                - Passing num_bytes >  1 returns an int array
        """
        rx_byte = self.uart.read(num_bytes)
        if (len(rx_byte) < 1):
            raise Exception("Error. No reply")

        if (num_bytes <= 1):
            return int.from_bytes(rx_byte, "big")

        # Convert to ints
        return [x for x in rx_byte]

    def SendUploadSelectionCommand(self, command: str):
        send_data = [ch for ch in bytes(command, "UTF-8")]
        res = self.WriteBytesWaitForACK(bytes(send_data), 5)
        if res == -1:
            raise Exception("Failed to move device into upload mode")

        if (command == "ui_upload"):
            #  Change to parity even
            print(f"Update uart to parity: {self.BB}EVEN{self.NW}")
            self.uart.parity = serial.PARITY_EVEN
            # Wait for a response
            res = self.WaitForBytes(1)
            if (res != self.READY):
                raise Exception("NO REPLY received after activating ui upload")

            print(f"Activating UI Upload Mode: {self.BG}SUCCESS{self.NW}")
            # Give time for the hactar device to be prepared
            # time.sleep(0.6)

    def SendSync(self, retry_num: int = 5):
        """ Sends the sync byte 0x7F and waits for an self.ACK from the device.
        """

        reply = self.WriteByteWaitForACK(self.Commands.sync,
                                         retry_num, False)

        # Check if ack or self.nack
        if (reply == self.NACK):
            print(f"Activating device: {self.BR}FAILED{self.NW}")
            raise Exception("Failed to Activate device")
        elif (reply == -1):
            print(f"Activating device: {self.BY}NO REPLY{self.NW}")
            raise Exception("Failed to Activate device")

        print(f"Activating device: {self.BG}SUCCESS{self.NW}")

    def SendGetID(self, retry_num: int = 5):
        """ Sends the GetID command and it's compliment.
            Command = 0x02
            Compliment = 0xFD = 0x02 ^ 0xFF

            returns pid
        """
        res = self.WriteByteWaitForACK(self.Commands.get_id, 1)

        if res == self.NACK:
            raise Exception("NACK was received during GetID")
        elif res == -1:
            raise Exception("No reply was received during GetID")

        # Read the next byte which should be the num of bytes - 1

        num_bytes = self.WaitForBytesExcept(1)

        # NOTE there is a bug in the bootloader for stm, or there is a
        # inaccuracy in the an3155 datasheet that says that a single self.ACK
        # is sent after receiving the get_id command.
        # But in reality 90% of the time two self.ACK bytes are sent
        if (num_bytes == self.ACK):
            num_bytes = self.WaitForBytesExcept(1)
        elif (num_bytes == self.NACK):
            raise Exception("NACK was received while trying to get the "
                            "num bytes in GetID")

        # Get the pid which should be N+1 bytes
        pid = self.WaitForBytesExcept(num_bytes+1)

        # Wait for an ack
        res = self.WaitForBytesExcept(1)
        if (res == self.NACK):
            raise Exception("A self.NACK was received at the end of GetID")
        elif res == -1:
            raise Exception("No reply was received during GetID")

        h_pid = hex(int.from_bytes(bytes(pid), "big"))
        print(f"Chip ID: {self.BC}{h_pid}{self.NW}")

        return pid

    def SendGet(self, retry_num: int = 5):
        """ Sends the Get command and it's compliment.
            Command = 0x00
            Compliment = 0xFF = 0x00 ^ 0xFF

            returns all available commands
        """
        reply = self.WriteByteWaitForACK(self.Commands.get, retry_num)
        if (reply == self.NACK):
            raise Exception("NACK was received during Get Commands")

        # Read the next byte which should be the num of bytes - 1
        #  for some reason they minus off one even tho 2 bytes come in
        num_bytes = self.WaitForBytesExcept(1)

        bootloader_verison = self.WaitForBytesExcept(1)

        # Get all of the available commands
        recv_commands = self.WaitForBytesExcept(num_bytes)

        # Wait for an ack
        reply = self.WaitForBytesExcept(1)

        # Parse the commands
        available_commands = []
        for key, val in self.Commands.__dict__.items():
            for cmd in recv_commands:
                if (val == cmd):
                    available_commands.append(key)

        if (reply == self.NACK):
            raise Exception("A self.NACK was received at the end of GetID")

        print(f"Bootloader version: {self.BM}{bootloader_verison}{self.NW}")

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
        reply = self.WriteByteWaitForACK(self.Commands.read_memory,
                                         retry_num)
        if (reply == self.NACK):
            raise Exception("NACK was received during Read Memory command")

        memory_addr = address + self.CalculateChecksum(address)

        # Write the memory we want to read from and the checksum
        reply = self.WriteBytesWaitForACK(memory_addr, 1)
        if (reply == self.NACK):
            raise Exception("NACK was received after sending memory address to"
                            " read")

        reply = self.WriteByteWaitForACK(num_bytes-1, 1)
        if (reply == self.NACK):
            raise Exception("NACK was received after sending how many bytes to"
                            " read")

        recv_data = self.WaitForBytesExcept(num_bytes)
        if type(recv_data) is int:
            recv_data = [recv_data]

        return recv_data

    def SendExtendedEraseMemory(self, sectors: [int],
                                special: bool, retry_num: int = 5):
        """ Sends the Erase memory command and it's compliment.
            Command = 0x44
            Compliment = 0xBB = 0x44 ^ 0xFF

            returns self.ACK/self.NACK/0
        """
        reply = self.WriteByteWaitForACK(self.Commands.extended_erase, 5)
        if (reply == self.NACK):
            raise Exception("NACK was received after sending erase command")
        elif (reply == self.NO_REPLY):
            raise Exception("No reply was received after sending erase"
                            " command")

        # TODO error check sectors?

        # Number of sectors starts at 0x00 0x00. So 0x00 0x00 means
        # delete 1 sector
        # An exception is thrown if the number is bigger than 2 bytes
        num_sectors = [(len(sectors) - 1).to_bytes(2, "big")]

        # Turn the sectors received into two byte elements
        byte_sectors = [x.to_bytes(2, "big") for x in sectors]

        # Join the num sectors and the byte sectors into one list
        data = b''.join(num_sectors + byte_sectors)

        # Calculate the checksum
        checksum = [self.CalculateChecksum(data)]

        # Join all the data together
        data = b''.join(num_sectors + byte_sectors + checksum)

        print(f"Erase: Sectors {self.NY}{sectors}{self.NW}")
        print(f"Erase: {self.BB}STARTED{self.NW}")

        # Give more time to reply with the deleted sectors
        self.uart.timeout = 10
        reply = self.WriteBytesWaitForACK(data, 1)
        # Revert the change back to two seconds
        self.uart.timeout = 2
        if (reply == -1):
            raise Exception("Failed to erase, no reply received")
        elif (reply == self.NACK):
            raise Exception("Failed to erase")
        print(f"Erase Verify: {self.BB}BEGIN{self.NW}")

        mem_bytes_sz = 256
        expected_mem = [255] * mem_bytes_sz
        mem = [0] * mem_bytes_sz
        read_count = 0
        bytes_verified = 0
        total_bytes_to_verify = 0
        for sector in sectors:
            total_bytes_to_verify += self.Sectors[sector].size
        percent_verified = int((bytes_verified / total_bytes_to_verify)*100)

        for sector in sectors:
            memory_address = self.Sectors[sector].addr
            end_of_sector = (self.Sectors[sector].addr +
                             self.Sectors[sector].size)
            while (memory_address != end_of_sector):
                read_count = 0
                mem = [0] * mem_bytes_sz
                percent_verified = int(
                    (bytes_verified / total_bytes_to_verify)*100)
                print(f"Verifying erase: {self.BG}{percent_verified:2}"
                      f"{self.NW}% verified", end="\r")
                while (mem != expected_mem) and read_count != 10:
                    mem = self.SendReadMemory(memory_address.to_bytes(
                                                4, "big"),
                                              mem_bytes_sz)
                    read_count += 1
                    if (mem != expected_mem):
                        print(f"Sector not verified {sector} retry"
                              f" {read_count}")

                if (read_count == 10 and mem != expected_mem):
                    print(f"Verifying: {self.BR}Failed to verify sector "
                          f"[{sector}]{self.NW}")
                    return False

                memory_address += mem_bytes_sz
                bytes_verified += mem_bytes_sz

        # Don't actually need to do the math here
        print(f"Verifying erase: {self.BG}100{self.NW}% verified")
        print(f"Erase: {self.BG}COMPLETE{self.NW}")
        return True

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

        print(f"Write to Memory: {self.BB}STARTED{self.NW}")
        print(f"Address: {self.BW}{address:04x}{self.NW}")
        print(f"Byte Stream Size: {self.BW}{total_bytes}{self.NW}")

        while file_addr < total_bytes:
            percent_flashed = int((file_addr / total_bytes) * 100)

            print(f"Flashing: {self.BG}{percent_flashed:2}{self.NW}%",
                  end="\r")
            reply = self.WriteByteWaitForACK(self.Commands.write_memory, 5)
            if (reply == self.NACK):
                raise Exception("NACK was received after sending write "
                                "command")
            elif (reply == -1):
                raise Exception("No reply was received after sending write "
                                "command")

            checksum = self.CalculateChecksum(addr.to_bytes(4, "big"))
            write_address_bytes = addr.to_bytes(4, "big") + checksum
            reply = self.WriteBytesWaitForACK(write_address_bytes, 1)
            if (reply == self.NACK):
                raise Exception("NACK was received after sending write "
                                "command")
            elif (reply == -1):
                raise Exception("No reply was received after sending write "
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
            chunk = chunk + self.CalculateChecksum(chunk)

            # Write the chunk to the chip
            reply = self.WriteBytesWaitForACK(chunk, 1)
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
        print(f"Flashing: {self.BG}100{self.NW}%")

        # Verify the written memory by reading the size of the file
        addr = address
        file_addr = 0

        while file_addr < total_bytes:
            percent_verified = int(
                (file_addr / total_bytes)*100)
            print(f"Verifying write: {self.BG}{percent_verified:2}{self.NW}"
                  f"% verified", end="\r")

            chunk = data[file_addr:file_addr+Max_Num_Bytes]
            mem = bytes(self.SendReadMemory(addr.to_bytes(4, "big"),
                        len(chunk)))

            if chunk != mem:
                raise Exception(f"Failed to verify at memory address {addr}")

            addr += len(mem)
            file_addr += len(chunk)

        print(f"Verifying write: {self.BG}100{self.NW}% verified")
        print(f"Write: {self.BG}COMPLETE{self.NW}")

        return self.ACK

    def ProgramSTM(self):

        # Send the command to the mgmt chip that we want to program the UI chip
        self.SendUploadSelectionCommand("ui_upload")

        self.SendSync()
        # Sometimes a small delay is required otherwise it won't
        # continue on correctly

        self.SendGetID(1)

        self.SendGet()

        mem = self.SendReadMemory(bytes([0x08, 0x00, 0x00, 0x00]), 256)
        print(f"Reading memory: {self.BG}SUCCESSFUL{self.NW}")
        print(f"Number of bytes read from memory: {self.BM}{len(mem)}\
            {self.NW}")

        time.sleep(1)

        # TODO get the size of our binary file and erase the
        # sectors that it would fit into
        sectors = [x for x in range(6)]
        self.SendExtendedEraseMemory(sectors, False)

        firmware = open("../ui/build/ui.bin", "rb").read()
        self.SendWriteMemory(firmware, self.User_Sector_Start_Address, 1)

