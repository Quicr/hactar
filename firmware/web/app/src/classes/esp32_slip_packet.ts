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
    static END = 0xC0
    static ESC = 0xDB
    static ESC_END = 0xDC
    static ESC_ESC = 0xDD

    static OUTGOING = 0
    static INCOMING = 1

    constructor(direction: number = ESP32SlipPacket.OUTGOING,
        command: number = 0, size: number = 0)
    {
        this.data = Array(8).fill(0);
        this.data_length = 0;
        this.update_encoding = true;
        this.encoded_data = [];
        this.SetHeader(direction, command, size);
    }

    static FromBytes(bytes: number[])
    {
        /// Expects a little endian byte stream

        if (bytes[0] != ESP32SlipPacket.END ||
            bytes[bytes.length - 1] != ESP32SlipPacket.END)
        {
            throw "Error. Start and end bytes are missing.";
        }

        let decoded_bytes = []

        // Skip the start byte
        let idx = 1
        let byte = 0;
        while (idx < bytes.length)
        {
            byte = bytes[idx];
            if (byte == ESP32SlipPacket.ESC)
            {
                if (bytes[idx + 1] == ESP32SlipPacket.ESC_END)
                {
                    decoded_bytes.push(ESP32SlipPacket.END);
                }
                else if (bytes[idx + 1] == ESP32SlipPacket.ESC_ESC)
                {
                    decoded_bytes.push(ESP32SlipPacket.ESC)
                }
                else
                {
                    throw `[ESP32SlipPacket.FromBytes(...)] Error. idx: ${idx}
                     value: ${byte} was not properly escaped`;
                }

                // Increment the idx by 1 extra to skip over the escaped value
                idx++;
            }
            else if (byte == ESP32SlipPacket.END)
            {
                // Found the end break out
                break;
            }
            else
            {
                decoded_bytes.push(byte);
            }

            idx++;
        }

        if (decoded_bytes.length < 8)
        {
            throw "[ESP32SlipPacket.FromBytes(...)] Error. " +
            "Decoded bytes passed in was less than the required 8 bytes."
        }

        let packet = new ESP32SlipPacket();
        for (let i = 0; i < 8; ++i)
        {
            packet.Set(i, decoded_bytes[i]);
        }

        let data_len = packet.GetDataLength();

        packet.PushDataArray(decoded_bytes.slice(8, 8 + data_len), "little");

        return packet;
    }

    Set(idx: number, value: number)
    {
        if (value > 255)
        {
            throw "[ESP32SlipPacket.Set(...)] Error. value must be less than 256";
        }

        this.data[idx] = value;
        this.update_encoding = true;
    }

    SetHeader(direction: number = ESP32SlipPacket.OUTGOING, command: number = 0,
        size: number = 0)
    {
        this.SetDirection(direction);
        this.SetCommand(command);
        this.SetSize(size);
    }

    SetDirection(direction: number)
    {
        if (direction != ESP32SlipPacket.OUTGOING &&
            direction != ESP32SlipPacket.INCOMING)
        {
            throw "Direction must be either OUTGOING (0) or INCOMING (1)";
        }

        this.Set(0, direction);
    }

    SetCommand(command: number)
    {
        this.Set(1, command);
    }

    SetSize(size: number)
    {
        let size_bytes = ToByteArray([size], 2, "little");

        this.Set(2, size_bytes[0]);
        this.Set(3, size_bytes[1]);
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
        // Pushes an element into the data ,fi the element >1 byte in size
        // it will be stored in little endian format
        let num_bytes = size;
        if (num_bytes == -1)
        {
            num_bytes = Math.floor((Math.max(this.GetBitLength(ele), 1) + 7) / 8);
        }
        let bytes = ToByteArray([ele], num_bytes, "little");

        let this_data = this.data;
        bytes.forEach((value: number) =>
        {
            this_data.push(value);
        });

        this.IncrementDataLength(bytes.length);
        this.update_encoding = true;
    }

    PushDataArray(arr: number[], endian_format: string = "little",
        size: number = -1)
    {
        // Pushes an array of data into the data array
        // - Expects the data array to be in big endian format

        const this_PushData = this.PushData;
        if (endian_format == "little")
        {
            // i is signed
            for (const ele of arr)
            {
                this.PushData(ele);
            }
        }
        else
        {
            for (const ele of arr)
            {
                this.PushData(ele);
            }
        }
        this.update_encoding = true;
    }

    GetDataLength()
    {
        return this.data_length;
    }

    GetBitLength(value: number)
    {
        let v = value;
        let bits = 0;
        while (v > 0)
        {
            v = v << 1;
            bits++;
        }

        return bits;
    }

    IncrementDataLength(inc: number)
    {
        // Reminder, little endian
        // Increment the first byte
        this.data[2] += inc;
        while (this.data[2] > 255)
        {
            // "Overflow" the first byte and increment the second byte
            this.data[2] -= 255;
            this.data[3]++;

            // Overflow the upper part of the length
            if (this.data[3] > 255)
            {
                this.data[3] = 0
            }
        }
        this.update_encoding = true;
    }

    SetCheckSum()
    {
        // Checksum seed
        let checksum = 0xEF;

        // All data sections actual data starts at idx 16
        let idx = 16;
        while (idx < this.data.length)
        {
            checksum ^= this.data[idx];
            idx++;
        }

        // Convert into bytes
        const bytes = ToByteArray([checksum], 4, "little");

        this.data[4] = bytes[0];
        this.data[5] = bytes[1];
        this.data[6] = bytes[2];
        this.data[7] = bytes[3];
        this.update_encoding = true;
    }

    SlipEncode(checksum: boolean = false)
    {
        // Not all packets require a checksum
        if (checksum)
        {
            this.SetCheckSum();
        }

        if (this.update_encoding == false)
        {
            return this.encoded_data;
        }


        this.encoded_data = [];

        // SLIP starts with an end byte
        this.encoded_data.push(ESP32SlipPacket.END);

        let idx = 0;
        let byte = 0;
        while (idx < this.data.length)
        {
            byte = this.data[idx];
            if (ESP32SlipPacket.END == byte)
            {
                this.encoded_data.push(ESP32SlipPacket.ESC);
                this.encoded_data.push(ESP32SlipPacket.ESC_END);
            }
            else if (ESP32SlipPacket.ESC == byte)
            {
                this.encoded_data.push(ESP32SlipPacket.ESC);
                this.encoded_data.push(ESP32SlipPacket.ESC_ESC);
            }
            else
            {
                this.encoded_data.push(byte);
            }
            idx += 1;
        }

        // Append the final end byte
        this.encoded_data.push(ESP32SlipPacket.END);

        this.update_encoding = false;

        return this.encoded_data;
    }



    // Variables
    data: any;
    data_length: number;
    update_encoding: boolean;
    encoded_data: Array<number>;
}

export default ESP32SlipPacket;