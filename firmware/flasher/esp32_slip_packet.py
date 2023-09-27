import functools
from copy import deepcopy

# TODO decide on if I want to represent the data in big endian and then
# encode it into little endian, or just have it little endian from the start???
#


class esp32_slip_packet:
    """Slip packet for esp32 bootloader.
        |-------------------|
        |Byte    name       |
        |-------------------|
        |0       Direction  |
        |1       Command    |
        |2-3     Size       |
        |4-7     Checksum   |
        |8..n    Data       |
        |-------------------|
        - NOTE: Data is stored in little endian format for multi-byte fields
    """
    END = 0xC0
    ESC = 0xDB
    ESC_END = 0xDC
    ESC_ESC = 0xDD

    # Header is at least 8 bytes + 1 for end byte at start

    def __init__(self, direction: int = 0, command: int = 0,
                 size: int = 0):
        self.data = [0] * 8
        self.data_length = 0
        self.SetHeader(direction, command, size)

    def __getitem__(self, key):
        # -2 for the end byte
        if key < 0 and key > len(self.data) - 2:
            return -1

        return self.data[key-1]

    def FromBytes(self, data: bytes):
        """
        Takes a little endian byte stream and inputs it into the data field
        """

        array = []
        for d in data:
            array.append(int.from_bytes(d, "little"))

        if (array[0] != self.END and array[-1] != self.END):
            raise Exception("Error. Start and end bytes missing")

        idx = 1
        # Remove slip encapsulation
        cleaned_data = []
        while idx < len(array):
            if array[idx] == self.ESC:
                if array[idx+1] == self.ESC_END:
                    cleaned_data.append(self.END)
                elif array[idx+1] == self.ESC_ESC:
                    # following byte is not an escape
                    cleaned_data.append(self.ESC)
                else:
                    raise Exception(f"{array[idx]} not properly escaped" +
                                    f" at idx {idx}")
                idx += 1
            elif array[idx] == self.END:
                break
            else:
                cleaned_data.append(array[idx])

            idx += 1

        if (len(cleaned_data) < 8):
            raise Exception("Error. Data needs to be at least 8 bytes")

        for i in range(8):
            self.data[i] = cleaned_data[i]

        length = self.GetSize()
        self.PushDataArray(cleaned_data[8:8+length], "little")

    def SetHeader(self, direction: int, command: int, size: int = 0):
        self.SetDirection(direction)
        self.SetCommand(command)
        self.SetSize(size)

    def SetDirection(self, direction: int):
        if (direction != 0 and direction != 1):
            raise Exception("Direction must be either 0 or 1")
        self.data[0] = direction

    def SetCommand(self, command: int):
        if (command.bit_length() > 8):
            raise Exception("Command should not be larger than 1 byte")
        self.data[1] = command

    def SetSize(self, size: int):
        if (size.bit_length() > 16):
            raise Exception("Size should not be larger than 2 bytes")
        data = size.to_bytes(2, "little")
        self.data[2] = data[0]
        self.data[3] = data[1]

    def Get(self, start_idx: int, num_bytes: int):
        data = self.data[start_idx:start_idx+num_bytes]

        return int.from_bytes(data, "little")

    def GetDirection(self):
        return self.data[0]

    def GetCommand(self):
        return self.data[1]

    def GetDataField(self):
        return self.data[8:8+self.data_length]

    def GetSize(self):
        return int.from_bytes(bytes(self.data[2:3]), "little")

    def PushData(self, ele: int, size: int = -1):
        """ Pushes an element into the data, if the element is > 1 byte it
            will be stored in little endian format.
        """
        num_bytes = size
        if (num_bytes == -1):
            num_bytes = (max(ele.bit_length(), 1) + 7) // 8
        ele_bytes = ele.to_bytes(num_bytes, "little")
        if (ele == 0x4000):
            print(ele_bytes)

        for ele in ele_bytes:
            self.data.append(ele)

        self.data_length += len(ele_bytes)

    def PushDataArray(self, data_in: [], endian: str = "little",
                      size: int = -1):
        """ Pushes an array of data into the data array
            - Expects data in big endian format
        """
        if (endian == "little"):
            for ele in reversed(data_in):
                self.PushData(ele, size)
        else:
            for ele in data_in:
                self.PushData(ele, size)

    def Length(self):
        return self.data_length

    def SetChecksum(self):
        # Checksum seed
        checksum = 0xEF
        idx = 16
        # print(f"checksum: {hex(checksum)}, idx: {idx}")
        while (idx < len(self.data)):
            checksum ^= self.data[idx]
            # print(f"checksum: {hex(checksum)}, v: {hex(self.data[idx])}, idx: {idx}")
            idx += 1

        # print(f"checksum: {hex(checksum)}, idx: {idx}")

        # checksum = functools.reduce(lambda a, b: a ^ b, self.data)
        # checksum ^ self.CHECKSUM_SEED
        checksum_bytes = checksum.to_bytes(4, "little")

        self.data[4] = checksum_bytes[0]
        self.data[5] = checksum_bytes[1]
        self.data[6] = checksum_bytes[2]
        self.data[7] = checksum_bytes[3]

    def SLIPEncode(self, checksum: bool = False):
        # Set the checksum
        # Sometimes it is ignored, but we will just set it anyways
        # TODO only use checksum with certain types
        if checksum:
            self.SetChecksum()

        # Set the current size
        self.SetSize(self.data_length)

        # TODO might change?
        # encoded_data = deepcopy(self.data)
        encoded_data = []
        encoded_data.append(self.END)

        # Skip the first byte
        idx = 0
        while (idx < len(self.data)):
            if self.END == self.data[idx]:
                encoded_data.append(self.ESC)
                encoded_data.append(self.ESC_END)
            elif self.ESC == self.data[idx]:
                encoded_data.append(self.ESC)
                encoded_data.append(self.ESC_ESC)
            else:
                encoded_data.append(self.data[idx])
            idx += 1

        # Put on the final end byte
        encoded_data.append(self.END)

        return bytes(encoded_data)

    def __str__(self):
        # Print the encoded version
        to_print = self.SLIPEncode(True)

        s_out = ""
        idx = 1
        for b in to_print:
            s_out += "%0.2X" % b
            if (idx % 8 == 0):
                s_out += " "
            if (idx % 16 == 0):
                s_out += "\n"
            idx += 1

        return s_out

    def Data():
        return

    def ToEncodedBytes(self):
        return bytes(self.SLIPEncode())
