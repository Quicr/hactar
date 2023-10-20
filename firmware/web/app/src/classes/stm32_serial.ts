import Serial from "./serial"
import logger from "./logger"
import { NO_REPLY, ACK, NACK } from "./uart_utils";

export async function WriteByteWaitForACK(serial: Serial, byte: number, retry: number = 1,
    compliment: boolean = true, timeout = 2000) {

    let data: number[] = [byte];
    if (compliment) {
        data.push(byte ^ 0xFF)
    }

    let num = retry;
    while (num--) {
        await serial.WriteBytes(new Uint8Array(data));

        let reply = await serial.ReadByte(timeout);

        if (reply == NO_REPLY) {
            logger.Warning(`Received no reply for byte: ${byte}`)
            continue;
        }

        return reply;
    }

    return NO_REPLY;
}

export async function WriteBytesWaitForACK(serial: Serial, bytes: Uint8Array,
    timeout: number = 2000, retry: number = 1) {
    let num = retry;
    while (num--) {
        await serial.WriteBytes(bytes);

        let reply = await serial.ReadByte(timeout);

        if (reply == NO_REPLY)
            continue;

        return reply;
    }

    return NO_REPLY;
}

export default {
    WriteByteWaitForACK,
    WriteBytesWaitForACK
};
