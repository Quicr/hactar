import axios from 'https://cdn.skypack.dev/axios'
import Sleep from "./sleep.js";
// import { ACK, READY, NACK, NO_REPLY } from "./uart_utils.js";
// import Serial from "./classes/serial.js";
// import logger from "./logger.js";
// import stm32_flasher from "./stm32_flasher.js";
// import stm32_serial from "./stm32_serial.js";
// import esp32_flasher from "./esp32_flasher.js";

class HactarFlasher {
    constructor() {
        this.logs = [];
        // this.serial = new Serial();
        this.stm_flasher = null;
        this.esp_flasher = null;
    }

    async GetBinary(firmware) {
        const res = await fetch(`https://quicr.github.io/hactar/build/ui/ui.bin`);
        console.log(res)
        return res.data;
    }

    async Flash(mode = "ui+net") {
        try {
            let sleep_before_net_upload = false;

            if (mode.includes("mgmt")) {
                const binary = await this.GetBinary("mgmt");
                await this.SendUploadSelectionCommand("mgmt_upload");
                // await stm32_flasher.FlashSTM(this.serial, binary, true);
                // await this.serial.ClosePortAndNull();
                return;
            }

            if (mode.includes("ui")) {
                const binary = await this.GetBinary("ui");
                await this.SendUploadSelectionCommand("ui_upload");
                // await stm32_flasher.FlashSTM(this.serial, binary);
                // await this.serial.ClosePort();
                sleep_before_net_upload = true;
            }

            if (mode.includes("net")) {
                if (sleep_before_net_upload) {
                    await Sleep(2000);
                }

                const binaries = await this.GetBinary("net");
                if (binaries.length === 0) {
                    console.log("No net binaries received");
                    return;
                }

                await this.SendUploadSelectionCommand("net_upload");
                // await esp32_flasher.FlashESP(this.serial, binaries);
            }

            // await this.serial.ClosePortAndNull();
        } catch (exception) {
            // await this.serial.ClosePortAndNull();
            console.error(exception);
        }
    }

    async SendUploadSelectionCommand(command) {
        // await this.serial.OpenPort("none");

        if (!["mgmt_upload", "ui_upload", "net_upload"].includes(command)) {
            logger.Error(`Error. ${command} is an invalid command`);
            throw `Error. ${command} is an invalid command`;
        }

        if (command === "mgmt_upload") {
            // await this.serial.OpenPort("even");
            this.PulseSignals(false, false);
            logger.Info("Activating MGMT Upload Mode: SUCCESS");
            logger.Debug("Update uart to parity: EVEN");
            return;
        }

        const enc = new TextEncoder();
        // let reply = await stm32_serial.WriteBytesWaitForACK(this.serial, enc.encode(command), 4000, 5);

        if (reply === NO_REPLY) {
            logger.Error("Failed to move Hactar into upload mode");
            throw "Failed to move Hactar into upload mode";
        }

        if (command === "ui_upload") {
            // await this.serial.OpenPort("even");
            for (let i = 0; i < 5; i++) {
                reply = await this.serial.ReadByte(5000);
                if (reply === READY) break;
                logger.Warning("Failed to get Hactar into ui upload mode, retrying...");
            }
            if (reply === NO_REPLY) {
                logger.Error("Hactar took too long to get ready");
                throw "Hactar took too long to get ready";
            }

            logger.Info("Activating UI Upload Mode: SUCCESS");
            logger.Info("Update uart to parity: EVEN");
        } else if (command === "net_upload") {
            for (let i = 0; i < 5; i++) {
                reply = await this.serial.ReadByte(5000);
                if (reply === READY) break;
                logger.Warning("Failed to get Hactar into net upload mode, retrying...");
            }

            if (reply === NO_REPLY) {
                logger.Error("Hactar took too long to get ready");
                throw "Hactar took too long to get ready";
            }

            logger.Info("Activating NET Upload Mode: SUCCESS");
            logger.Info("Update uart to parity: NONE");
        }
    }

    async PulseSignals(rts = false, dtr = false, toggle_speed_ms = 1) {
        // await this.serial.port.setSignals({ requestToSend: rts });
        // await this.serial.port.setSignals({ dataTerminalReady: dtr });
        // await new Promise(resolve => setTimeout(resolve, toggle_speed_ms));
        // await this.serial.port.setSignals({ requestToSend: !rts });
        // await this.serial.port.setSignals({ dataTerminalReady: !dtr });
        // await new Promise(resolve => setTimeout(resolve, toggle_speed_ms));
        // await this.serial.port.setSignals({ requestToSend: rts });
        // await this.serial.port.setSignals({ dataTerminalReady: dtr });
    }
}

export default HactarFlasher;
export const flasher = new HactarFlasher();