import Sleep from "./sleep"
import { ACK, READY, NACK, NO_REPLY } from "./uart_utils"
import Serial from "./serial"

class ESP32Flasher
{
    async FlashESP(serial: Serial, ui_bin: number[])
    {

    }

    async Sync(serial: Serial, retry: number = 5)
    {

    }

    // async ReadMemory(serial: Serial, address:)
};

const esp32_flasher = new ESP32Flasher();

export default esp32_flasher;