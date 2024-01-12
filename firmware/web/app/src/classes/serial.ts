import Sleep from "./sleep"
import {ACK, NACK, READY, NO_REPLY} from "./uart_utils"

import logger from "./logger";

class Serial
{

    in_buffer: number[];
    open: boolean = false;
    port: any = null;
    reader: any = null;
    reader_promise: any = null;
    released_reader: boolean = true;
    stop: boolean = false;
    parity: string = "none";

    constructor()
    {
        this.in_buffer = [];
    }

    // TODO clean up by keeping track of the parity for hactar
    async ConnectToDevice(filters: Object[]): Promise<boolean>
    {
        if (!('serial' in navigator))
            return false;

        // We already have a port, do nothing.
        if (this.port) return false;

        this.port = await (navigator as any).serial.requestPort({ filters });

        // No port was selected
        if (!this.port) return false;

        await this.OpenPort(this.parity);

        return true;
    }

    async ClosePort()
    {
        logger.Debug("[ClosePort(...)] Start");
        if (!this.open)
        {
            logger.Debug("[ClosePort(...)] Port not open, so return");
            return;
        }

        this.stop = true;
        this.reader.cancel();

        await this.reader_promise;
        await this.port.close();
        this.open = false;

        logger.Debug("[ClosePort(...)] Closed Port for Hactar");
    }

    async ClosePortAndNull()
    {
        await this.ClosePort();
        this.port = null;
    }

    async OpenPort(parity: string)
    {
        if (this.open && parity == this.parity)
        {
            // Do nothing.
            return;
        }

        this.parity = parity;

        logger.Debug(`[OpenPort(...)] Start with parity: ${this.parity}`);
        const options = {
            baudRate: 115200,
            dataBits: 8,
            stopBits: 1,
            parity: this.parity
        };

        await this.ClosePort();

        this.stop = false;
        this.released_reader = true;
        this.in_buffer = [];

        await this.port.open(options);

        this.open = true;

        // Start listening
        this.reader_promise = this.ListenForBytes()
    }

    async ListenForBytes()
    {
        logger.Debug("[ListenForBytes(...)] Start");
        logger.Debug(`[ListenForBytes(...)] Port readable: ${this.port.readable}, Stop: ${this.stop}`)
        this.released_reader = false;

        while (this.port.readable && !this.stop)
        {
            this.reader = this.port.readable.getReader();
            try
            {
                while (!this.stop)
                {
                    const { value, done } = await this.reader.read()

                    if (done)
                    {
                        break;
                    }

                    value.forEach((element: any) =>
                    {
                        this.in_buffer.push(element);
                    });
                }
            }
            catch (error)
            {
                logger.Debug(error);
                return;
            }
            finally
            {
                this.reader.releaseLock();
                this.released_reader = true;
            }
        }

        logger.Debug("[ListenForBytes(...)] Stop");
    }

    async ReadByte(timeout: number = 2000): Promise<number>
    {
        let res = await this.ReadBytes(1, timeout)

        return res[0];
    }

    async ReadBytes(num_bytes: number, timeout: number = 2000): Promise<number[]>
    {
        let num = num_bytes
        let bytes: number[] = []

        var run = true;
        const clock:any = setTimeout(() => {
            run = false;
        }, timeout);

        while (num > 0 && run)
        {
            if (this.in_buffer.length == 0)
            {
                await Sleep(0.001)
                continue;
            }
            bytes.push(this.in_buffer[0]);
            this.in_buffer.shift();
            num--;
        }

        clearTimeout(clock);

        if (bytes.length < 1)
            return [NO_REPLY];


        return bytes;
    }

    async ReadBytesExcept(num_bytes: number, timeout: number = 2000): Promise<number[]>
    {
        let result: number[] = await this.ReadBytes(num_bytes, timeout);
        if (result[0] == NO_REPLY)
        {
            logger.Error("Error. Didn't receive any bytes");
            throw "Error. Didn't receive any bytes";
        }

        return result;
    }

    async WriteBytes(bytes: Uint8Array)
    {
        const writer = this.port.writable.getWriter();
        await writer.write(bytes);
        writer.releaseLock();
    }
}

export default Serial;