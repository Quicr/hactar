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

    async FlashESP(serial: Serial, net_bins: number[][])
    {
        this.Sync(serial);

        this.AttachSPI(serial);
        this.SetSPIParameters(serial);

        this.Flash(serial, net_bins);
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

    }

    async SetSPIParameters(serial: Serial)
    {

    }

    async Flash(serial: Serial, net_bins: number[][])
    {

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

            let packet = ESP32SlipPacket.FromBytes(bytes);

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