<script setup lang="ts">

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
    { "size": 0x4000, "addr": 0x08000000 },
    { "size": 0x0400, "addr": 0x08004000 },
    { "size": 0x0400, "addr": 0x08008000 },
    { "size": 0x0400, "addr": 0x0800C000 },
    { "size": 0x010000, "addr": 0x08010000 },
    { "size": 0x020000, "addr": 0x08020000 },
    { "size": 0x020000, "addr": 0x08040000 },
    { "size": 0x020000, "addr": 0x08060000 },
    { "size": 0x020000, "addr": 0x08080000 },
    { "size": 0x020000, "addr": 0x080A0000 },
    { "size": 0x020000, "addr": 0x080C0000 },
    { "size": 0x020000, "addr": 0x080E0000 }
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
                    console.log(`recv ${element}`);
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
    var cancel: boolean = false;
    const timer = setTimeout(() =>
    {
        console.log("cancel")
        cancel = true;
    }, timeout);

    while (num > 0 && !cancel)
    {
        // Because js handles jobs weird, we need this timeout otherwise
        // it will keep trying to run this while loop without breaking and
        // allowing for the above timer to timeout.
        await sleep(10);
        if (in_buffer.length == 0)
            continue;
        bytes.push(in_buffer[0]);
        in_buffer.shift();
        num--;
    }

    if (!cancel)
        clearTimeout(timer);

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
    compliment: boolean = true)
{

    let data: number[] = [byte];
    if (compliment)
        data.push(byte ^ 0xFF)

    let num = retry;
    while (num--)
    {
        console.log(data);
        await WriteBytes(new Uint8Array(data));

        let reply = await ReadByte();

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

        let addr = await ConvertToByteArray([User_Sector_Start_Address], 4);
        let memory = await ReadMemory(addr, 1);
        console.log(memory);

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
    let reply = await WriteBytesWaitForACK(enc.encode("ui_upload"));
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
    console.info("send read memory");
    let reply: number = await WriteByteWaitForACK(Commands.read_memory, retry);
    if (reply == NACK)
        throw "NACK was received during Read Memory";
    else if (reply == NO_REPLY)
        throw "NO REPLY was received during Read Memory";

    address.push(await CalculateChecksum(address));

    console.log(address);

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

    let recv_data = ReadBytesExcept(num_bytes);
    return recv_data
}

async function SendExtendedEraseMemory(sectors: number[], special_erase = false)
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

    Defined_Sectors.forEach(sector =>
    {
        total_bytes_to_verify += sector.size;
    });

    let percent_verified =
        Math.floor((bytes_verified / total_bytes_to_verify) * 100);

    sectors.forEach(async (sector_idx) =>
    {
        let memory_address = Defined_Sectors[sector_idx].addr;
        let end_of_sector = Defined_Sectors[sector_idx].addr +
            Defined_Sectors[sector_idx].size;

        while (memory_address != end_of_sector)
        {
            read_count = 0;
            percent_verified =
                Math.floor((bytes_verified / total_bytes_to_verify) * 100);
            console.log(`Verifying erase: ${percent_verified}% verified`);

            mem.fill(0);

            // TODO array comparison doesn't work like this in js
            // TODO make a function to compare them.

            while ((mem != expected_mem) && read_count != Max_Attempts)
            {
                let memory_address_bytes = await ConvertToByteArray([memory_address], 4);
                mem = await ReadMemory(memory_address_bytes, mem_bytes_sz);
                read_count += 1;

                if (mem != expected_mem)
                {
                    console.log(`Sector [${sector_idx}] not verified. Retry: ${read_count}`);
                }
            }

            if (read_count != Max_Attempts && mem != expected_mem)
            {
                throw `Verifying: Failed to verify sector [${sector_idx}]`;
            }

            memory_address += mem_bytes_sz;
            bytes_verified += mem_bytes_sz;

        }
    });

    // Don't actually need to do the math here
    console.log("Verifying erase: 100% verified");
    console.log("Erase: COMPLETE");
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

</script>

<template>
    <main>
        <button @click="ConnectToHactar()">Flash hactar app</button>
        <button @click="ClosePortAndNull()">Close port</button>
        <button @click="FlashFirmware()">Flash</button>
        <button @click="ConvertToByteArray([0xff], 4)">test</button>
    </main>
</template>
