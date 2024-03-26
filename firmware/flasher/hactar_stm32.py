from stm32 import stm32_flasher
import uart_utils

class HactarStm32:
    def __init__(self, stm32_flasher):
        self.stm32_flasher = stm32_flasher

    def ProgramHactarSTM(self, chip, binary_path, recover:bool):
        # Determine which chip will be flashed
        uart_utils.FlashSelection(self.stm32_flasher.uart, chip)

        # Get the firmware
        firmware = open(binary_path, "rb").read()

        # Get the size of memory that we need to erase
        sectors = self.GetSectorsForFirmware(len(firmware))


        self.SendExtendedEraseMemory(sectors, False)


        finished_writing = False
        tried_to_recover = False
        while (finished_writing):
            try:
                finished_writing = self.SendWriteMemory(firmware, self.chip["usr_start_addr"], tried_to_recover, 5)
            except Exception as ex:
                if (not recover or tried_to_recover):
                    raise ex

                tried_to_recover = True

        if (chip == "mgmt"):
            self.SendGo(self.chip["usr_start_addr"])

        return True