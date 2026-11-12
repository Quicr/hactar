export const sleep = ms => new Promise(resolve => setTimeout(resolve, ms));

export class SerialTimeoutError extends Error {}

// A persistent reader ensures timed-out requests cannot steal later replies.
export class SerialConnection {
    constructor(port) { this.port = port; }

    async open(baudRate) {
        await this.port.open({ baudRate, dataBits: 8, stopBits: 1, parity: "even", flowControl: "none", bufferSize: 16384 });
        this.buffer = new Uint8Array();
        this.receivedBytes = 0;
        this.error = null;
        this.reader = this.port.readable.getReader();
        this.writer = this.port.writable.getWriter();
        this.readTask = this.pump();
    }

    async pump() {
        try {
            while (true) {
                const { value, done } = await this.reader.read();
                if (done) break;
                this.receivedBytes += value.length;
                this.buffer = concat(this.buffer, value);
                if (this.buffer.length > 1024 * 1024) throw new Error("Serial receive buffer overflow.");
                this.wake?.();
            }
            this.error = new Error("Serial connection closed.");
        } catch (error) { this.error = error; }
        finally { this.wake?.(); }
    }

    async read(length, timeout = 3000) {
        const deadline = Date.now() + timeout;
        while (this.buffer.length < length) {
            if (this.error) throw this.error;
            const remaining = deadline - Date.now();
            if (remaining <= 0) throw new SerialTimeoutError("Timed out waiting for the device.");
            await new Promise(resolve => {
                const finish = () => { clearTimeout(timer); this.wake = null; resolve(); };
                const timer = setTimeout(finish, remaining);
                this.wake = finish;
            });
        }
        const result = this.buffer.slice(0, length);
        this.buffer = this.buffer.slice(length);
        return result;
    }

    async write(data) {
        if (this.error) throw this.error;
        await this.writer.write(Uint8Array.from(data));
    }

    async drain() { await sleep(100); this.buffer = new Uint8Array(); }

    async close() {
        if (!this.reader) return;
        // cancel() rejects when USB disconnection already errored the stream.
        try { await this.reader.cancel().catch(() => {}); await this.readTask; }
        finally {
            this.reader.releaseLock();
            this.writer.releaseLock();
            this.reader = null;
            await this.port.close();
        }
    }
}

export function concat(...arrays) {
    const result = new Uint8Array(arrays.reduce((sum, a) => sum + a.length, 0));
    let offset = 0;
    for (const array of arrays) { result.set(array, offset); offset += array.length; }
    return result;
}

export function words(...values) {
    const result = new Uint8Array(values.length * 4);
    const view = new DataView(result.buffer);
    values.forEach((value, i) => view.setUint32(i * 4, value, true));
    return result;
}

export function frame(type, value = []) {
    const result = new Uint8Array(10 + value.length);
    result.set([0x4c, 0x49, 0x4e, 0x4b]);
    const view = new DataView(result.buffer);
    view.setUint16(4, type, true);
    view.setUint32(6, value.length, true);
    result.set(value, 10);
    return result;
}

export class Management {
    constructor(serial, log = () => {}) { this.serial = serial; this.log = log; }

    async receive(timeout = 3000) {
        const deadline = Date.now() + timeout;
        const read = n => this.serial.read(n, Math.max(1, deadline - Date.now()));
        const sync = [0x4c, 0x49, 0x4e, 0x4b];
        let matched = 0;
        while (matched < 4) {
            if (Date.now() >= deadline) throw new SerialTimeoutError("Timed out waiting for a LINK frame.");
            const byte = (await read(1))[0];
            matched = byte === sync[matched] ? matched + 1 : (byte === sync[0] ? 1 : 0);
        }
        const header = await read(6);
        const view = new DataView(header.buffer);
        const length = view.getUint32(2, true);
        if (length > 640) throw new Error("Invalid LINK frame length.");
        return { type: view.getUint16(0, true), value: await read(length) };
    }

    async request(type, value, response = 0x13, timeout = 3000) {
        await this.serial.write(frame(type, value));
        const deadline = Date.now() + timeout;
        while (Date.now() < deadline) {
            const reply = await this.receive(deadline - Date.now());
            if (reply.type === response) return reply.value;
        }
        throw new Error("MGMT did not acknowledge the command.");
    }

    async ready() {
        const receivedBefore = this.serial.receivedBytes ?? 0;
        let lastError;
        for (let attempt = 0; attempt < 10; attempt++) {
            await this.serial.drain();
            const challenge = crypto.getRandomValues(new Uint8Array(4));
            try {
                const reply = await this.request(3, challenge, 0x14, 500);
                if (reply.length === 4 && reply.every((b, i) => b === (challenge[i] ^ [76, 73, 78, 75][i]))) return;
                lastError = new Error("MGMT Hello reply did not match the challenge.");
            } catch (error) {
                if (this.serial.error) throw error;
                lastError = error;
            }
            this.log(`MGMT handshake ${attempt + 1}/10 failed: ${lastError.message}`);
        }
        const received = (this.serial.receivedBytes ?? 0) - receivedBefore;
        this.log(`Received ${received} serial bytes during the MGMT handshake.`);
        throw new Error(`MGMT is not responding to LINK commands at 1000000 baud, 8E1. Check that MGMT BOOT0 is low and reset is released, and that the selected port belongs to the device. Last handshake error: ${lastError?.message}`);
    }

    async control(type, value, description) {
        this.log(`MGMT: ${description}…`);
        try { return await this.request(type, value); }
        catch (error) { throw new Error(`MGMT ${description}: ${error.message}`, { cause: error }); }
    }

    pin(pin, value) {
        const name = ["UI BOOT0", "UI BOOT1", "UI reset", "NET BOOT", "NET reset"][pin] ?? `pin ${pin}`;
        return this.control(4, [pin, value], `setting ${name} ${value ? "HIGH" : "LOW"}`);
    }
    uiBaud(baud) { return this.control(5, words(baud), `setting UI UART to ${baud} baud`); }

    async resetUI(bootloader = false) {
        await this.pin(0, bootloader ? 1 : 0);
        await this.pin(1, bootloader ? 0 : 1);
        await this.pin(2, 0);
        await sleep(50);
        await this.pin(2, 1);
    }

    async resetNET(bootloader = false) {
        if (bootloader) {
            await this.pin(4, 0); await sleep(10); await this.pin(4, 1);
        }
        await this.pin(3, bootloader ? 0 : 1);
        await this.pin(4, 0);
        await sleep(10);
        await this.pin(4, 1);
    }

    tunnel(chip) {
        const outgoing = chip === "ui" ? 1 : 2;
        const incoming = chip === "ui" ? 0x11 : 0x12;
        let buffer = new Uint8Array();
        return {
            write: async data => {
                for (let offset = 0; offset < data.length; offset += 640) {
                    await this.serial.write(frame(outgoing, data.slice(offset, offset + 640)));
                }
            },
            read: async (length, timeout = 3000) => {
                const deadline = Date.now() + timeout;
                while (buffer.length < length) {
                    if (Date.now() >= deadline) throw new SerialTimeoutError(`Timed out waiting for ${chip.toUpperCase()}.`);
                    const reply = await this.receive(deadline - Date.now());
                    if (reply.type === incoming) buffer = concat(buffer, reply.value);
                }
                const result = buffer.slice(0, length);
                buffer = buffer.slice(length);
                return result;
            },
        };
    }
}
