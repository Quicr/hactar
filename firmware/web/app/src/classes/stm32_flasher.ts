import Sleep from "./sleep";
import { ACK, READY, NACK, NO_REPLY, MemoryCompare, ToByteArray } from "./uart_utils"
import Serial from "./serial"

class STM32Flasher
{
    async FlashSTM(serial: Serial, ui_bin: number[])
    {
        let sectors_to_erase = this.SectorsToErase(ui_bin.length);

        this.progress = "Starting";

        await this.Sync(serial);

        await Sleep(200);

        await this.ExtendedEraseMemory(serial, sectors_to_erase);

        await this.WriteMemory(serial, ui_bin, this.User_Sector_Start_Address);

        this.Log("Update Complete.");
        this.progress = "Update complete";
    }

    async Sync(serial: Serial, retry: number = 5)
    {
        let reply: number = await serial.WriteByteWaitForACK(
            this.Commands.sync, retry, false);

        if (reply == NACK)
            throw "Activating device: FAILED";
        else if (reply == NO_REPLY)
            throw "Activating device: NO REPLY";

        this.Log("Activating device: SUCCESS");
    }


    async ReadMemory(serial: Serial, address: number[], num_bytes: number,
        retry: number = 1)
    {
        let reply: number = await serial.WriteByteWaitForACK(
            this.Commands.read_memory, retry);

        if (reply == NACK)
            throw "NACK was received during Read Memory";
        else if (reply == NO_REPLY)
            throw "NO REPLY was received during Read Memory";

        address.push(this.CalculateChecksum(address));

        reply = await serial.WriteBytesWaitForACK(new Uint8Array(address));
        if (reply == NACK)
            throw "NACK was received after sending memory address";
        else if (reply == NO_REPLY)
            throw "NO REPLY received after sending memory address";

        reply = await serial.WriteByteWaitForACK(num_bytes - 1);
        if (reply == NACK)
            throw "NACK was received after sending num bytes to receive";
        else if (reply == NO_REPLY)
            throw "NO REPLY received after sending num bytes to receive";

        let recv_data = await serial.ReadBytesExcept(num_bytes);
        return recv_data
    }


    async ExtendedEraseMemory(serial: Serial, sectors: number[]): Promise<boolean>
    {
        this.Log(`Erase: Sectors ${sectors}`);

        let reply: number = await serial.WriteByteWaitForACK(
            this.Commands.extended_erase);
        if (reply == NACK)
            throw "NACK was received after sending erase command";
        else if (reply == NO_REPLY)
            throw "No reply was received after sending erase command";

        // TODO error check sectors

        // Number of sectors starts at 0x00 0x00. So 0x00 0x00 means delete
        // 1 sector.
        let num_sectors = ToByteArray([sectors.length - 1], 2);

        // Convert sectors into bytes
        let byte_sectors = ToByteArray(sectors, 2);

        // Await the two and join them together
        let data = num_sectors.concat(byte_sectors);

        let checksum = [this.CalculateChecksum(data)];

        // Concat the checksum to data
        data = data.concat(checksum);

        this.Log(`Erase: STARTED`);

        reply = await serial.WriteBytesWaitForACK(new Uint8Array(data), 10000);
        if (reply == NACK)
        {
            throw "Failed to erase";
        }
        else if (reply == NO_REPLY)
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
        this.progress = `Preparing: ${percent_verified}%`;

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
                this.progress = `Preparing: ${percent_verified}%`;

                let compare = false;
                do
                {
                    let memory_address_bytes = ToByteArray([memory_address], 4);
                    mem = await this.ReadMemory(serial, memory_address_bytes, mem_bytes_sz);
                    read_count += 1;

                    compare = MemoryCompare(mem, expected_mem);

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
        this.progress = `Preparing: 100%`;
        this.Log("Verifying erase: 100% verified", true);
        this.Log("Erase: COMPLETE");
        return true;
    }

    async WriteMemory(serial: Serial, data: number[], start_addr: number, retry: number = 1)
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

            this.progress = `Updating: ${Math.floor(percent_flashed * 50)}%`;

            reply = await serial.WriteByteWaitForACK(this.Commands.write_memory);
            if (reply == NACK)
                throw "NACK was received after sending write command";
            else if (reply == NO_REPLY)
                throw "NO REPLY received after sending write command";

            let write_address_bytes = ToByteArray([addr], 4);
            let checksum = this.CalculateChecksum(write_address_bytes);

            write_address_bytes.push(checksum);

            this.Log(`Flashing: ${Math.floor(percent_flashed * 100)}%`);
            reply = await serial.WriteBytesWaitForACK(new Uint8Array(write_address_bytes));
            if (reply == NACK)
                throw "NACK was received after sending write command";
            else if (reply == NO_REPLY)
                throw "NO REPLY received after sending write command";


            // Get the contents of the binary
            // Exclusive  slice
            let chunk = data.slice(file_addr, file_addr + Max_Num_Bytes);
            let chunk_size = chunk.length;

            while (chunk.length % 4 != 0)
                chunk.push(255);

            // Push the length of the chunk onto the bytes
            chunk.unshift(chunk.length - 1);
            chunk.push(this.CalculateChecksum(chunk));

            reply = await serial.WriteBytesWaitForACK(new Uint8Array(chunk));
            if (reply == NACK)
                throw `NACK was received while writing to addr: ${addr}`;
            else if (reply == NO_REPLY)
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
            this.Log(`Verifying write: ${Math.floor(percent_flashed * 100)}%`, true);
            this.progress = `Updating: ${(Math.floor(percent_flashed * 50)) + 50}%`;

            const chunk = data.slice(file_addr, file_addr + Max_Num_Bytes);
            const addr_bytes = ToByteArray([addr], 4);
            const mem = await this.ReadMemory(serial, addr_bytes, chunk.length);

            const compare = MemoryCompare(mem, chunk);
            if (!compare)
                throw `Failed to verify at memory address ${addr}`;

            addr += mem.length;
            file_addr += chunk.length;
        }

        this.Log("Verifying write: 100%", true);
        this.Log("Write: COMPLETE");

        return ACK;
    }

    // Helper functions
    CalculateChecksum(array: number[]): number
    {
        return array.reduce((a, b) => a ^ b);
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
};

// TODO return a static flasher?
export default STM32Flasher;