import functools
from copy import deepcopy

# TODO decide on if I want to represent the data in big endian and then
# encode it into big endian, or just have it little endian from the start???
#

class esp32_slip_packet:
    END = 0xC0
    ESC = 0xDB
    ESC_END = 0xDC
    ESC_ESC = 0xDD

    # Header is at least 8 bytes + 1 for end byte at start
    data = [0] * 8
    data_length = 0

    def __init__(self, request_response: int = 0, command: int = 0,
                 size: int = 0):
        # self.data[0] = self.END
        self.SetHeader(request_response, command, size)

    def __getitem__(self, key):
        # -2 for the end byte
        if key < 0 and key > len(self.data) - 2:
            return -1

        return self.data[key-1]

    def FromBytes(self, data: bytes):
        if (data[0] != self.END and data[-1] != self.END):
            raise Exception("Error. Start and end bytes missing")
        idx = 1
        # Remove slip encapsulation
        cleaned_data = []
        while idx < len(data):
            if data[idx] == self.ESC:
                if data[idx+1] == self.ESC_END:
                    cleaned_data.append(self.END)
                elif data[idx+1] == self.ESC_ESC:
                    # following byte is not an escape
                    cleaned_data.append(self.ESC)
                else:
                    raise Exception(f"{data[idx]} not properly escaped" +
                                    f" at idx {idx}")
                idx += 1
            elif data[idx] == self.END:
                print("end found")
                break
            else:
                cleaned_data.append(data[idx])

            idx += 1

        if (len(cleaned_data) < 8):
            raise Exception("Error. Data needs to be at least 8 bytes")

        print(cleaned_data)

        for i in range(8):
            print(i)
            self.data[i] = cleaned_data[i]

        self.PushData(cleaned_data[8:8])

    def SetHeader(self, request_response: int, command: int, size: int = 0):
        # TODO error checking
        self.data[0] = int(request_response).to_bytes(1, "little")[0]
        self.data[1] = command.to_bytes(1, "little")[0]
        self.SetSize(size)

    def SetSize(self, size: int):
        data = size.to_bytes(2, "little")
        self.data[2] = data[0]
        self.data[3] = data[1]

    def GetSize(self):
        []

    def PushData(self, ele: int):
        # Convert to bytes and then append all of the bytes
        ele_bytes = ele.to_bytes((max(ele.bit_length(), 1) + 7) // 8, "little")

        for ele in ele_bytes:
            self.data.append(ele)

        self.data_length += len(ele_bytes)

    def PushDataArray(self, data_in: []):
        for ele in data_in:
            self.PushData(ele)

    def Length(self):
        return self.data_length

    def SetChecksum(self):
        # Checksum seed
        checksum = 0xEF
        idx = 8
        while (idx < self.data_length):

            checksum ^= self.data[idx]
            idx += 1

        # checksum = functools.reduce(lambda a, b: a ^ b, self.data)
        # checksum ^ self.CHECKSUM_SEED
        checksum_bytes = checksum.to_bytes(4, "little")

        self.data[4] = checksum_bytes[0]
        self.data[5] = checksum_bytes[1]
        self.data[6] = checksum_bytes[2]
        self.data[7] = checksum_bytes[3]

    def SLIPEncode(self, checksum: bool = False):
        # First check if there is END

        # Set the checksum
        # Sometimes it is ignored, but we will just set it anyways
        self.SetChecksum()

        # Set the current size
        self.SetSize(self.data_length)

        # TODO might change?
        # encoded_data = deepcopy(self.data)
        encoded_data = []
        encoded_data.append(self.END)

        # Skip the first byte
        idx = 1
        while (idx < len(self.data)):
            if self.END == encoded_data[idx]:
                encoded_data[idx] = self.ESC
                encoded_data.insert(idx+1, self.ESC_END)
                idx += 1
            elif self.ESC == encoded_data[idx]:
                encoded_data.insert(idx+1, self.ESC_ESC)
                idx += 1
            encoded_data.append(self.data[idx])
            idx += 1

        # Put on the final end byte
        encoded_data.append(self.END)

        return bytes(encoded_data)

    def Data():
        return

    def ToEncodedBytes(self):
        return bytes(self.SLIPEncode())
