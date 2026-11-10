import { concat, words } from "./serial.js";
import { md5 } from "./md5.js";

// ESP32-S3 ROM protocol: no stub upload or baud-rate changes through the tunnel.
export class ESP32 {
    constructor(port) { this.port = port; }

    async send(opcode, data, checksum = 0) {
        const header = new Uint8Array(8);
        const view = new DataView(header.buffer);
        header[1] = opcode;
        view.setUint16(2, data.length, true);
        view.setUint32(4, checksum, true);
        const escaped = [0xc0];
        for (const byte of concat(header, data)) {
            if (byte === 0xc0) escaped.push(0xdb, 0xdc);
            else if (byte === 0xdb) escaped.push(0xdb, 0xdd);
            else escaped.push(byte);
        }
        escaped.push(0xc0);
        await this.port.write(Uint8Array.from(escaped));
    }

    async response(opcode, timeout) {
        const deadline = Date.now() + timeout;
        const byte = async () => {
            if (Date.now() >= deadline) throw new Error("ESP32 bootloader response timed out.");
            return (await this.port.read(1, deadline - Date.now()))[0];
        };
        let packet = [], started = false, escaped = false;
        while (true) {
            let b = await byte();
            if (b === 0xc0) {
                if (started && packet.length >= 8 && packet[0] === 1 && packet[1] === opcode) {
                    const result = Uint8Array.from(packet);
                    const view = new DataView(result.buffer);
                    const size = view.getUint16(2, true);
                    if (result.length !== 8 + size || size < 4) throw new Error("Invalid ESP32 ROM response.");
                    const payload = result.slice(8);
                    if (payload[payload.length - 4] !== 0) throw new Error(`ESP32 command 0x${opcode.toString(16)} failed (code ${payload[payload.length - 3]}).`);
                    return { value: view.getUint32(4, true), data: payload.slice(0, -4) };
                }
                started = true; packet = []; escaped = false;
            } else if (started) {
                if (escaped) {
                    if (b !== 0xdc && b !== 0xdd) throw new Error("Invalid ESP32 SLIP escape.");
                    b = b === 0xdc ? 0xc0 : 0xdb; escaped = false;
                } else if (b === 0xdb) { escaped = true; continue; }
                packet.push(b);
                if (packet.length > 65536) throw new Error("ESP32 response exceeds packet limit.");
            }
        }
    }

    async command(opcode, data, checksum = 0, timeout = 3000) {
        await this.send(opcode, data, checksum);
        return this.response(opcode, timeout);
    }

    async init() {
        const sync = concat(Uint8Array.of(7, 7, 18, 32), new Uint8Array(32).fill(0x55));
        // SYNC returns eight replies; subsequent commands skip leftover replies.
        await this.command(8, sync, 0, 5000);
        const chip = await this.command(0x0a, words(0x40001000));
        if (chip.value !== 0x00000009) throw new Error("NET is not an ESP32-S3.");
        await this.command(0x0d, words(0, 0)); // SPI_ATTACH (default flash pins)
        // Match the existing ESP-IDF image's 8 MB flash geometry.
        await this.command(0x0b, words(0, 8 * 1024 * 1024, 65536, 4096, 256, 0xffff));
    }

    async flash(images, progress) {
        await this.init();
        const total = images.reduce((sum, image) => sum + image.data.length, 0);
        let written = 0;
        for (const { address, data, name } of images) {
            const blocks = Math.ceil(data.length / 1024);
            progress(`Erasing ${name}`, written, total);
            // The S3 ROM expects the additional encrypted-write flag (zero).
            await this.command(2, words(Math.ceil(data.length / 4096) * 4096, blocks, 1024, address, 0), 0, 120000);
            for (let sequence = 0; sequence < blocks; sequence++) {
                const chunk = new Uint8Array(1024).fill(255);
                const part = data.slice(sequence * 1024, (sequence + 1) * 1024);
                chunk.set(part);
                const checksum = chunk.reduce((value, byte) => value ^ byte, 0xef);
                await this.command(3, concat(words(1024, sequence, 0, 0), chunk), checksum, 10000);
                written += part.length;
                progress(`Writing ${name}`, written, total);
            }
            progress(`Verifying ${name}`, written, total);
            const checksum = await this.command(0x13, words(address, data.length, 0, 0), 0, 30000);
            if (new TextDecoder().decode(checksum.data).toLowerCase() !== md5(data)) throw new Error(`NET verification failed for ${name}.`);
        }
        await this.command(4, words(1)); // Stay in ROM until MGMT resets NET.
    }
}
