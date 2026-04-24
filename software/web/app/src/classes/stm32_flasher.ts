import Sleep from "./sleep";
import Serial from "./serial"
import logger from "./logger";
import { WriteByteWaitForACK, WriteBytesWaitForACK } from "./stm32_serial";

import { ACK, READY, NACK, NO_REPLY, MemoryCompare, ToByteArray, FromByteArray } from "./uart_utils"

import axios from "axios";

class STM32Flasher
{
    async FlashSTM(serial: Serial, bin: number[], jump_to_usr_code: boolean = false)
    {
        this.progress = "Starting";

        await this.Sync(serial);

        const uid = await this.GetId(serial);

        await this.GetConfiguration(uid);

        await Sleep(200);

        let sectors_to_erase = this.SectorsToErase(bin.length);
        await this.ExtendedEraseMemory(serial, sectors_to_erase, false);

        await this.WriteMemory(serial, bin, this.User_Sector_Start_Address);

        if (jump_to_usr_code)
        {
            await this.Go(serial, this.User_Sector_Start_Address);
        }

        logger.Info("Update Complete.");
        this.progress = "Update complete";
    }

    async Sync(serial: Serial, retry: number = 5)
    {
        let reply: number = await WriteByteWaitForACK(serial,
            this.Commands.sync, retry, false);

        if (reply == NACK)
        {
            logger.Error("Activating device: FAILED");
            throw "Activating device: FAILED";
        }
        else if (reply == NO_REPLY)
        {
            logger.Error("Activating device: NO REPLY");
            throw "Activating device: NO REPLY";
        }

        logger.Info("Activating device: SUCCESS");
    }

    async GetId(serial: Serial, retry: number = 5)
    {
        let reply: number = await WriteByteWaitForACK(serial,
            this.Commands.get_id,
            retry,
            true);

        if (reply == NACK)
        {
            logger.Error("Getting ID: Failed");
            throw "Getting ID: Failed";
        }
        else if (reply == NO_REPLY)
        {
            logger.Error("Getting ID: NO REPLY");
            throw "Getting ID: No reply";
        }

        // Read the next byte which should be the number of bytes incoming - 1
        let num_bytes = await serial.ReadByte();


        // NOTE there is a bug in the bootloader for stm, or there is a
        // inaccuracy in the an3155 datasheet that says that a single ACK
        // is sent after receiving the get_id command.
        // However, it seems two ACK's are sent
        if (num_bytes == ACK)
        {
            num_bytes = await serial.ReadByte();
        }
        else if (num_bytes == NACK)
        {
            logger.Error("NACK was received while trying to get the number of bytes in GetID");
            throw "NACK was received while trying to get the number of bytes in GetID";
        }

        // Get the pid in bytes format
        const pid_bytes = await serial.ReadBytes(num_bytes+1);

        // Wait for an ack
        reply = await serial.ReadByte(1);

        if (reply == NACK)
        {
            logger.Error("NACK was received during GetID");
            throw "NACK was received during GetID";
        }
        else if (reply == NO_REPLY)
        {
            logger.Error("No reply was received during GetID");
            throw "No reply was received during GetID";
        }

        const pid = FromByteArray(pid_bytes, "big");
        logger.Debug(`Chip ID: ${pid}`);

        return pid;
    }

    async GetConfiguration(uid: number)
    {
        // Get a binary from the server
        let res = await axios.get(`http://localhost:7775/stm_configuration?uid=${uid}`);
        this.User_Sector_Start_Address = res.data["usr_start_addr"];
        this.Defined_Sectors = res.data["sectors"]
    }

    async ReadMemory(serial: Serial, address: number[], num_bytes: number,
        retry: number = 1)
    {
        let reply: number = await WriteByteWaitForACK(serial,
            this.Commands.read_memory, retry);

        if (reply == NACK)
        {
            logger.Error("NACK was received during Read Memory");
            throw "NACK was received during Read Memory";
        }
        else if (reply == NO_REPLY)
        {
            logger.Error("NO REPLY was received during Read Memory");
            throw "NO REPLY was received during Read Memory";
        }

        address.push(this.CalculateChecksum(address));

        reply = await WriteBytesWaitForACK(serial,new Uint8Array(address));
        if (reply == NACK)
        {
            logger.Error("NACK was received after sending memory address");
            throw "NACK was received after sending memory address";
        }
        else if (reply == NO_REPLY)
        {
            logger.Error("NO REPLY received after sending memory address");
            throw "NO REPLY received after sending memory address";
        }

        reply = await WriteByteWaitForACK(serial, num_bytes - 1);
        if (reply == NACK)
        {
            logger.Error("NACK was received after sending num bytes to receive");
            throw "NACK was received after sending num bytes to receive";
        }
        else if (reply == NO_REPLY)
        {
            logger.Error("NO REPLY received after sending num bytes to receive");
            throw "NO REPLY received after sending num bytes to receive";
        }

        let recv_data = await serial.ReadBytesExcept(num_bytes);
        return recv_data
    }

    async CompareFlash(serial: Serial, chunk: number[], addr:number)
    {
        /**
         * Compares a byte array to the flash at the provided addr.
         * True if equal, false otherwise
         */

        const Max_Attempt = 10;
        let read_count = 0;
        let mem: number[] = [];
        let result: boolean = false

        while (!result && read_count < Max_Attempt)
        {
            mem = await this.ReadMemory(serial, ToByteArray([addr], 4), chunk.length);
            result = MemoryCompare(chunk, mem);
            read_count += 1;
        }

        return result;
    }

    async* FullVerify(serial: Serial, data: number[], start_addr: number): any
    {
        const Max_Attempts = 15;
        const Max_Num_Bytes = 256;
        const data_len = data.length;
        let data_addr = 0;
        let addr = start_addr;

        let status = {"addr": addr, "failed": false, "percent": 0};

        while (data_addr < data_len)
        {
            status["addr"] = addr;
            status["percent"] = (data_addr / data_len) * 100;
            yield status;

            const chunk = data.slice(data_addr, data_addr + Max_Num_Bytes);
            const addr_bytes = ToByteArray([addr], 4);
            const mem = await this.ReadMemory(serial, addr_bytes, chunk.length);

            if(!MemoryCompare(mem, chunk))
            {
                status["failed"] = true;
                break;
            }

            addr += chunk.length;
            data_addr += chunk.length;
        }

        status["percent"] = 100;
        yield status;

        // await sectors.forEach(sector_idx =>
        // {
        //     total_bytes_to_verify += this.Defined_Sectors[sector_idx].size;
        // });


        // let percent_verified =
        //     Math.floor((bytes_verified / total_bytes_to_verify) * 100);
        // logger.Info(`Verifying erase: ${percent_verified}% verified`);
        // this.progress = `Preparing: ${percent_verified}%`;

        // let sector_idx = -1;
        // for (let i = 0; i < sectors.length; ++i)
        // {
        //     sector_idx = sectors[i];
        //     let memory_address = this.Defined_Sectors[sector_idx].addr;
        //     let end_of_sector = this.Defined_Sectors[sector_idx].addr +
        //         this.Defined_Sectors[sector_idx].size;

        //     while (memory_address != end_of_sector)
        //     {
        //         read_count = 0;
        //         percent_verified =
        //             Math.floor((bytes_verified / total_bytes_to_verify) * 100);
        //         logger.Info(`Verifying erase: ${percent_verified}% verified`, true);
        //         this.progress = `Preparing: ${percent_verified}%`;

        //         let compare = false;
        //         do
        //         {
        //             let memory_address_bytes = ToByteArray([memory_address], 4);
        //             mem = await this.ReadMemory(serial, memory_address_bytes, mem_bytes_sz);
        //             read_count += 1;

        //             compare = MemoryCompare(mem, expected_mem);

        //             if (compare)
        //             {
        //                 logger.Info(`Sector [${sector_idx}] not verified. Retry: ${read_count}`);
        //             }
        //         } while ((compare) && read_count != Max_Attempts);

        //         if (read_count >= Max_Attempts && compare)
        //         {
        //             logger.Error(`Verifying: Failed to verify sector [${sector_idx}]`);
        //             throw `Verifying: Failed to verify sector [${sector_idx}]`;
        //         }

        //         memory_address += mem_bytes_sz;
        //         bytes_verified += mem_bytes_sz;

        //     }
        // }

        // // Don't actually need to do the math here
        // this.progress = `Preparing: 100%`;
        // logger.Info("Verifying erase: 100% verified", true);
        // logger.Info("Erase: COMPLETE");
        // return true;
    }

    async FastEraseVerify(serial: Serial, sectors: number[]): Promise<boolean>
    {
        logger.Info(`Erase: Sectors ${sectors}`);

        const Mem_Bytes_Sz = 256;
        const expected_mem: number[] = Array(Mem_Bytes_Sz - 1);
        expected_mem.fill(255);
        const num_sectors = sectors.length;
        let num_sectors_verified = 0;

        let percent_verified = 0;
        logger.Info(`Verifying erase: ${percent_verified}% verified`);
        this.progress = `Preparing: ${percent_verified}%`;
        const Defined_Sectors = this.Defined_Sectors;

        sectors.forEach(sector => {
            percent_verified =
                Math.floor((num_sectors_verified / num_sectors) * 100);
            logger.Info(`Verifying erase: ${percent_verified}% verified`, true);
            this.progress = `Preparing: ${percent_verified}%`;

            // Get the next address to verify
            const addr = Defined_Sectors[sector]["addr"];

            // Verify the flash
            if (!this.CompareFlash(serial, expected_mem, addr))
            {
                logger.Error(`Verifying: Failed to verify sector [${sector}]`);
                return false;
            }

            ++num_sectors_verified;
        });

        this.progress = `Preparing: 100%`;
        logger.Info("Verifying erase: 100% verified", true);
        logger.Info("Erase: COMPLETE");

        return true;
    }


    async ExtendedEraseMemory(serial: Serial,
        sectors: number[],
        fast_verify: boolean = true): Promise<boolean>
    {
        logger.Info(`Erase: Sectors ${sectors}`);

        let reply: number = await WriteByteWaitForACK(serial,
            this.Commands.extended_erase);
        if (reply == NACK)
        {
            logger.Error("NACK was received after sending erase command");
            throw "NACK was received after sending erase command";
        }
        else if (reply == NO_REPLY)
        {
            logger.Error("NO REPLY was received after sending erase command");
            throw "NO REPLY was received after sending erase command";
        }

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

        logger.Info(`Erase: STARTED`);
        this.progress = "Preparing";

        reply = await WriteBytesWaitForACK(serial, new Uint8Array(data), 10000);
        if (reply == NACK)
        {
            logger.Error("Failed to erase");
            throw "Failed to erase";
        }
        else if (reply == NO_REPLY)
        {
            logger.Error("Failed to erase, NO REPLY received");
            throw "Failed to erase, NO REPLY received";
        }

        if (fast_verify)
        {
            return await this.FastEraseVerify(serial, sectors);
        }
        else
        {
            // Create an array of the entire memory space to be verified against
            let total_bytes = 0;
            for (let i = 0; i < sectors.length; ++i)
            {
                total_bytes += this.Defined_Sectors[sectors[i]]["size"];
            }

            const data = new Array(total_bytes);
            data.fill(255);

            // Loop through generated results
            for await (const status of this.FullVerify(serial,
                                    data,
                                    this.Defined_Sectors[sectors[0]]["addr"]))
            {
                if (status["failed"])
                {
                    this.progress = `Failed to verify at ${status["addr"]}`;
                    logger.Info(`Failed to verify at ${status["addr"]}`);
                    throw `Failed to verify at ${status["addr"]}`;
                }

                logger.Info(`Verifying erase: ${Math.floor(status["percent"])}%`, true);
                this.progress = `Preparing: ${(Math.floor(status["percent"]))}%`;
            }
        }

        return true;
    }

    async WriteMemory(serial: Serial, data: number[], start_addr: number)
    {
        const Max_Num_Bytes = 256;
        let addr = start_addr;
        let file_addr = 0;

        let percent_flashed = 0;

        const total_bytes = data.length;

        logger.Info("Write to Memory: Started");
        logger.Info(`Address: ${addr.toString(16)}`)
        logger.Info(`Byte Stream Size: ${total_bytes.toString(16)}`);

        logger.Info(`Flashing: ${percent_flashed}%`);
        this.progress = `Updating: ${percent_flashed}%`;

        let reply = -1;
        while (file_addr < total_bytes)
        {
            percent_flashed = (file_addr / total_bytes);

            this.progress = `Updating: ${Math.floor(percent_flashed * 50)}%`;

            reply = await WriteByteWaitForACK(serial,this.Commands.write_memory);
            if (reply == NACK)
            {
                logger.Error("NACK was received after sending write command");
                throw "NACK was received after sending write command";
            }
            else if (reply == NO_REPLY)
            {
                logger.Error("NO REPLY received after sending write command");
                throw "NO REPLY received after sending write command";
            }

            let write_address_bytes = ToByteArray([addr], 4);
            let checksum = this.CalculateChecksum(write_address_bytes);

            write_address_bytes.push(checksum);

            logger.Info(`Flashing: ${Math.floor(percent_flashed * 100)}%`,
                true);
            reply = await WriteBytesWaitForACK(serial,new Uint8Array(write_address_bytes));
            if (reply == NACK)
            {
                logger.Error("NACK was received after sending write command");
                throw "NACK was received after sending write command";
            }
            else if (reply == NO_REPLY)
            {
                logger.Error("NO REPLY received after sending write command");
                throw "NO REPLY received after sending write command";
            }


            // Get the contents of the binary
            // Exclusive  slice
            let chunk = data.slice(file_addr, file_addr + Max_Num_Bytes);
            let chunk_size = chunk.length;

            while (chunk.length % 4 != 0)
                chunk.push(255);

            // Push the length of the chunk onto the bytes
            chunk.unshift(chunk.length - 1);
            chunk.push(this.CalculateChecksum(chunk));

            reply = await WriteBytesWaitForACK(serial,new Uint8Array(chunk));
            if (reply == NACK)
            {
                logger.Error(`NACK was received while writing to addr: ${addr}`);
                throw `NACK was received while writing to addr: ${addr}`;
            }
            else if (reply == NO_REPLY)
            {
                logger.Error(`NO REPLY was received while writing to addr: ${addr}`);
                throw `NO REPLY was received while writing to addr: ${addr}`;
            }

            file_addr += chunk_size;
            addr += chunk_size;
        }

        logger.Info(`Flashing: 100%`, true);

        addr = start_addr;
        file_addr = 0;

        logger.Info(`Verifying write: 0%`);
        this.progress = `Updating: 50%`;

        while (file_addr < total_bytes)
        {
            percent_flashed = (file_addr / total_bytes);
            logger.Info(`Verifying write: ${Math.floor(percent_flashed * 100)}%`, true);
            this.progress = `Updating: ${(Math.floor(percent_flashed * 50)) + 50}%`;

            const chunk = data.slice(file_addr, file_addr + Max_Num_Bytes);
            const addr_bytes = ToByteArray([addr], 4);
            const mem = await this.ReadMemory(serial, addr_bytes, chunk.length);

            const compare = MemoryCompare(mem, chunk);
            if (!compare)
            {
                logger.Error(`Failed to verify at memory address ${addr}`);
                throw `Failed to verify at memory address ${addr}`;
            }

            addr += mem.length;
            file_addr += chunk.length;
        }

        logger.Info("Verifying write: 100%", true);
        logger.Info("Write: COMPLETE");

        return ACK;
    }

    async Go(serial: Serial, addr: number)
    {
        logger.Info(`Jumping to address: ${addr.toString(16)}`);
        console.log(`Jumping to address: ${addr.toString(16)}`);

        let reply = await WriteByteWaitForACK(serial, this.Commands.go);

        if (reply == NACK)
        {
            logger.Info(`NACK received when trying to send Go command}`);
            return `NACK received when trying to send Go command}`;
        }
        else if (reply == NO_REPLY)
        {
            logger.Info(`NO REPLY received when trying to send Go command}`);
            return `NO REPLY received when trying to send Go command}`;
        }

        // Convert the address to bytes and add the checksum
        const addr_bytes = this.AddressToBytes(addr);

        reply = await WriteBytesWaitForACK(serial, addr_bytes);

        if (reply == NACK)
        {
            logger.Info(`NACK received when jumping to ${addr.toString(16)}`);
            return `NACK received when jumping to ${addr.toString(16)}`;
        }
        else if (reply == NO_REPLY)
        {
            logger.Info(`NO REPLY received when jumping to ${addr.toString(16)}`);
            return `NO REPLY received when jumping to ${addr.toString(16)}`;
        }

        logger.Info(`Jumped to address: ${addr.toString(16)}`);
        console.log(`Jumped to address: ${addr.toString(16)}`);
    }

    AddressToBytes(addr: number, append_checksum: boolean = true): Uint8Array
    {
        let bytes = ToByteArray([addr], 4);

        if (append_checksum)
        {
            let checksum = this.CalculateChecksum(bytes);
            bytes.push(checksum);
        }

        return new Uint8Array(bytes);
    }

    // Helper functions
    CalculateChecksum(array: number[]): number
    {
        return array.reduce((a, b) => a ^ b);
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

    User_Sector_Start_Address: number = 0;
    // Array of objects
    Defined_Sectors: any[] = [];
    erasing_verification: number = 0;
    flashing_progress: number = 0;
    flash_verification: number = 0;
    mode: String = ""
    logs: any = [];
    progress: any = [];
};

const stm32_flasher = new STM32Flasher();
export default stm32_flasher;