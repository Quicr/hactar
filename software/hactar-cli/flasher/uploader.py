import serial

import uart_utils


class Uploader:
    def __init__(self, uart: serial.Serial, chip: str):
        self.uart = uart
        self.chip = chip.lower()

    def FlashSelect(self):
        print("Select function is not defined")

    def FlashFirmware(self, binary_path: str) -> bool:
        print("Flash function is not defined")
        return False

    def TryPattern(self, pattern: int, recv_bytes_cnt: int, num_retry: int = 5):
        rx = 0
        for i in range(num_retry):
            rx = uart_utils.GetBytes(self.uart, recv_bytes_cnt)

            if rx == pattern:
                return

        raise Exception(f"Didn't receive pattern: {pattern} got {rx}")

    def TryHandshake(self, pattern: int, recv_bytes_cnt: int, num_retry: int = 5):
        rx = 0
        for i in range(num_retry):
            rx = uart_utils.GetBytes(self.uart, recv_bytes_cnt)

            if rx == pattern:
                uart_utils.WriteByte(self.uart, pattern, False)
                return

        raise Exception(f"Didn't receive handshake: {pattern} got {rx}")
