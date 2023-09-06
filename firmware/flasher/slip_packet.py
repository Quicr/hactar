class slip_packet:
    END = 0xC0
    ESC = 0xDB
    ESC_END = 0xDC
    ESC_ESC = 0xDD

    data = []

    def __init__(self, data_in: [] = []):
        self.PushArray(data_in)

    def Push(self, ele):
        self.data.push(ele)

    def PushArray(self, data_in: []):
        self.data += data_in

    def SLIPEncode(self):
        # First check if there is END
        encoded_data = self.data
        data_len = len(encoded_data)

        idx = 0
        while (idx < data_len):
            if self.END == encoded_data[idx]:
                encoded_data[idx] = self.ESC
                encoded_data.insert(idx+1, self.ESC_END)
                idx += 1
            elif self.ESC == encoded_data[idx]:
                encoded_data.insert(idx+1, self.ESC_ESC)
                idx += 1
            idx += 1

        return encoded_data

    def Data():
        return

    def ToEncodedBytes(self):
        return bytes(self.SLIPEncode())
