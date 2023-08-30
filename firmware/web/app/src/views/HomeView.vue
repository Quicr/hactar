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

const User_Sector_Start_Address = [0x08, 0x00, 0x00, 0x00];

const Sectors = [
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
var in_buffer: number[] = [];
var out_buffer = [];

let reader_promise: Promise<void>;
let reader: any;
let released_reader = true;
let writer: any;

async function CalculateChecksum(array: number[]) : Promise<number>
{
    return array.reduce((a, b) => a ^ b);
}

async function sleep(delay: number)
{
    await new Promise(r => setTimeout(r, delay));
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

        console.log("Open port");
        let res = await port.open({ baudRate: 115200 });
        stop = false;
        console.log(res);
        in_buffer = [];
        // Start listening
        reader_promise = ListenForBytes()
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
    released_reader = false;
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
        await WriteBytes(new Uint8Array(data));

        let reply = await ReadByte();

        if (reply == NO_REPLY)
            continue;

        return reply;
    }

    return NO_REPLY;
}

async function WriteBytesWaitForACK(bytes: Uint8Array, retry: number = 1)
{
    let num = retry;
    while (num--)
    {
        await WriteBytes(bytes);

        let reply = await ReadByte();

        if (reply == NO_REPLY)
            continue;

        return reply;
    }

    return NO_REPLY;
}

async function FlashFirmware()
{
    await SendUploadSelectionCommand("ui_upload");

    await SendSync();

    await ReadMemory(User_Sector_Start_Address, 256);

    stop = true;
    reader.cancel();
    // writer.cancel();
    await reader_promise;
    // while (released_reader)
    // {
    //   console.log("not closed");
    // }
    await port.close();
    port = null;
    console.log("done")
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
        port.parity = "even";
        console.log("Activating UI Upload Mode: SUCCESS");
        console.log("Update uart to parity: EVEN");

        reply = await ReadByte();

        if (reply != READY)
            throw "Hactar took too long to get ready";

        // Give hactar some time to catch up
        await sleep(800);
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

    address.push(await CalculateChecksum(address));

    console.log(address);

}

async function ClosePort()
{
    if (port != null)
    {

        stop = true;
        reader.cancel();
        await port.close()
        port = null;
        console.log("Close hactar");
    }
}

</script>

<template>
    <main>
        <button @click="ConnectToHactar()">Flash hactar app</button>
        <button @click="ClosePort()">Close port</button>
        <button @click="FlashFirmware()">Flash</button>
    </main>
</template>
