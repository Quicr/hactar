import Sleep from "./sleep"
import { ACK, READY, NACK, NO_REPLY } from "./uart_utils"
import Serial from "./serial"
import ESP32SlipPacket from "./esp32_slip_packet"

class ESP32Flasher
{
    static SYNC = 0x08
    static FLASH_BEGIN = 0x02
    static FLASH_DATA = 0x03
    static FLASH_END = 0x04
    static SPI_SET_PARAMS = 0x0b
    static SPI_ATTACH = 0x0d
    static SPI_FLASH_MD5 = 0x13

    // Block size (1k)
    static Block_Size = 0x400

    async FlashESP(serial: Serial, net_bins: [])
    {
        console.log(net_bins);
        await this.Sync(serial);

        await this.AttachSPI(serial);
        await this.SetSPIParameters(serial);

        await this.Flash(serial, net_bins);
    }

    async Sync(serial: Serial)
    {
        let packet = new ESP32SlipPacket(ESP32SlipPacket.OUTGOING,
            ESP32Flasher.SYNC);

        packet.PushDataArray([0x07, 0x07, 0x12, 0x20], "big");
        packet.PushDataArray(Array(32).fill(0x55), "big");

        let reply = await this.WritePacketWaitForResponsePacket(serial, packet,
            ESP32Flasher.SYNC);

        if (reply == NO_REPLY)
        {
            console.log("Activating device: NO REPLY");
            throw "[ESP32Flasher.Sync(...)] Error. Failed to activate device.";
        }

        console.log("Activating device: SUCCESS");
    }

    async AttachSPI(serial: Serial)
    {
        const packet = new ESP32SlipPacket(ESP32SlipPacket.OUTGOING,
            ESP32Flasher.SPI_ATTACH);

        packet.PushDataArray(Array(8).fill(0));

        const reply = await this.WritePacketWaitForResponsePacket(serial,
            packet, ESP32Flasher.SPI_ATTACH);

        if (reply.End() == 1)
        {
            console.log("[ESP32Flasher.AttachSPI(...)] Error. "
                + "Failed to set spi parameters. Reply dump: ");
            console.log(reply.Data());

            throw "[ESP32Flasher.AttachSPI(...)] Attach SPI: FAILED";
        }

        console.log("Attach SPI: SUCCESS");
    }

    async SetSPIParameters(serial: Serial)
    {
        const packet = new ESP32SlipPacket(ESP32SlipPacket.OUTGOING,
            ESP32Flasher.SPI_SET_PARAMS);

        // ID
        packet.PushData(0, 4);

        // Total size (4MB)
        packet.PushData(0x400000, 4);

        // esp32s3 block Size
        packet.PushData(64 * 1024, 4);

        // esp32s3 sector size
        packet.PushData(4 * 1024, 4);

        // esp32s3 page size
        packet.PushData(256, 4);

        // Status mask
        packet.PushData(0xffff, 4);

        const reply = await this.WritePacketWaitForResponsePacket(serial,
            packet, ESP32Flasher.SPI_SET_PARAMS);

        if (reply.End() == 1)
        {
            console.log("[ESP32Flasher.SetSPIParamters(...)] Error. "
                + "Failed to set spi parameters. Reply dump: ");
            console.log(reply.Data());

            throw "[ESP32Flasher.SetSPIParamters(...)] Set SPI Parameters: failed";
        }

        console.log("Set SPI Parameters: SUCCESS");
    }

    async Flash(serial: Serial, net_bins: any[])
    {

        for (const bin of net_bins)
        {
            const size = bin['binary'].length;
            const num_blocks = Math.floor((size +
                ESP32Flasher.Block_Size - 1) / ESP32Flasher.Block_Size);

            await this.FlashBegin(serial, size, num_blocks, bin['offset']);
            await this.FlashData(serial, bin['name'], bin['binary'], size, num_blocks);
            await this.FlashMD5(serial, bin['md5'], bin['offset'], size);
        }

        await this.FlashEnd(serial);
    }

    async FlashBegin(serial: Serial, size: number, num_blocks: number,
        offset: number)
    {
        const packet = new ESP32SlipPacket(ESP32SlipPacket.OUTGOING,
            ESP32Flasher.FLASH_BEGIN);

        // Size to erase
        packet.PushData(size, 4);

        // Number of incoming packets (blocks)
        packet.PushData(num_blocks, 4);

        // How big each packet will be
        packet.PushData(ESP32Flasher.Block_Size, 4);

        // Where to begin writing for the incoming data
        packet.PushData(offset, 4);

        // Just some zeroes
        packet.PushData(0, 4);

        const reply = await this.WritePacketWaitForResponsePacket(serial,
            packet, ESP32Flasher.FLASH_BEGIN);

        // Check for error
        if (reply.End() == 1)
        {
            console.log("[ESP32Flasher.FlashBegin(...)] Error. Failed to " +
                "start flash. Packet dump: ");
            console.log(reply.Data());

            throw "[ESP32Flasher.FlashBegin(...)] Flash Begin: FAILED";
        }

        console.log("Flash Begin: SUCCESS");
    }

    async FlashData(serial: Serial, bin_name: string, data: number[],
        size: number, num_blocks: number)
    {
        let data_ptr = 0;
        let packet_idx = 0;

        console.log(`Flashing binary ${bin_name}, Blocks: ${num_blocks}, Size ${size}`);
        console.log(`Flashing: 00.00%`);

        let bin_packet;
        let data_bytes;
        while (data_ptr < size)
        {
            bin_packet = new ESP32SlipPacket(ESP32SlipPacket.OUTGOING,
                ESP32Flasher.FLASH_DATA);

            // Push the data size aka block size
            bin_packet.PushData(ESP32Flasher.Block_Size, 4);

            // Push the current packet number aka seq
            bin_packet.PushData(packet_idx, 4);

            // Push some zeroes (32 *2 bits of zeroes)
            bin_packet.PushData(0, 4);
            bin_packet.PushData(0, 4);

            // Get a data slice
            data_bytes = data.slice(data_ptr,
                data_ptr + ESP32Flasher.Block_Size);

            while (data_bytes.length < ESP32Flasher.Block_Size)
            {
                data_bytes.push(0xFF);
            }

            // Push it all into the packet
            bin_packet.PushDataArray(data_bytes, "big");

            const reply = await this.WritePacketWaitForResponsePacket(serial,
                bin_packet, ESP32Flasher.FLASH_DATA, true);

            // Check for error
            if (reply.End() == 1)
            {
                console.log("[ESP32Flasher.FlashWrite(...)] Error. Failed to " +
                    `write flash for ${data_ptr} for file ${bin_name}. Dump: `);
                console.log(reply.Data());

                throw "[ESP32Flasher.FlashWrite(...)] Flash Write: FAILED";
            }

            console.log(`Flashing: ${((data_ptr / size) * 100).toFixed(2)}%`);

            // Slide the file pointer idx over
            data_ptr += ESP32Flasher.Block_Size;

            // Increment the sequence number
            packet_idx++;
        }

        console.log("Flashing: 100.00%");
    }

    async FlashEnd(serial: Serial)
    {
        const packet = new ESP32SlipPacket(ESP32SlipPacket.OUTGOING,
            ESP32Flasher.FLASH_END);

        packet.PushData(0x1, 4);

        const reply = await this.WritePacketWaitForResponsePacket(serial,
            packet, ESP32Flasher.FLASH_END);

        if (reply.End() == 1)
        {
            console.log("[ESP32Flasher.FlashEnd(...)] Error. Failed to send " +
                "flash end command.");
            throw "[ESP32Flasher.FlashEnd(...)] Flash End: FAILED";
        }

        console.log("Flashing Complete");
    }

    async FlashMD5(serial: Serial, hash: string, address: number,
        size: number)
    {
        console.log("Verify: STARTED");
        const packet = new ESP32SlipPacket(ESP32SlipPacket.OUTGOING,
            ESP32Flasher.SPI_FLASH_MD5);

        packet.PushData(address, 4);
        packet.PushData(size, 4);
        packet.PushData(0, 4);
        packet.PushData(0, 4);

        const reply = await this.WritePacketWaitForResponsePacket(serial,
            packet, ESP32Flasher.SPI_FLASH_MD5);

        if (reply.End() == 1)
        {
            console.log("[ESP32Flasher.FlashMD5(...)] Error. Got an error " +
                " response");
            throw "[ESP32Flasher.FlashMD5(...)] Error. Failed to verify";
        }

        // Get the md5 from the response
        const esp_md5 = reply.GetBytes(8, 32);
        for (let i = 0; i < esp_md5.length && i < hash.length; ++i)
        {
            if (hash.charCodeAt(i) != esp_md5[i])
            {
                console.log("Verify: FAILED. MD5 hashes did not match");
                throw "[ESP32Flasher.FlashMD5(...)] Error. Failed to verify";
            }
            i++;
        }

        console.log("Verify: COMPLETE");
    }

    async WaitForResponsePacket(serial: Serial, packet_type: number = -1)
    {
        let packet;
        let packets: any = [];

        let bytes = [];
        let byte = 0;
        while (true)
        {
            // Keep reading bytes until a start byte is received
            while (byte != ESP32SlipPacket.END)
            {
                byte = await serial.ReadByte(2000);

                // No reply was received
                if (byte == NO_REPLY)
                {
                    if (packets.length > 0)
                    {
                        // Return the packts that were completed
                        return packets;
                    }
                    else
                    {
                        // Got no packets
                        return NO_REPLY;
                    }
                }
            }

            // Append the end byte (which is the start)
            bytes.push(byte);

            do
            {
                byte = await serial.ReadByte(2000);

                if (byte == NO_REPLY)
                {
                    if (packets.length > 0)
                    {
                        return packets;
                    }
                    else
                    {
                        return NO_REPLY;
                    }
                }
                bytes.push(byte);
            } while (byte != ESP32SlipPacket.END);

            packet = ESP32SlipPacket.FromBytes(bytes);

            if (packet.GetCommand() == packet_type)
            {
                break;
            }
            else if (packet_type == -1)
            {
                packets.push(packet);
            }
        }

        return packet;
    }

    async WritePacket(serial: Serial, packet: ESP32SlipPacket,
        checksum: boolean = false)
    {
        let data = packet.SlipEncode(checksum);
        serial.WriteBytes(new Uint8Array(data));
    }

    async WritePacketWaitForResponsePacket(serial: Serial,
        packet: ESP32SlipPacket, packet_type: number = -1,
        checksum: boolean = false, retry_num: number = 5)
    {
        while (retry_num > 0)
        {
            retry_num--;

            await this.WritePacket(serial, packet, checksum);

            let reply = await this.WaitForResponsePacket(serial, packet_type);

            if (reply == NO_REPLY)
            {
                continue;
            }

            return reply;
        }

        return NO_REPLY;
    }
};

const esp32_flasher = new ESP32Flasher();

export default esp32_flasher;