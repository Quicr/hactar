import axios from "axios"
import { ACK, READY, NACK, NO_REPLY } from "./uart_utils"
import Serial from "@/classes/serial"
import STM32Flasher from "./stm32_flasher";
// TODO export types
import esp32_flasher from "./esp32_flasher";

class HactarFlasher
{
    logs: any;
    serial: Serial;
    stm_flasher: STM32Flasher;
    esp_flasher: any;

    constructor()
    {
        this.logs = [];
        this.serial = new Serial();
        this.stm_flasher = new STM32Flasher();
        this.esp_flasher = esp32_flasher;
    }

    async ConnectToHactar(filters: Object[])
    {
        return await this.serial.ConnectToDevice(filters);
    }

    async ClosePortAndNull()
    {
        await this.serial.ClosePortAndNull();
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
            if (mode.includes("ui"))
            {
                const binary = await this.GetBinary("get_ui_bins");
                await this.SendUploadSelectionCommand("ui_upload");
                await this.stm_flasher.FlashSTM(this.serial, binary);
            }

            if (mode.includes("net"))
            {
                const binaries = await this.GetBinary("get_net_bins");
                await this.SendUploadSelectionCommand("net_upload");
                await this.esp_flasher.FlashESP(this.serial, binaries);
            }

            await this.ClosePortAndNull();
        }
        catch (exception)
        {
            await this.ClosePortAndNull();

            console.error(exception);
        }
    }

    async SendUploadSelectionCommand(command: string)
    {
        // Ensure that the port is opened with the right parity for send
        // commands to the mgmt
        await this.serial.OpenPort("none");

        if (command != 'ui_upload' && command != 'net_upload')
            throw `Error. ${command} is an invalid command`;

        let enc = new TextEncoder()

        // Get the response
        let reply = await this.serial.WriteBytesWaitForACK(enc.encode(command), 4000);
        if (reply == NO_REPLY)
        {
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
            }

            if (reply == NO_REPLY)
                throw "Hactar took too long to get ready";


            this.Log("Activating UI Upload Mode: SUCCESS");
            this.Log("Update uart to parity: EVEN");
        }
        else if (command == "net_upload")
        {
            await this.serial.OpenPort("none");

            for (let i = 0; i < 5; i++)
            {
                reply = await this.serial.ReadByte(5000);

                if (reply == READY)
                {
                    break;
                }
            }

            if (reply == NO_REPLY)
                throw "Hactar took too long to get ready";

            this.Log("Activating NET Upload Mode: SUCCESS");
            this.Log("Update uart to parity: NONE");
        }
    }

    Log(text: any, replace_previous: boolean = false)
    {
        console.log(`replace: ${replace_previous}: ${text}`);
        this.logs.push({ text, replace_previous });
    }

};

export default HactarFlasher;