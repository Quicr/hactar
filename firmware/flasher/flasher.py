import sys
import serial
import uart_utils
import stm32
import esp32
import esp32_slip_packet

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


def main():
    try:
        if (len(sys.argv) < 3):
            print("Error. Need port followed by baudrate")
            exit()

        port = sys.argv[1]
        baud = sys.argv[2]

        uart = serial.Serial(
            port=port,
            baudrate=baud,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=2
        )

        print(f"Opened port: {port} baudrate: {baud}")

        print(f"{BW}Starting{NW}")
        stm32_flasher = stm32.stm32_flasher(uart)
        stm32_flasher.ProgramSTM()

        # esp32_flasher = esp32.esp32_flasher(uart)
        # esp32_flasher.ProgramESP()

        # data = bytes([0xc0, 0xdb, 0xdc, 0xdb, 0xdc, 0xdb, 0xdd, 0xdd, 0x00, 0x00, 0x00, 0x00, 0xc0])
        # packet = esp32_slip_packet.esp32_slip_packet()
        # packet.FromBytes(data)

        # print(packet.SLIPEncode())

        uart.close()
    except Exception as ex:
        print(f"{ex}")
        # print(f"{B_RED}{ex}{N_WHITE}")


main()
