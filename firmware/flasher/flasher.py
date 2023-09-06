import sys
import stm32
import serial
import slip_packet


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

        # print(f"{B_WHITE}Starting{N_WHITE}")
        stm32_flasher = stm32.stm32_flasher(uart)
        stm32_flasher.ProgramSTM()

        uart.close()
    except Exception as ex:
        print(f"{ex}")
        # print(f"{B_RED}{ex}{N_WHITE}")


main()
