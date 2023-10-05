import Sleep from "./sleep"
import {ACK, NACK, READY, NO_REPLY} from "./uart_utils"

class Serial
{

    in_buffer: number[];
    open: boolean = false;
    port: any = null;
    reader: any = null;
    reader_promise: any = null;
    released_reader: boolean = true;
    stop: boolean = false;

    constructor()
    {
        this.in_buffer = [];
    }

    async ConnectToDevice(filters: Object[]): Promise<boolean>
    {
        if (!('serial' in navigator))
            return false;

        // We already have a port, do nothing.
        if (this.port) return false;

        this.port = await (navigator as any).serial.requestPort({ filters });

        // No port was selected
        if (!this.port) return false;

        await this.OpenPort("none");

        return true;
    }

    async ClosePort()
    {
        console.log("close port");
        if (!this.open)
        {
            console.log("Port not open");
            return;
        }

        this.stop = true;
        this.reader.cancel();

        console.log(this.reader_promise);
        await this.reader_promise;
        await this.port.close();
        this.open = false;

        console.log("Close hactar");
    }

    async ClosePortAndNull()
    {
        await this.ClosePort();
        this.port = null;
    }

    async OpenPort(parity: string)
    {
        console.log("Open port");
        const options = {
            baudRate: 115200,
            dataBits: 8,
            stopBits: 1,
            parity: parity
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
        this.released_reader = false;
        console.log(`Port readable: ${this.port.readable}, Stop: ${this.stop}`)

        while (this.port.readable && !this.stop)
        {
            console.log("Listening to bytes on serial port");
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
                console.log(error);
                return;
            }
            finally
            {
                this.reader.releaseLock();
                this.released_reader = true;
            }
        }
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

        // let start_time = Date.now();
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
            throw "Error, didn't receive any bytes";
        }

        return result;
    }

    async WriteBytes(bytes: Uint8Array)
    {
        const writer = this.port.writable.getWriter();
        await writer.write(bytes);
        writer.releaseLock();
    }

    async WriteByteWaitForACK(byte: number, retry: number = 1,
        compliment: boolean = true, timeout = 2000)
    {

        let data: number[] = [byte];
        if (compliment)
        {
            data.push(byte ^ 0xFF)
        }

        let num = retry;
        while (num--)
        {
            await this.WriteBytes(new Uint8Array(data));

            let reply = await this.ReadByte(timeout);

            if (reply == NO_REPLY)
                continue;

            return reply;
        }

        return NO_REPLY;
    }

    async WriteBytesWaitForACK(bytes: Uint8Array, timeout: number = 2000,
        retry: number = 1)
    {
        let num = retry;
        while (num--)
        {
            await this.WriteBytes(bytes);

            let reply = await this.ReadByte(timeout);

            if (reply == NO_REPLY)
                continue;

            return reply;
        }

        return NO_REPLY;
    }
}

export default Serial;