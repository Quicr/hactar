import logger from "./logger";
import Serial from "./serial";
import Sleep from "./sleep";

class Monitor
{
    serial: Serial;
    running: boolean;
    read_interval: NodeJS.Timer | null;

    constructor()
    {
        this.serial = new Serial();
        this.running = true;
        this.read_interval = null;
    }

    async ConnectToHactar(filters: Object[])
    {
        return await this.serial.ConnectToDevice(filters);
    }

    async Begin()
    {
        if (this.read_interval == null)
        {
            this.Read();
        }
    }

    async Stop()
    {
        if (this.read_interval != null)
        {
            clearInterval(this.read_interval);
        }

        this.serial.ClosePortAndNull();
    }

    async Read()
    {
        while (this.running)
        {
            if (!this.serial.HasBytes())
            {
                await Sleep(0.1);
                continue;
            }

            let line = await this.serial.ReadLine();

            if (line != "")
            {
                logger.Info(line);
            }
        }
    }

    async Write(data: Uint8Array)
    {
        if (!this.serial.open)
        {
            logger.Info("Monitor is not connected");
            return;
        }
        this.serial.WriteBytes(data);
    }
}




// const monitor = new Monitor();
export default Monitor;