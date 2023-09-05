class HactarFlasher
{
    constructor()
    {

    }

    async FlashUI(ui_bin: number[])
    {
        try
        {
            let sectors_to_erase = this.SectorsToErase(ui_bin.length);

            this.progress = "Preparing";
            await this.SendUploadSelectionCommand("ui_upload");

            await this.Sync();

            await this.Sleep(200);

            this.progress = "Cleaning";
            await this.ExtendedEraseMemory(sectors_to_erase);

            await this.WriteMemory(ui_bin, this.User_Sector_Start_Address);

            await this.ClosePortAndNull();

            this.Log("Update Complete.");
            this.progress = "Update complete";
        }
        catch (exception)
        {
            await this.ClosePortAndNull();

            console.error(exception);
        }
    }

    FlashNet(net_bin: number[])
    {
        this.mode = "net_upload";
    }

    async ConnectToHactar(filters: Object[]): Promise<boolean>
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

    async SendUploadSelectionCommand(command: String)
    {
        if (command != 'ui_upload' && command != 'net_upload')
            throw `Error. ${command} is an invalid command`;

        this.mode = command;
        let enc = new TextEncoder()

        // Get the response
        let reply = await this.WriteBytesWaitForACK(enc.encode("ui_upload"), 4000);
        if (reply == this.NO_REPLY)
        {
            throw "Failed to move Hactar into upload mode";
        }

        if (command == "ui_upload")
        {
            // ChangeParity("even");
            await this.OpenPort("even");
            this.Log("Activating UI Upload Mode: SUCCESS");
            this.Log("Update uart to parity: EVEN");

            reply = await this.ReadByte(5000);

            if (reply != this.READY)
                throw "Hactar took too long to get ready";
        }

    }

    async Sync(retry: number = 5)
    {
        let reply: number = await this.WriteByteWaitForACK(
            this.Commands.sync, retry, false);

        if (reply == this.NACK)
            throw "Activating device: FAILED";
        else if (reply == this.NO_REPLY)
            throw "Activating device: NO REPLY";

        this.Log("Activating device: SUCCESS");
    }


    async ReadMemory(address: number[], num_bytes: number,
        retry: number = 1)
    {
        let reply: number = await this.WriteByteWaitForACK(
            this.Commands.read_memory, retry);

        if (reply == this.NACK)
            throw "NACK was received during Read Memory";
        else if (reply == this.NO_REPLY)
            throw "NO REPLY was received during Read Memory";

        address.push(this.CalculateChecksum(address));

        reply = await this.WriteBytesWaitForACK(new Uint8Array(address));
        if (reply == this.NACK)
            throw "NACK was received after sending memory address";
        else if (reply == this.NO_REPLY)
            throw "NO REPLY received after sending memory address";

        reply = await this.WriteByteWaitForACK(num_bytes - 1);
        if (reply == this.NACK)
            throw "NACK was received after sending num bytes to receive";
        else if (reply == this.NO_REPLY)
            throw "NO REPLY received after sending num bytes to receive";

        let recv_data = await this.ReadBytesExcept(num_bytes);
        return recv_data
    }


    async ExtendedEraseMemory(sectors: number[]): Promise<boolean>
    {
        this.Log(`Erase: Sectors ${sectors}`);

        let reply: number = await this.WriteByteWaitForACK(
            this.Commands.extended_erase);
        if (reply == this.NACK)
            throw "NACK was received after sending erase command";
        else if (reply == this.NO_REPLY)
            throw "No reply was received after sending erase command";

        // TODO error check sectors

        // Number of sectors starts at 0x00 0x00. So 0x00 0x00 means delete
        // 1 sector.
        let num_sectors = this.ConvertToByteArray([sectors.length - 1], 2);

        // Convert sectors into bytes
        let byte_sectors = this.ConvertToByteArray(sectors, 2);

        // Await the two and join them together
        let data = num_sectors.concat(byte_sectors);

        let checksum = [this.CalculateChecksum(data)];

        // Concat the checksum to data
        data = data.concat(checksum);

        this.Log(`Erase: STARTED`);

        reply = await this.WriteBytesWaitForACK(new Uint8Array(data), 10000);
        if (reply == this.NACK)
        {
            throw "Failed to erase";
        }
        else if (reply == this.NO_REPLY)
        {
            throw "Failed to erase, no reply received";
        }

        const Max_Attempts = 15;
        const mem_bytes_sz = 256;
        const expected_mem: number[] = Array(mem_bytes_sz - 1);
        expected_mem.fill(255);

        let mem: number[] = Array(mem_bytes_sz - 1);
        let read_count = 0;
        let bytes_verified = 0;
        let total_bytes_to_verify = 0;

        await sectors.forEach(sector_idx =>
        {
            total_bytes_to_verify += this.Defined_Sectors[sector_idx].size;
        });


        let percent_verified =
            Math.floor((bytes_verified / total_bytes_to_verify) * 100);
        this.Log(`Verifying erase: ${percent_verified}% verified`);

        let sector_idx = -1;
        for (let i = 0; i < sectors.length; ++i)
        {
            sector_idx = sectors[i];
            let memory_address = this.Defined_Sectors[sector_idx].addr;
            let end_of_sector = this.Defined_Sectors[sector_idx].addr +
                this.Defined_Sectors[sector_idx].size;

            while (memory_address != end_of_sector)
            {
                read_count = 0;
                percent_verified =
                    Math.floor((bytes_verified / total_bytes_to_verify) * 100);
                this.Log(`Verifying erase: ${percent_verified}% verified`, true);

                let compare = false;
                do
                {
                    let memory_address_bytes = this.ConvertToByteArray([memory_address], 4);
                    mem = await this.ReadMemory(memory_address_bytes, mem_bytes_sz);
                    read_count += 1;

                    compare = this.MemoryCompare(mem, expected_mem);

                    if (compare)
                    {
                        this.Log(`Sector [${sector_idx}] not verified. Retry: ${read_count}`);
                    }
                } while ((compare) && read_count != Max_Attempts);

                if (read_count >= Max_Attempts && compare)
                {
                    throw `Verifying: Failed to verify sector [${sector_idx}]`;
                }

                memory_address += mem_bytes_sz;
                bytes_verified += mem_bytes_sz;

            }
        }

        // Don't actually need to do the math here
        this.Log("Verifying erase: 100% verified", true);
        this.Log("Erase: COMPLETE");
        return true;
    }

    async WriteMemory(data: number[], start_addr: number, retry: number = 1)
    {
        const Max_Num_Bytes = 256;
        let addr = start_addr;
        let file_addr = 0;

        let percent_flashed = 0;

        const total_bytes = data.length;

        this.Log("Write to Memory: Started");
        this.Log(`Address: ${addr.toString(16)}`)
        this.Log(`Byte Stream Size: ${total_bytes.toString(16)}`);

        this.Log(`Flashing: ${percent_flashed}%`);
        this.progress = `Updating: ${percent_flashed}%`;

        let reply = -1;
        while (file_addr < total_bytes)
        {
            percent_flashed = (file_addr / total_bytes);

            this.Log(`Flashing: ${Math.floor(percent_flashed*100)}%`, true);
            this.progress = `Updating: ${Math.floor(percent_flashed*50)}%`;

            reply = await this.WriteByteWaitForACK(this.Commands.write_memory);
            if (reply == this.NACK)
                throw "NACK was received after sending write command";
            else if (reply == this.NO_REPLY)
                throw "NO REPLY received after sending write command";

            let write_address_bytes = this.ConvertToByteArray([addr], 4);
            let checksum = this.CalculateChecksum(write_address_bytes);

            write_address_bytes.push(checksum);
            reply = await this.WriteBytesWaitForACK(new Uint8Array(write_address_bytes));
            if (reply == this.NACK)
                throw "NACK was received after sending write command";
            else if (reply == this.NO_REPLY)
                throw "NO REPLY received after sending write command";

            // Get the contents of the binary
            // Exclusive  slice
            let chunk = data.slice(file_addr, file_addr + Max_Num_Bytes);
            let chunk_size = chunk.length;

            while (chunk.length % 4 != 0)
                chunk.push(0);

            chunk.unshift(chunk.length - 1);
            chunk.push(this.CalculateChecksum(chunk));

            reply = await this.WriteBytesWaitForACK(new Uint8Array(chunk));
            if (reply == this.NACK)
                throw `NACK was received while writing to addr: ${addr}`;
            else if (reply == this.NO_REPLY)
                throw `NO REPLY was received while writing to addr: ${addr}`;

            file_addr += chunk_size;
            addr += chunk_size;
        }

        this.Log(`Flashing: 100%`, true);

        addr = start_addr;
        file_addr = 0;

        this.Log(`Verifying write: 0%`);
        this.progress = `Updating: 50%`;

        while (file_addr < total_bytes)
        {
            percent_flashed = (file_addr / total_bytes);
            this.Log(`Verifying write: ${Math.floor(percent_flashed*100)}%`, true);
            this.progress = `Updating: ${(Math.floor(percent_flashed*50)) + 50}%`;

            const chunk = data.slice(file_addr, file_addr + Max_Num_Bytes);
            const addr_bytes = this.ConvertToByteArray([addr], 4);
            const mem = await this.ReadMemory(addr_bytes, chunk.length);

            const compare = this.MemoryCompare(mem, chunk);
            if (!compare)
                throw `Failed to verify at memory address ${addr}`;

            addr += mem.length;
            file_addr += chunk.length;
        }

        this.Log("Verifying write: 100%", true);
        this.Log("Write: COMPLETE");

        return this.ACK;
    }

    // Helper functions
    CalculateChecksum(array: number[]): number
    {
        return array.reduce((a, b) => a ^ b);
    }

    ConvertToByteArray(array: number[], bytes_per_element: number)
    {
        if (bytes_per_element > 4)
            throw "JS can't handle more than like 6 byte integers because \
                    precision";

        let array_bytes = []
        let byte_count = 0;
        let value = 0;
        let mask = 0;
        let bits = 0;
        let byte = 0;

        for (let i = 0; i < array.length; ++i)
        {
            byte_count = bytes_per_element;
            value = array[i];


            mask = (0x100 ** (bytes_per_element)) - 1;
            while (byte_count > 0)
            {
                bits = (byte_count - 1) * 8;
                byte = (value & mask) >>> bits;
                mask = mask >>> 8;
                value = value & mask;

                array_bytes.push(byte);

                byte_count--;
            }
        }

        return array_bytes;
    }


    MemoryCompare(arr1: number[], arr2: number[]): boolean
    {
        if (arr1.length != arr2.length) return false;

        for (let i = 0; i < arr1.length; ++i)
        {
            if (arr1[i] != arr2[i])
                return false;
        }

        return true;
    }

    async ListenForBytes()
    {
        this.released_reader = false;
        this.Log(`Port readable: ${this.port.readable}, Stop: ${this.stop}`)
        while (this.port.readable && !this.stop)
        {
            this.Log("Listening to bytes on serial port");
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
            } catch (error)
            {
                this.Log(error);
                return;
            } finally
            {
                this.reader.releaseLock();
                this.released_reader = true;
            }
        }
    }

    async ReadBytes(num_bytes: number, timeout: number = 2000): Promise<number[]>
    {
        let num = num_bytes
        let bytes: number[] = []

        let start_time = Date.now();
        while (num > 0 && (Date.now() - start_time) < timeout)
        {
            if (this.in_buffer.length == 0)
            {
                await this.Sleep(0.001)
                continue;
            }
            bytes.push(this.in_buffer[0]);
            this.in_buffer.shift();
            num--;
        }

        if (bytes.length < 1)
            return [this.NO_REPLY];

        return bytes;
    }

    async ReadBytesExcept(num_bytes: number, timeout: number = 2000): Promise<number[]>
    {
        let result: number[] = await this.ReadBytes(num_bytes, timeout);
        if (result[0] == this.NO_REPLY)
        {
            throw "Error, didn't receive any bytes";
        }

        return result;
    }

    async ReadByte(timeout: number = 2000): Promise<number>
    {
        let res = await this.ReadBytes(1, timeout)

        return res[0];
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
            data.push(byte ^ 0xFF)

        let num = retry;
        while (num--)
        {
            await this.WriteBytes(new Uint8Array(data));

            let reply = await this.ReadByte(timeout);

            if (reply == this.NO_REPLY)
                continue;

            return reply;
        }

        return this.NO_REPLY;
    }

    async WriteBytesWaitForACK(bytes: Uint8Array, timeout: number = 2000,
        retry: number = 1)
    {
        let num = retry;
        while (num--)
        {
            await this.WriteBytes(bytes);

            let reply = await this.ReadByte(timeout);

            if (reply == this.NO_REPLY)
                continue;

            return reply;
        }

        return this.NO_REPLY;
    }

    async ClosePort()
    {
        if (!this.open)
        {
            this.Log("Port not open");
            return;
        }

        this.stop = true;
        this.reader.cancel();

        await this.reader_promise;
        await this.port.close();
        this.open = false;

        this.Log("Close hacatar");
    }

    async ClosePortAndNull()
    {
        await this.ClosePort();
        this.port = null;
    }

    Log(text: any, replace_previous: boolean = false)
    {
        console.log(`replace: ${replace_previous}: ${text}`);
        this.logs.push({ text, replace_previous });
    }

    SectorsToErase(data_len: number): number[]
    {
        let total_bytes = data_len;
        let sectors: number[] = []
        let sector_idx = 0;

        while (total_bytes > 0 && sector_idx < this.Defined_Sectors.length)
        {
            total_bytes -= this.Defined_Sectors[sector_idx].size;
            sectors.push(sector_idx++);
        }

        return sectors;
    }

    async OpenPort(parity: string)
    {
        this.Log("Open port");
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

    async Sleep(delay: number)
    {
        await new Promise(r => setTimeout(r, delay));
    }

    ACK: number = 0x79;
    READY: number = 0x80;
    NACK: number = 0x1F;
    NO_REPLY: number = -1;

    Commands = {
        "sync": 0x7F,
        "get": 0x00,
        "get_version_and_read_protection_status": 0x01,
        "get_id": 0x02,
        "read_memory": 0x11,
        "go": 0x21,
        "write_memory": 0x31,
        "erase": 0x43,
        "extended_erase": 0x44,
        "write_protect": 0x63,
        "write_unprotect": 0x73,
        "readout_protect": 0x82,
        "readout_unprotect": 0x92
    };

    User_Sector_Start_Address = 0x08000000;

    Defined_Sectors = [
        { "size": 0x00004000, "addr": 0x08000000 },
        { "size": 0x00004000, "addr": 0x08004000 },
        { "size": 0x00004000, "addr": 0x08008000 },
        { "size": 0x00004000, "addr": 0x0800C000 },
        { "size": 0x00010000, "addr": 0x08010000 },
        { "size": 0x00020000, "addr": 0x08020000 },
        { "size": 0x00020000, "addr": 0x08040000 },
        { "size": 0x00020000, "addr": 0x08060000 },
        { "size": 0x00020000, "addr": 0x08080000 },
        { "size": 0x00020000, "addr": 0x080A0000 },
        { "size": 0x00020000, "addr": 0x080C0000 },
        { "size": 0x00020000, "addr": 0x080E0000 }
    ];


    erasing_verification: number = 0;
    flashing_progress: number = 0;
    flash_verification: number = 0;
    mode: String = ""
    logs: any = [];
    progress: any = [];
    port: any = null;
    reader: any = null;
    released_reader: boolean = true;
    in_buffer: number[] = [];
    stop: boolean = false;
    open: boolean = false;
    reader_promise: any = null;
};

export default HactarFlasher;