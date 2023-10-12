import axios from "axios"
import { ACK, READY, NACK, NO_REPLY } from "./uart_utils"
import Serial from "@/classes/serial"

import logger from "./logger";

// TODO typescript types
import stm32_flasher from "./stm32_flasher";
import esp32_flasher from "./esp32_flasher";
import Sleep from "./sleep";

class HactarFlasher
{
    logs: any;
    serial: Serial;
    stm_flasher: any;
    esp_flasher: any;

    constructor()
    {
        this.logs = [];
        this.serial = new Serial();
    }

    async ConnectToHactar(filters: Object[])
    {
        return await this.serial.ConnectToDevice(filters);
    }

    async GetBinary(url: string)
    {
        // Get a binary from the server
        let res = await axios.get(`http://localhost:7775/${url}`);
        return res.data;
    }

    async Flash(mode: string = "ui+net")
    {
        try
        {
            let sleep_before_net_upload = false;
            if (mode.includes("ui"))
            {
                const binary = await this.GetBinary("get_ui_bins");
                await this.SendUploadSelectionCommand("ui_upload");
                await stm32_flasher.FlashSTM(this.serial, binary);
                await this.serial.ClosePort();
                sleep_before_net_upload = true;
            }

            if (mode.includes("net"))
            {
                if (sleep_before_net_upload)
                {
                    await Sleep(2000);
                }

                const binaries = await this.GetBinary("get_net_bins");
                await this.SendUploadSelectionCommand("net_upload");
                await esp32_flasher.FlashESP(this.serial, binaries);
            }

            await this.serial.ClosePortAndNull();
        }
        catch (exception)
        {
            await this.serial.ClosePortAndNull();

            console.error(exception);
        }
    }

    async SendUploadSelectionCommand(command: string)
    {
        // Ensure that the port is opened with the right parity for send
        // commands to the mgmt
        await this.serial.OpenPort("none");

        if (command != 'ui_upload' && command != 'net_upload')
        {
            logger.Error(`Error. ${command} is an invalid command`);
            throw `Error. ${command} is an invalid command`;
        }

        let enc = new TextEncoder()

        // Get the response
        let reply = await this.serial.WriteBytesWaitForACK(enc.encode(command), 4000, 5);
        if (reply == NO_REPLY)
        {
            logger.Error("Failed to move Hactar into upload mode");
            throw "Failed to move Hactar into upload mode";
        }

        if (command == "ui_upload")
        {
            await this.serial.OpenPort("even");

            for (let i = 0; i < 5; i++)
            {
                reply = await this.serial.ReadByte(5000);

                if (reply == READY)
                {
                    break;
                }

                logger.Warning("Failed to get Hactar into ui upload mode, retrying...");
            }

            if (reply == NO_REPLY)
            {
                logger.Error("Hactar took too long to get ready");
                throw "Hactar took too long to get ready";
            }


            logger.Info("Activating UI Upload Mode: SUCCESS");
            logger.Info("Update uart to parity: EVEN");
        }
        else if (command == "net_upload")
        {
            // await this.serial.OpenPort("none");

            for (let i = 0; i < 5; i++)
            {
                reply = await this.serial.ReadByte(5000);

                if (reply == READY)
                {
                    break;
                }
                logger.Warning("Failed to get Hactar into net upload mode, retrying...");
            }

            if (reply == NO_REPLY)
            {
                logger.Error("Hactar took too long to get ready");
                throw "Hactar took too long to get ready";
            }

            logger.Info("Activating NET Upload Mode: SUCCESS");
            logger.Info("Update uart to parity: NONE");
        }
    }

};

export default HactarFlasher;