import { concat } from "./serial.js";

const BASE = 0x08000000;
const xor = data => data.reduce((sum, byte) => sum ^ byte, 0);

export class STM32 {
    constructor(port) { this.port = port; }

    async ack(timeout = 3000) {
        const byte = (await this.port.read(1, timeout))[0];
        if (byte !== 0x79) throw new Error(byte === 0x1f ? "STM32 rejected the command (NACK)." : `Unexpected STM32 reply: 0x${byte.toString(16)}.`);
    }

    async init() { await this.port.write([0x7f]); await this.ack(); }
    async command(code) { await this.port.write([code, code ^ 255]); await this.ack(); }
    async packet(data, timeout) { await this.port.write([...data, xor(data)]); await this.ack(timeout); }
    async address(address) {
        await this.packet([address >>> 24, (address >>> 16) & 255, (address >>> 8) & 255, address & 255]);
    }

    async identify(expected) {
        await this.command(0);
        const count = (await this.port.read(1))[0];
        const info = await this.port.read(count + 1);
        await this.ack();
        this.commands = info.slice(1);
        await this.command(2);
        const length = (await this.port.read(1))[0] + 1;
        const id = await this.port.read(length);
        await this.ack();
        const product = id.reduce((n, byte) => (n << 8) | byte, 0);
        if (product !== expected) throw new Error(`Wrong STM32 chip: expected 0x${expected.toString(16)}, received 0x${product.toString(16)}.`);
    }

    async erase(index) {
        if (this.commands.includes(0x44)) {
            await this.command(0x44);
            await this.packet([0, 0, index >>> 8, index & 255], 30000);
        } else if (this.commands.includes(0x43)) {
            await this.command(0x43);
            await this.packet([0, index], 30000);
        } else throw new Error("STM32 bootloader does not support erase.");
    }

    async write(address, data) {
        await this.command(0x31); await this.address(address);
        await this.packet([data.length - 1, ...data]);
    }

    async read(address, length) {
        await this.command(0x11); await this.address(address);
        await this.port.write([length - 1, (length - 1) ^ 255]);
        await this.ack();
        return this.port.read(length);
    }

    async go() { await this.command(0x21); await this.address(BASE); }

    async setMgmtVersion(version, reinitialize) {
        if (version !== 16 && version !== 17) throw new Error("MGMT version must be 16 or 17.");
        // Match ctl's DATA0 setter: preserve all 16 option bytes, including
        // read/write protection and boot configuration, and update nDATA0 too.
        const base = 0x1ffff800;
        const options = await this.read(base, 16);
        const complement = version ^ 255;
        if (options[4] === version && options[5] === complement) return;
        options[4] = version;
        options[5] = complement;
        await this.write(base, options);
        // Programming option bytes may reset MGMT. Establish a new bootloader
        // session before verification, as ctl does for the same operation.
        await reinitialize();
        const actual = await this.read(base, 16);
        if (!actual.every((byte, i) => byte === options[i])) throw new Error("MGMT version option-byte verification failed.");
    }

    async flash(binary, chip, progress) {
        const isMgmt = chip === "mgmt";
        await this.identify(isMgmt ? 0x448 : 0x413);
        // ROM writes require word-aligned lengths, including the last block.
        const data = concat(binary, new Uint8Array((4 - binary.length % 4) % 4).fill(255));
        const sizes = isMgmt ? Array(64).fill(2048) : [16384, 16384, 16384, 16384, 65536, ...Array(7).fill(131072)];
        if (!binary.length || data.length > sizes.reduce((sum, n) => sum + n, 0)) throw new Error(`${chip.toUpperCase()} firmware exceeds flash capacity or is empty.`);
        let capacity = 0, sectors = 0;
        while (capacity < data.length) capacity += sizes[sectors++];
        for (let i = 0; i < sectors; i++) {
            progress("Erasing", i, sectors); await this.erase(i);
        }
        for (let offset = 0; offset < data.length; offset += 256) {
            await this.write(BASE + offset, data.slice(offset, offset + 256));
            progress("Writing", Math.min(offset + 256, data.length), data.length);
        }
        for (let offset = 0; offset < data.length; offset += 256) {
            const expected = data.slice(offset, offset + 256);
            const actual = await this.read(BASE + offset, expected.length);
            if (!actual.every((b, i) => b === expected[i])) throw new Error(`Verification failed at 0x${(BASE + offset).toString(16)}.`);
            progress("Verifying", Math.min(offset + 256, data.length), data.length);
        }
    }
}
