import { FromByteArray, ToByteArray } from "./uart_utils"


class ESP32SlipPacket
{
    /* Slip packet for esp32 bootloader.
     * |-------------------|
     * |Byte    name       |
     * |-------------------|
     * |0       Direction  |
     * |1       Command    |
     * |2-3     Size       |
     * |4-7     Checksum   |
     * |8..n    Data       |
     * |-------------------|
     * - NOTE: Data is stored in little endian format for multi-byte fields
     * - NOTE: Header is 8 bytes
    */

    // Constants
    END = 0xC0
    ESC = 0xDB
    ESC_END = 0xDC
    ESC_ESC = 0xDD

    OUTGOING = 0
    INCOMING = 1

    constructor(direction: number = 0, command: number = 0, size: number = 0)
    {
        this.data = Array(8).fill(0);
        this.data_length = 0;
    }

    static FromBytes(bytes: number[])
    {
        let packet = new ESP32SlipPacket();

        // TODO
        return packet;
    }

    SetHeader(direction: number = this.OUTGOING, command: number = 0,
        size: number = 0)
    {

    }

    SetDirection(direction: number)
    {
        if (direction != this.OUTGOING && direction != this.INCOMING)
        {
            throw "Direction must be either OUTGOING (0) or INCOMING (1)";
        }

        this.data[0] = direction;
    }

    SetCommand(command: number)
    {
        this.data[1] = command;
    }

    SetSize(size: number)
    {
        let size_bytes = ToByteArray([size], 2, "little");
        this.data[2] = size_bytes[0];
        this.data[3] = size_bytes[1];
    }

    Get(start_idx: number, num_bytes: number)
    {
        return FromByteArray(this.data.slice(start_idx, start_idx + num_bytes),
            "little");
    }

    GetDirection()
    {
        return this.data[0];
    }

    GetCommand()
    {
        return this.data[1];
    }

    GetSize()
    {
        return this.Get(2, 3);
    }

    GetDataField()
    {
        return this.data.slice(8, 8 + this.data_length);
    }

    PushData(ele: number, size: number = -1)
    {

    }

    PushDataArray(arr: number[], endian_format: string = "little",
        size: number = -1)
    {

    }

    GetDataLength()
    {
        return this.data_length;
    }

    SetCheckSum()
    {

    }

    SlipEncode(checksum: boolean = false)
    {

    }



    // Variables
    data: any;
    data_length: number;
}

export default ESP32SlipPacket;