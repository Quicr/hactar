import functools


def FromBytes(byte_arr: [] = []):
    packet = esp32_slip_packet()

    return packet


class esp32_slip_packet:
    END = 0xC0
    ESC = 0xDB
    ESC_END = 0xDC
    ESC_ESC = 0xDD

    # Header is at least 8 bytes + 1 for end byte at start
    data = [0] * 9
    data_length = 0

    def __init__(self, request_response: int = 0, command: int = 0,
                 size: int = 0):
        self.data[0] = self.END
        self.SetHeader(request_response, command, size)
        pass

    def __getitem__(self, key):
        # -2 for the end byte
        if key < 0 and key > len(self.data) - 2:
            return -1

        return self.data[key-1]

    def SetHeader(self, request_response: int, command: int, size: int = 0):
        # TODO error checking
        self.data[1] = int(request_response).to_bytes(1, "little")
        self.data[2] = command.to_bytes(1, "little")
        self.SetSize(size)

    def SetSize(self, size: int):
        data = size.to_bytes(2, "little")
        self.data[3] = data[0]
        self.data[4] = data[1]

    def PushData(self, ele: int):
        # Convert to bytes and then append all of the bytes
        ele_bytes = ele.to_bytes((max(ele.bit_length(), 1) + 7) // 8, "little")
        self.data_length += 1
        self.data.push(ele_bytes)

    def PushDataArray(self, data_in: []):
        for ele in data_in:
            self.PushData(ele)

    def Length(self):
        return self.data_length

    def SetChecksum(self, arr: [int]):
        checksum = functools.reduce(lambda a, b: a ^ b, arr)
        checksum ^ self.CHECKSUM_SEED
        checksum_bytes = checksum.to_bytes(4, "little")

        self.data[5] = checksum_bytes[0]
        self.data[6] = checksum_bytes[1]
        self.data[7] = checksum_bytes[2]
        self.data[8] = checksum_bytes[3]

    def SLIPEncode(self, checksum: bool = False):
        # First check if there is END

        # Set the checksum
        # Sometimes it is ignored, but we will just set it anyways
        self.SetChecksum()

        # Set the current size
        self.SetSize(self.data_length)

        # TODO might change?
        encoded_data = self.data
        data_len = len(encoded_data)

        # Skip the first byte
        idx = 1
        while (idx < data_len):
            if self.END == encoded_data[idx]:
                encoded_data[idx] = self.ESC
                encoded_data.insert(idx+1, self.ESC_END)
                idx += 1
            elif self.ESC == encoded_data[idx]:
                encoded_data.insert(idx+1, self.ESC_ESC)
                idx += 1
            idx += 1

        # Put on the final end byte
        encoded_data.push(self.END)

        return bytes(encoded_data)

    def Data():
        return

    def ToEncodedBytes(self):
        return bytes(self.SLIPEncode())
