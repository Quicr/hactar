<script setup lang="ts">

// @ts-ignore
import bin from "./ui_bin.js";

const data: number[] = bin;

const ACK: number = 0x79;
const READY: number = 0x80;
const NACK: number = 0x1F;
const NO_REPLY: number = -1;

const Commands = {
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

const User_Sector_Start_Address = 0x08000000;

const Defined_Sectors = [
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

var port: any = null;
var stop = false;
var open = false;
var in_buffer: number[] = [];
var out_buffer = [];

let reader_promise: Promise<void>;
let reader: any;
let released_reader = true;
let writer: any;

async function CalculateChecksum(array: number[]): Promise<number>
{
    return array.reduce((a, b) => a ^ b);
}

async function sleep(delay: number)
{
    await new Promise(r => setTimeout(r, delay));
}

async function ClosePort()
{
    if (!open)
    {
        console.log("port not open");
        return;
    }

    stop = true;
    reader.cancel();

    await reader_promise;
    await port.close();
    open = false;

    console.log("Close hacatar");
}

async function ClosePortAndNull()
{
    await ClosePort();
    port = null;
}

async function OpenPort(parity: string)
{
    console.log("open port");
    const options = {
        baudRate: 115200,
        dataBits: 8,
        stopBits: 1,
        parity: parity
    };

    await ClosePort();

    stop = false;
    released_reader = true;
    in_buffer = [];

    await port.open(options);

    open = true;

    // Start listening
    reader_promise = ListenForBytes()
}

async function ConnectToHactar()
{

    if ('serial' in navigator)
    {
        if (port) return;
        const filters = [
            { usbVendorId: 6790, usbProductId: 29987 }
        ];

        port = await (navigator as any).serial.requestPort({ filters });

        if (!port) return

        await OpenPort("none");
    }
    else
    {
        alert("Web serial is not enabled in your browser");
    }
    // Send off request to server for the latest file

    // Save it locally on the web browser

    //

    // Delete the hactar file
}

async function ListenForBytes()
{
    console.log("start listening function")
    released_reader = false;
    console.log(`port readable: ${port.readable}, stop: ${stop}`)
    while (port.readable && !stop)
    {
        console.log("start reading");
        reader = port.readable.getReader();
        try
        {

            while (!stop)
            {
                const { value, done } = await reader.read()

                if (done)
                {
                    break;
                }

                value.forEach((element: any) =>
                {
                    in_buffer.push(element);
                });
            }
        } catch (error)
        {
            console.log(error);
            return;
        } finally
        {
            reader.releaseLock();
            released_reader = true;
        }
    }
}

async function ReadBytes(num_bytes: number, timeout: number = 2000): Promise<number[]>
{
    let num = num_bytes
    let bytes: number[] = []

    let start_time = Date.now();
    while (num > 0 && (Date.now() - start_time) < timeout)
    {
        if (in_buffer.length == 0)
        {
            await sleep(0.00001)
            continue;
        }
        bytes.push(in_buffer[0]);
        in_buffer.shift();
        num--;
    }

    if (bytes.length < 1)
        return [NO_REPLY];

    return bytes;
}

async function ReadBytesExcept(num_bytes: number, timeout: number = 2000): Promise<number[]>
{
    let result: number[] = await ReadBytes(num_bytes, timeout);
    if (result[0] == NO_REPLY)
    {
        throw "Error, didn't receive any bytes";
    }

    return result;
}

async function ReadByte(timeout: number = 2000): Promise<number>
{
    let res = await ReadBytes(1, timeout)

    return res[0];
}

async function WriteBytes(bytes: Uint8Array)
{
    const writer = port.writable.getWriter();
    await writer.write(bytes);
    writer.releaseLock();
}

async function WriteByteWaitForACK(byte: number, retry: number = 1,
    compliment: boolean = true, timeout = 2000)
{

    let data: number[] = [byte];
    if (compliment)
        data.push(byte ^ 0xFF)

    let num = retry;
    while (num--)
    {
        await WriteBytes(new Uint8Array(data));

        let reply = await ReadByte(timeout);

        if (reply == NO_REPLY)
            continue;

        return reply;
    }

    return NO_REPLY;
}

async function WriteBytesWaitForACK(bytes: Uint8Array, timeout: number = 2000,
    retry: number = 1)
{
    let num = retry;
    while (num--)
    {
        await WriteBytes(bytes);

        let reply = await ReadByte(timeout);

        if (reply == NO_REPLY)
            continue;

        return reply;
    }

    return NO_REPLY;
}

async function FlashFirmware()
{
    try
    {

        await SendUploadSelectionCommand("ui_upload");

        await SendSync();

        await sleep(200);

        // let addr = await ConvertToByteArray([User_Sector_Start_Address], 4);
        // let memory = await ReadMemory(addr, 1);
        // console.log(memory);

        // console.log(await ExtendedEraseMemory([0, 1, 2, 3, 4, 5]));

        await WriteMemory(data, User_Sector_Start_Address);

        await ClosePortAndNull();

        console.log("done");
    }
    catch (exception)
    {
        console.error(exception);
    }
}

async function SendUploadSelectionCommand(command: string)
{
    let enc = new TextEncoder()

    // Get the response
    let reply = await WriteBytesWaitForACK(enc.encode("ui_upload"), 4000);
    if (reply == NO_REPLY)
    {
        throw "Failed to move Hactar into upload mode";
    }

    if (command == "ui_upload")
    {
        // ChangeParity("even");
        await OpenPort("even");
        console.log("Activating UI Upload Mode: SUCCESS");
        console.log("Update uart to parity: EVEN");

        reply = await ReadByte(5000);

        if (reply != READY)
            throw "Hactar took too long to get ready";

        // Give hactar some time to catch up
        // await sleep(800);
    }

}

async function SendSync(retry: number = 5)
{
    let reply: number = await WriteByteWaitForACK(Commands.sync, retry, false);

    if (reply == NACK)
        throw "Activating device: FAILED";
    else if (reply == NO_REPLY)
        throw "Activating device: NO REPLY";

    console.log("Activating device: SUCCESS");
}

async function ReadMemory(address: number[], num_bytes: number,
    retry: number = 1)
{
    let reply: number = await WriteByteWaitForACK(Commands.read_memory, retry);
    if (reply == NACK)
        throw "NACK was received during Read Memory";
    else if (reply == NO_REPLY)
        throw "NO REPLY was received during Read Memory";

    address.push(await CalculateChecksum(address));

    reply = await WriteBytesWaitForACK(new Uint8Array(address));
    if (reply == NACK)
        throw "NACK was received after sending memory address";
    else if (reply == NO_REPLY)
        throw "NO REPLY received after sending memory address";

    reply = await WriteByteWaitForACK(num_bytes - 1);
    if (reply == NACK)
        throw "NACK was received after sending num bytes to receive";
    else if (reply == NO_REPLY)
        throw "NO REPLY received after sending num bytes to receive";

    let recv_data = await ReadBytesExcept(num_bytes);
    return recv_data
}

async function ExtendedEraseMemory(sectors: number[]): Promise<boolean>
{
    let reply: number = await WriteByteWaitForACK(Commands.extended_erase);
    if (reply == NACK)
        throw "NACK was received after sending erase command";
    else if (reply == -1)
        throw "No reply was received after sending erase command";

    // TODO error check sectors

    // Number of sectors starts at 0x00 0x00. So 0x00 0x00 means delete
    // 1 sector.
    let num_sectors = ConvertToByteArray([sectors.length - 1], 2);

    // Convert sectors into bytes
    let byte_sectors = ConvertToByteArray(sectors, 2);

    // Let the above two run in parallel

    // Await the two and join them together
    let data = (await num_sectors).concat((await byte_sectors));

    let checksum = [await CalculateChecksum(data)];

    // Concat the checksum to data
    data = data.concat(checksum);

    console.log(`Erase: Sectors ${sectors}`);
    console.log(`Erase: STARTED`);

    reply = await WriteBytesWaitForACK(new Uint8Array(data), 10000);
    if (reply == -1)
    {
        throw "Failed to erase, no reply received";
    }
    else if (reply == NACK)
    {
        throw "Failed to erase";
    }

    const Max_Attempts = 10;
    const mem_bytes_sz = 256;
    const expected_mem: number[] = Array(mem_bytes_sz - 1);
    expected_mem.fill(255);

    let mem: number[] = Array(mem_bytes_sz - 1);
    let read_count = 0;
    let bytes_verified = 0;
    let total_bytes_to_verify = 0;

    await sectors.forEach(sector_idx =>
    {
        total_bytes_to_verify += Defined_Sectors[sector_idx].size;
    });

    console.log(`num bytes to verify = ${total_bytes_to_verify.toString(16)}`);
    let percent_verified =
        Math.floor((bytes_verified / total_bytes_to_verify) * 100);

    // sectors.forEach(async (sector_idx) =>
    let sector_idx = -1;
    for (let i = 0; i < sectors.length; ++i)
    {
        sector_idx = sectors[i];
        let memory_address = Defined_Sectors[sector_idx].addr;
        let end_of_sector = Defined_Sectors[sector_idx].addr +
            Defined_Sectors[sector_idx].size;

        console.log(`Verify sector [${sector_idx}] bytes verified: ${bytes_verified.toString(16)}`);
        console.log(`Addr [${memory_address.toString(16)}] end of sector: ${end_of_sector.toString(16)}`);

        while (memory_address != end_of_sector)
        {
            read_count = 0;
            percent_verified =
                Math.floor((bytes_verified / total_bytes_to_verify) * 100);
            console.log(`Verifying erase: ${percent_verified}% verified`);

            let compare = false;
            do
            {
                let memory_address_bytes = await ConvertToByteArray([memory_address], 4);
                mem = await ReadMemory(memory_address_bytes, mem_bytes_sz);
                read_count += 1;

                compare = await MemoryCompare(mem, expected_mem);

                if (compare)
                {
                    console.log(`Sector [${sector_idx}] not verified. Retry: ${read_count}`);
                }
            } while ((compare) && read_count != Max_Attempts);

            if (read_count != Max_Attempts && compare)
            {
                throw `Verifying: Failed to verify sector [${sector_idx}]`;
            }

            memory_address += mem_bytes_sz;
            bytes_verified += mem_bytes_sz;

        }
    }

    // Don't actually need to do the math here
    console.log("Verifying erase: 100% verified");
    console.log("Erase: COMPLETE");
    return true;
}

async function WriteMemory(data: number[], start_addr: number, retry: number = 1)
{
    const Max_Num_Bytes = 256;
    let addr = start_addr;
    let file_addr = 0;

    let percent_flashed = 0;

    const total_bytes = data.length;

    console.log("Write to Memory: Started");
    console.log(`Address: ${addr.toString(16)}`)
    console.log(`Byte Stream Size: ${total_bytes.toString(16)}`);

    let reply = -1;
    while (file_addr < total_bytes)
    {
        percent_flashed = Math.floor((file_addr / total_bytes) * 100);
        console.log(`Flashing: ${percent_flashed}%`);
        reply = await WriteByteWaitForACK(Commands.write_memory);
        if (reply == NACK)
            throw "NACK was received after sending write command";
        else if (reply == NO_REPLY)
            throw "NO REPLY received after sending write command";

        let write_address_bytes = await ConvertToByteArray([addr], 4);
        let checksum = await CalculateChecksum(write_address_bytes);

        write_address_bytes.push(checksum);
        reply = await WriteBytesWaitForACK(new Uint8Array(write_address_bytes));
        if (reply == NACK)
            throw "NACK was received after sending write command";
        else if (reply == NO_REPLY)
            throw "NO REPLY received after sending write command";

        // Get the contents of the binary
        // Exclusive  slice
        let chunk = data.slice(file_addr, file_addr + Max_Num_Bytes);
        let chunk_size = chunk.length;

        while (chunk.length % 4 != 0)
            chunk.push(0);

        chunk.unshift(chunk.length - 1);
        chunk.push(await CalculateChecksum(chunk));

        reply = await WriteBytesWaitForACK(new Uint8Array(chunk));
        if (reply == NACK)
            throw `NACK was received while writing to addr: ${addr}`;
        else if (reply == NO_REPLY)
            throw `NO REPLY was received while writing to addr: ${addr}`;

        file_addr += chunk_size;
        addr += chunk_size;
    }

    console.log(`Flashing: 100%`);

    addr = start_addr;
    file_addr = 0;

    while (file_addr < total_bytes)
    {
        percent_flashed = Math.floor((file_addr / total_bytes) * 100);
        console.log(`Verifying write: ${percent_flashed}%`);

        const chunk = data.slice(file_addr, file_addr + Max_Num_Bytes);
        const addr_bytes = await ConvertToByteArray([addr], 4);
        const mem = await ReadMemory(addr_bytes, chunk.length);

        const compare = MemoryCompare(mem, chunk);
        if (!compare)
            throw `Failed to verify at memory address ${addr}`;

        addr += mem.length;
        file_addr += chunk.length;
    }

    console.log("Verifying write: 100%");
    console.log("Write: COMPLETE");

    return ACK;
}

async function MemoryCompare(arr1: number[], arr2: number[]): Promise<boolean>
{
    if (arr1.length != arr2.length) return false;

    for (let i = 0; i < arr1.length; ++i)
    {
        if (arr1[i] != arr2[i])
            return false;
    }

    return true;
}

async function ConvertToByteArray(array: number[], bytes_per_element: number)
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

async function test()
{
    console.log(bin[2]);
}

</script>

<template>
    <main>
        <button @click="ConnectToHactar()">Flash hactar app</button>
        <button @click="ClosePortAndNull()">Close port</button>
        <button @click="FlashFirmware()">Flash</button>
        <button @click="test">test</button>
    </main>
</template>
