import serial

import uart_utils


class Uploader:
    def __init__(self, chip: str):
        self.chip = chip.lower()

    def FlashSelect(self, uart: serial.Serial):
        print("Select function is not defined")

    def FlashFirmware(self, uart: serial.Serial) -> bool:
        print("Flash function is not defined")
        return False

    def TryPattern(self, uart: serial.Serial, pattern: int, recv_bytes_cnt: int, num_retry: int = 5):
        rx = 0
        for i in range(num_retry):
            rx = uart_utils.GetBytes(uart, recv_bytes_cnt)

            if rx == pattern:
                return

        raise Exception(f"Didn't receive pattern: {pattern} got {rx}")

    def TryHandshake(self, uart: serial.Serial, pattern: int, recv_bytes_cnt: int, num_retry: int = 5):
        rx = 0
        for i in range(num_retry):
            rx = uart_utils.GetBytes(uart, recv_bytes_cnt)

            if rx == pattern:
                uart_utils.WriteByte(uart, pattern, False)
                return

        raise Exception(f"Didn't receive handshake: {pattern} got {rx}")
