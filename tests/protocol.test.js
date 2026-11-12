import test from "node:test";
import assert from "node:assert/strict";
import { createHash, randomBytes } from "node:crypto";
import { SerialConnection, Management, SerialTimeoutError, concat, frame, words } from "../scripts/serial.js";
import { STM32 } from "../scripts/stm32.js";
import { ESP32 } from "../scripts/esp32.js";
import { md5 } from "../scripts/md5.js";
import { HactarFlasher } from "../scripts/flasher.js";

class Queue {
    buffer = new Uint8Array();
    push(data) { this.buffer = concat(this.buffer, Uint8Array.from(data)); }
    async read(length) {
        assert.ok(this.buffer.length >= length, `Missing mock response: need ${length}, have ${this.buffer.length}`);
        const result = this.buffer.slice(0, length);
        this.buffer = this.buffer.slice(length);
        return result;
    }
}

test("MD5 agrees with independent implementation at padding/block boundaries", () => {
    for (const length of [0, 1, 55, 56, 63, 64, 65, 127, 128, 1024, 65537]) {
        const data = randomBytes(length);
        assert.equal(md5(data), createHash("md5").update(data).digest("hex"));
    }
});

test("LINK tunnels preserve fragmented streams, skip other chips, and split writes at 640 bytes", async () => {
    const queue = new Queue();
    const writes = [];
    queue.write = async data => writes.push(data);
    const mgmt = new Management(queue);
    const tunnel = mgmt.tunnel("net");
    queue.push([0xff, 0x4c, 0x4c]); // Garbage, including an overlapping sync prefix.
    queue.push(frame(0x13));
    queue.push(frame(0x11, [42]));
    queue.push(frame(0x12, [1]));
    queue.push(frame(0x12, [2, 3, 4]));
    assert.deepEqual(await tunnel.read(3), Uint8Array.of(1, 2, 3));
    assert.deepEqual(await tunnel.read(1), Uint8Array.of(4));
    const data = randomBytes(1500);
    await tunnel.write(data);
    assert.deepEqual(writes.map(p => p.length), [650, 650, 230]);
    assert.deepEqual(concat(...writes.map(p => p.slice(10))), new Uint8Array(data));
    assert.deepEqual([...writes[0].slice(0, 10)], [76, 73, 78, 75, 2, 0, 128, 2, 0, 0]);
});

test("Web Serial timeout does not consume a later reply; streams unlock and reopen", async () => {
    let controller;
    const port = {
        async open(options) {
            assert.equal(options.parity, "even");
            this.readable = new ReadableStream({ start(c) { controller = c; } });
            this.writable = new WritableStream();
        },
        async close() { assert.equal(this.readable.locked, false); assert.equal(this.writable.locked, false); },
    };
    const serial = new SerialConnection(port);
    await serial.open(115200);
    await assert.rejects(serial.read(1, 5), /Timed out/);
    controller.enqueue(Uint8Array.of(0x79));
    assert.deepEqual(await serial.read(1), Uint8Array.of(0x79));
    await serial.close();
    await serial.open(1000000);
    const read = serial.read(1);
    controller.error(new Error("USB disconnected"));
    await assert.rejects(read, /USB disconnected/);
    await serial.close();
});

// Stateful STM ROM emulator checks command complements, checksums, alignment,
// erase selection and readback against flash memory instead of canned replies.
class STMROM extends Queue {
    constructor(id, extended = true, corrupt = false) {
        super(); this.id = id; this.extended = extended; this.corrupt = corrupt;
        this.memory = new Uint8Array(1024 * 1024).fill(255);
        this.options = Uint8Array.from([0xaa, 0x55, 0xff, 0, 0xff, 0, 0x34, 0xcb, 0xff, 0, 0xff, 0, 0xff, 0, 0xff, 0]);
        this.optionWrites = 0;
        this.erased = []; this.state = "init";
    }
    async write(input) {
        const bytes = Uint8Array.from(input);
        const xor = bytes.reduce((n, b) => n ^ b, 0);
        if (this.state === "init") {
            assert.deepEqual([...bytes], [0x7f]); this.state = "command";
        } else if (this.state === "command") {
            assert.equal(bytes.length, 2); assert.equal(xor, 255);
            this.command = bytes[0];
            if (this.command === 0) {
                const commands = [0, 2, 0x11, 0x21, 0x31, this.extended ? 0x44 : 0x43];
                this.push([0x79, commands.length, 0x31, ...commands, 0x79]); return;
            }
            if (this.command === 2) { this.push([0x79, 1, this.id >>> 8, this.id & 255, 0x79]); return; }
            this.state = [0x43, 0x44].includes(this.command) ? "erase" : "address";
        } else if (this.state === "erase") {
            assert.equal(xor, 0);
            this.erased.push(this.command === 0x44 ? (bytes[2] << 8) | bytes[3] : bytes[1]);
            this.state = "command";
        } else if (this.state === "address") {
            assert.equal(bytes.length, 5); assert.equal(xor, 0);
            const address = new DataView(bytes.buffer).getUint32(0);
            this.optionAccess = address === 0x1ffff800;
            this.address = this.optionAccess ? 0 : address - 0x08000000;
            this.state = this.command === 0x21 ? "command" : "data";
        } else if (this.state === "data") {
            const length = bytes[0] + 1;
            this.state = "command";
            if (this.command === 0x31) {
                assert.equal(xor, 0); assert.equal(length % 4, 0); assert.equal(bytes.length, length + 2);
                if (this.optionAccess) {
                    assert.equal(length, 16);
                    this.options.set(bytes.slice(1, -1));
                    this.optionWrites++;
                    this.state = "init"; // Option-byte programming resets the ROM session.
                } else this.memory.set(bytes.slice(1, -1), this.address);
            } else {
                assert.equal(xor, 255);
                const data = (this.optionAccess ? this.options : this.memory).slice(this.address, this.address + length);
                if (this.corrupt) data[0] ^= 1;
                if (this.optionAccess && this.optionWrites && this.corruptOptions) data[4] ^= 1;
                this.push([0x79, ...data]); return;
            }
        }
        this.push([0x79]);
    }
}

for (const [chip, id, size, extended, sectors] of [
    ["mgmt", 0x448, 2051, true, [0, 1]],
    ["mgmt", 0x448, 513, false, [0]],
    ["ui", 0x413, 131075, true, [0, 1, 2, 3, 4, 5]],
]) {
    test(`${chip} ROM erases correct regions, pads writes and verifies ${size} bytes`, async () => {
        const rom = new STMROM(id, extended);
        const loader = new STM32(rom);
        const data = randomBytes(size);
        await loader.init();
        await loader.flash(data, chip, () => {});
        assert.deepEqual(rom.erased, sectors);
        assert.deepEqual(rom.memory.slice(0, size), new Uint8Array(data));
        assert.equal(rom.memory[size], 255);
        await loader.go();
    });
}

test("STM32 refuses wrong chip and detects corrupted readback", async () => {
    const wrong = new STM32(new STMROM(0x413));
    await wrong.init();
    await assert.rejects(wrong.flash(Uint8Array.of(1), "mgmt", () => {}), /Wrong STM32/);
    const corrupt = new STM32(new STMROM(0x448, true, true));
    await corrupt.init();
    await assert.rejects(corrupt.flash(Uint8Array.of(1), "mgmt", () => {}), /Verification failed/);
});

for (const version of [16, 17]) {
    test(`MGMT DATA0=${version} preserves all other option bytes and verifies after reinitialization`, async () => {
        const rom = new STMROM(0x448), loader = new STM32(rom);
        const expected = rom.options.slice();
        expected[4] = version; expected[5] = version ^ 255;
        await loader.init();
        await loader.setMgmtVersion(version, () => loader.init());
        assert.deepEqual(rom.options, expected);
        assert.equal(rom.optionWrites, 1);
        // Selecting an already matching version does not reprogram option bytes.
        await loader.setMgmtVersion(version, () => assert.fail("Unnecessary ROM reset"));
        assert.equal(rom.optionWrites, 1);
    });
}

test("MGMT option-byte readback corruption fails verification", async () => {
    const rom = new STMROM(0x448), loader = new STM32(rom);
    rom.corruptOptions = true;
    await loader.init();
    await assert.rejects(loader.setMgmtVersion(17, () => loader.init()), /option-byte verification failed/);
});

function slip(data) {
    const result = [0xc0];
    for (const b of data) result.push(...(b === 0xc0 ? [0xdb, 0xdc] : b === 0xdb ? [0xdb, 0xdd] : [b]));
    return Uint8Array.from([...result, 0xc0]);
}

class ESPROM extends Queue {
    memory = new Uint8Array(8 * 1024 * 1024).fill(255);
    begins = [];
    commands = [];
    async write(input) {
        const decoded = [];
        for (let i = 1; i < input.length - 1; i++) {
            decoded.push(input[i] === 0xdb ? (input[++i] === 0xdc ? 0xc0 : 0xdb) : input[i]);
        }
        const packet = Uint8Array.from(decoded);
        const view = new DataView(packet.buffer);
        const opcode = packet[1], data = packet.slice(8);
        const params = new DataView(data.buffer);
        assert.equal(view.getUint16(2, true), data.length);
        this.commands.push(opcode);
        let value = 0, response = new Uint8Array();
        if (opcode === 8) {
            assert.deepEqual([...data.slice(0, 4)], [7, 7, 18, 32]);
            for (let i = 0; i < 7; i++) this.reply(opcode, 0x1234);
        } else if (opcode === 0x0a) {
            assert.equal(params.getUint32(0, true), 0x40001000); value = 9;
        } else if (opcode === 2) {
            assert.equal(data.length, 20);
            assert.equal(params.getUint32(16, true), 0);
            this.address = params.getUint32(12, true);
            this.begins.push(this.address);
            this.memory.fill(255, this.address, this.address + params.getUint32(0, true));
        } else if (opcode === 3) {
            assert.equal(params.getUint32(0, true), 1024);
            assert.equal(view.getUint32(4, true), data.slice(16).reduce((a, b) => a ^ b, 0xef));
            this.memory.set(data.slice(16), this.address + params.getUint32(4, true) * 1024);
        } else if (opcode === 0x13) {
            const start = params.getUint32(0, true), size = params.getUint32(4, true);
            response = new TextEncoder().encode(this.corrupt ? "0".repeat(32) : createHash("md5").update(this.memory.slice(start, start + size)).digest("hex"));
        } else if (opcode === 4) {
            // Reproduce the EV16 NET ROM's rejection of the final FLASH_END.
            this.reply(opcode, 0, new Uint8Array(), Uint8Array.of(1, 6, 0, 0));
            return;
        } else assert.ok([0x0d, 0x0b].includes(opcode));
        this.reply(opcode, value, response);
    }
    reply(opcode, value, data = new Uint8Array(), status = new Uint8Array(4)) {
        const header = new Uint8Array(8);
        header[0] = 1; header[1] = opcode;
        const view = new DataView(header.buffer);
        view.setUint16(2, data.length + 4, true); view.setUint32(4, value, true);
        this.push(slip(concat(header, data, status)));
    }
}

test("ESP32-S3 ROM flashes and independently verifies all images through fragmented LINK TLVs", async () => {
    const rom = new ESPROM(), serial = new Queue();
    let pending = new Uint8Array();
    serial.write = async packet => {
        assert.ok(packet.length <= 650);
        pending = concat(pending, packet.slice(10));
        if (pending[pending.length - 1] !== 0xc0) return;
        await rom.write(pending); pending = new Uint8Array();
        // Split SLIP escapes, headers, and payloads across tunnel frame boundaries.
        while (rom.buffer.length) {
            serial.push(frame(0x11, [0x79]));
            serial.push(frame(0x12, await rom.read(Math.min(7, rom.buffer.length))));
        }
    };
    const images = [
        { address: 0, name: "bootloader.bin", data: randomBytes(2001) },
        { address: 0x8000, name: "partition-table.bin", data: randomBytes(1024) },
        { address: 0x10000, name: "net.bin", data: randomBytes(4099) },
    ];
    await new ESP32(new Management(serial).tunnel("net")).flash(images, () => {});
    assert.deepEqual(rom.begins, [0, 0x8000, 0x10000]);
    for (const { address, data } of images) assert.deepEqual(rom.memory.slice(address, address + data.length), new Uint8Array(data));
    assert.equal(rom.commands.at(-1), 0x13);
    assert.equal(rom.commands.filter(opcode => opcode === 0x13).length, images.length);
    assert.equal(rom.commands.includes(4), false);
});

test("ESP32 verification failure is reported", async () => {
    const rom = new ESPROM(); rom.corrupt = true;
    await assert.rejects(new ESP32(rom).flash([{ address: 0x10000, name: "app", data: randomBytes(50) }], () => {}), /verification failed/);
});

test("NET synchronization retries missed startup requests through the LINK tunnel", async () => {
    const rom = new ESPROM(), serial = new Queue(), logs = [];
    let attempts = 0;
    serial.write = async packet => {
        assert.equal(new DataView(packet.buffer).getUint16(4, true), 2);
        if (++attempts < 3) return; // ROM is not ready for auto-baud yet.
        await rom.write(packet.slice(10));
        serial.push(frame(0x12, await rom.read(rom.buffer.length)));
    };
    const read = serial.read.bind(serial);
    serial.read = async length => {
        if (!serial.buffer.length) throw new SerialTimeoutError("Timed out waiting for the device.");
        return read(length);
    };
    await new ESP32(new Management(serial).tunnel("net"), message => logs.push(message)).sync();
    assert.equal(attempts, 3);
    assert.equal(logs.filter(message => message.includes("timed out")).length, 2);
    assert.match(logs.at(-1), /synchronized on attempt 3\/10/);
});

test("NET synchronization stops after ten timeouts and does not retry USB failures", async () => {
    let writes = 0;
    const port = {
        async write() { writes++; },
        async read() { throw new SerialTimeoutError("Timed out waiting for the device."); },
    };
    await assert.rejects(new ESP32(port).sync(), /NET ROM did not respond to SYNC after 10 attempts/);
    assert.equal(writes, 10);
    writes = 0;
    port.read = async () => { throw new Error("USB disconnected"); };
    await assert.rejects(new ESP32(port).sync(), /USB disconnected/);
    assert.equal(writes, 1);
});

test("NET write timeout identifies the image and block without retrying FLASH_DATA", async () => {
    const rom = new ESPROM();
    const write = rom.write.bind(rom), read = rom.read.bind(rom);
    let flashWrites = 0;
    rom.write = async packet => {
        if (packet[2] === 3) { flashWrites++; return; }
        await write(packet);
    };
    rom.read = async length => {
        if (!rom.buffer.length) throw new SerialTimeoutError("Timed out waiting for the device.");
        return read(length);
    };
    await assert.rejects(new ESP32(rom).flash([
        { address: 0x10000, name: "net.bin", data: randomBytes(2048) },
    ], () => {}), /net.bin at 0x10000 \(block 1\/2\): NET FLASH_DATA \(0x3\): Timed out/);
    assert.equal(flashWrites, 1);
});

test("missing firmware prevents opening or erasing a device", async () => {
    const original = globalThis.fetch;
    globalThis.fetch = async () => new Response("Missing", { status: 404 });
    try {
        await assert.rejects(new HactarFlasher({ log() {}, progress() {} }).flash(["mgmt"], {
            open() { assert.fail("Serial port must not be opened"); },
        }), /HTTP 404/);
    } finally { globalThis.fetch = original; }
});

class DevicePort {
    pins = [0, 1, 1, 1, 1];
    opened = [];
    mgmt = new STMROM(0x448);
    ui = new STMROM(0x413);
    net = new ESPROM();
    netPending = new Uint8Array();
    async open(options) {
        this.baud = options.baudRate; this.opened.push(this.baud);
        // Model a USB serial driver asserting both lines on every open,
        // including the baud-rate reopen after flashing MGMT.
        this.signals = { dataTerminalReady: true, requestToSend: true };
        this.readable = new ReadableStream({ start: c => { this.controller = c; } });
        this.writable = new WritableStream({ write: data => this.write(data) });
    }
    async close() { assert.equal(this.readable.locked, false); assert.equal(this.writable.locked, false); }
    async setSignals(signals) { this.signals = { ...this.signals, ...signals }; }
    emit(data) { this.controller.enqueue(Uint8Array.from(data)); }
    async write(data) {
        if (this.baud === 115200) {
            await this.mgmt.write(data);
            this.emit(await this.mgmt.read(this.mgmt.buffer.length)); return;
        }
        assert.deepEqual([...data.slice(0, 4)], [76, 73, 78, 75]);
        const view = new DataView(data.buffer, data.byteOffset, data.byteLength);
        const type = view.getUint16(4, true), payload = data.slice(10);
        assert.equal(view.getUint32(6, true), payload.length);
        if (type === 3) {
            assert.equal(this.signals.requestToSend, false, "Release MGMT BOOT0 after opening the port");
            assert.equal(this.signals.dataTerminalReady, false, "Release MGMT reset after opening the port");
            this.emit(frame(0x14, payload.map((b, i) => b ^ [76, 73, 78, 75][i])));
        }
        else if (type === 4) { this.pins[payload[0]] = payload[1]; this.emit(frame(0x13)); }
        else if (type === 5) { this.uiBaud = new DataView(payload.buffer).getUint32(0, true); this.emit(frame(0x13)); }
        else if (type === 1) {
            assert.equal(this.uiBaud, 115200);
            assert.equal(this.pins[4], 0); // NET must be held in reset.
            assert.deepEqual(this.pins.slice(0, 3), [1, 0, 1]);
            await this.ui.write(payload);
            this.emit(frame(0x11, await this.ui.read(this.ui.buffer.length)));
        } else if (type === 2) {
            assert.equal(this.pins[2], 0); // UI must be held in reset.
            assert.deepEqual(this.pins.slice(3), [0, 1]);
            this.netPending = concat(this.netPending, payload);
            if (this.netPending.at(-1) !== 0xc0) return;
            await this.net.write(this.netPending); this.netPending = new Uint8Array();
            while (this.net.buffer.length) this.emit(frame(0x12, await this.net.read(Math.min(640, this.net.buffer.length))));
        } else assert.fail(`Unexpected MGMT command ${type}`);
    }
}

test("complete default update sequences MGMT → UI → NET and restores both chips", async () => {
    const original = globalThis.fetch;
    const images = new Map([
        ["mgmt/mgmt.bin", randomBytes(259)], ["ui/ui.bin", randomBytes(513)],
        ["net/bootloader.bin", randomBytes(123)], ["net/partition-table.bin", randomBytes(1024)],
        ["net/net.bin", randomBytes(1025)],
    ]);
    globalThis.fetch = async url => new Response(images.get(url.pathname.split("/build/")[1]));
    const port = new DevicePort(), logs = [];
    try {
        await new HactarFlasher({ log: m => logs.push(m), progress() {} }).flash(["mgmt", "ui", "net"], port);
        assert.deepEqual(port.opened, [115200, 1000000]);
        assert.deepEqual(port.pins, [0, 1, 1, 1, 1]);
        assert.equal(port.uiBaud, 1000000);
        assert.equal(port.signals.requestToSend, false);
        assert.equal(port.signals.dataTerminalReady, false);
        assert.deepEqual(logs.filter(m => m.includes("flashed and verified")), ["MGMT flashed and verified.", "UI flashed and verified.", "NET flashed and verified."]);
        assert.equal(port.readable.locked, false);
        assert.equal(port.mgmt.options[4], 17);
        assert.equal(port.mgmt.options[5], 17 ^ 255);
    } finally { globalThis.fetch = original; }
});

test("failed UI verification still restores baud, resets UI/NET and closes the port", async () => {
    const original = globalThis.fetch;
    globalThis.fetch = async () => new Response(randomBytes(17));
    const port = new DevicePort(); port.ui.corrupt = true;
    try {
        await assert.rejects(new HactarFlasher({ log() {}, progress() {} }).flash(["ui"], port), /Verification failed/);
        assert.deepEqual(port.pins, [0, 1, 1, 1, 1]);
        assert.equal(port.uiBaud, 1000000);
        assert.equal(port.readable.locked, false);
    } finally { globalThis.fetch = original; }
});

test("EV16 MGMT and NET update completes with verified images and a hardware reset", async () => {
    const original = globalThis.fetch;
    globalThis.fetch = async () => new Response(randomBytes(123));
    const port = new DevicePort(), logs = [];
    try {
        await new HactarFlasher({ log: message => logs.push(message), progress() {} }).flash(["mgmt", "net"], port, false, 16);
        assert.equal(port.mgmt.options[4], 16);
        assert.equal(port.net.commands.filter(opcode => opcode === 0x13).length, 3);
        assert.equal(port.net.commands.includes(4), false);
        assert.deepEqual(port.pins, [0, 1, 1, 1, 1]);
        assert.equal(port.readable.locked, false);
        assert.equal(logs.at(-1), "NET flashed and verified.");
    } finally { globalThis.fetch = original; }
});

test("normal-mode initialization releases BOOT0 before reset", async () => {
    const calls = [];
    await new HactarFlasher({ log() {}, progress() {} }).initMGMT({
        port: { async setSignals(signals) { calls.push(signals); } },
    });
    assert.deepEqual(calls, [{ requestToSend: false }, { dataTerminalReady: false }]);
});

class NoSignalPort extends DevicePort {
    signalCalls = 0;
    async open(options) {
        await super.open(options);
        // This bridge carries serial data, but has no usable modem controls.
        this.signals = { dataTerminalReady: false, requestToSend: false };
    }
    async setSignals() {
        this.signalCalls++;
        throw new DOMException("Failed to set control signals.", "NetworkError");
    }
}

for (const chips of [["ui", "net"], ["mgmt", "ui", "net"]]) {
    test(`${chips.join(" → ")} update works when the USB bridge rejects setSignals`, async () => {
        const original = globalThis.fetch;
        globalThis.fetch = async () => new Response(randomBytes(123));
        const port = new NoSignalPort(), logs = [];
        try {
            await new HactarFlasher({ log: message => logs.push(message), progress() {} }).flash(chips, port);
            assert.equal(port.signalCalls, 1, "Do not keep sending rejected controls, including after a baud-rate reopen");
            assert.equal(logs.filter(message => message.includes("USB port rejected")).length, 1);
            assert.deepEqual(logs.filter(message => message.includes("flashed and verified")), chips.map(chip => `${chip.toUpperCase()} flashed and verified.`));
            assert.deepEqual(port.pins, [0, 1, 1, 1, 1]);
            assert.equal(port.readable.locked, false);
        } finally { globalThis.fetch = original; }
    });
}

test("rejected automatic reset gives manual MGMT bootloader instructions if ROM is not active", async () => {
    const original = globalThis.fetch;
    globalThis.fetch = async () => new Response(randomBytes(123));
    const port = new NoSignalPort();
    // A running application does not acknowledge STM32 ROM auto-baud.
    port.write = async () => {};
    try {
        await assert.rejects(new HactarFlasher({ log() {}, progress() {} }).flash(["mgmt"], port), /USB port rejected automatic reset control.*manually reset MGMT.*manual bootloader mode/);
        assert.equal(port.signalCalls, 1);
        assert.equal(port.readable.locked, false);
    } finally { globalThis.fetch = original; }
});

test("invalid serial-port state is not treated as unsupported control signals", async () => {
    const failure = new DOMException("The port is closed.", "InvalidStateError");
    const loader = new HactarFlasher({ log() {}, progress() {} });
    await assert.rejects(loader.initMGMT({
        port: { async setSignals() { throw failure; } },
    }), error => error === failure);
});

test("MGMT control timeout identifies the exact pin and requested level", async () => {
    const logs = [];
    const mgmt = new Management({
        async write() {},
        async read() { throw new SerialTimeoutError("Timed out waiting for the device."); },
    }, message => logs.push(message));
    await assert.rejects(mgmt.pin(4, 0), /MGMT setting NET reset LOW: Timed out waiting for the device/);
    await assert.rejects(mgmt.uiBaud(115200), /MGMT setting UI UART to 115200 baud: Timed out/);
    assert.match(logs[0], /NET reset LOW/);
});

test("cleanup tries all resets and preserves the original NET failure", async () => {
    const logs = [], calls = [];
    const loader = new HactarFlasher({ log: message => logs.push(message), progress() {} });
    const failure = new Error("NET FLASH_DATA (0x3): Timed out waiting for the device.");
    const steps = [
        async () => { calls.push("net"); throw new Error("NET reset timeout"); },
        async () => { calls.push("ui"); throw new Error("UI reset timeout"); },
    ];
    await assert.rejects((async () => {
        try { throw failure; }
        finally { await loader.restore(steps, failure); }
    })(), error => error === failure);
    assert.deepEqual(calls, ["net", "ui"]);
    assert.equal(logs.length, 2);
    // A reset failure after otherwise successful flashing must still be reported.
    await assert.rejects(loader.restore(steps), /NET reset timeout/);
});

test("NET failure survives a serial-port close error and is logged before cleanup", async () => {
    const original = globalThis.fetch;
    globalThis.fetch = async () => new Response(randomBytes(17));
    const port = new DevicePort(), logs = [];
    port.net.corrupt = true;
    port.close = async () => { throw new Error("USB disconnected during close"); };
    try {
        await assert.rejects(new HactarFlasher({ log: message => logs.push(message), progress() {} }).flash(["net"], port), /NET verification failed/);
        const failureIndex = logs.findIndex(message => message.startsWith("NET update failed:"));
        const cleanupIndex = logs.findIndex((message, index) => index > failureIndex && message.includes("setting NET BOOT HIGH"));
        assert.ok(failureIndex >= 0 && cleanupIndex > failureIndex);
        assert.match(logs.at(-1), /Closing serial port failed: USB disconnected during close/);
        assert.deepEqual(port.pins, [0, 1, 1, 1, 1]);
    } finally { globalThis.fetch = original; }
});

test("manual MGMT flashing preserves user-controlled bootloader signals", async () => {
    const original = globalThis.fetch;
    globalThis.fetch = async () => new Response(randomBytes(259));
    const port = new DevicePort();
    port.setSignals = async () => assert.fail("Do not change signals during manual MGMT flashing");
    try {
        await new HactarFlasher({ log() {}, progress() {} }).flash(["mgmt"], port, true);
        assert.deepEqual(port.opened, [115200]);
        assert.equal(port.mgmt.optionWrites, 1);
        assert.equal(port.mgmt.options[4], 17);
        assert.equal(port.mgmt.options[5], 17 ^ 255);
    } finally { globalThis.fetch = original; }
});

test("setting MGMT version in manual mode reinitializes without changing signals", async () => {
    const original = globalThis.fetch;
    globalThis.fetch = async () => new Response(randomBytes(259));
    const port = new DevicePort();
    port.setSignals = async () => assert.fail("Do not change manual bootloader signals");
    try {
        await new HactarFlasher({ log() {}, progress() {} }).flash(["mgmt"], port, true, 16);
        assert.equal(port.mgmt.options[4], 16);
        assert.equal(port.mgmt.optionWrites, 1);
    } finally { globalThis.fetch = original; }
});

test("invalid MGMT version selection fails before opening the port", async () => {
    const loader = new HactarFlasher({ log() {}, progress() {} });
    const port = { open() { assert.fail("Must not open the port"); } };
    await assert.rejects(loader.flash(["mgmt"], port, false, 18), /version 16 or 17/);
    await assert.rejects(loader.flash(["mgmt"], port, false, null), /version 16 or 17/);
    await assert.rejects(loader.flash(["ui"], port, false, 17), /requires selecting MGMT/);
});

test("failed MGMT handshake reports retries, received bytes and last error", async () => {
    const logs = [], writes = [];
    const mgmt = new Management({
        receivedBytes: 0,
        async drain() {},
        async write(data) { writes.push(data); },
        async read() { throw new Error("Timed out waiting for the device."); },
    }, message => logs.push(message));
    await assert.rejects(mgmt.ready(), /BOOT0 is low.*Last handshake error: Timed out/);
    assert.equal(writes.length, 10);
    assert.equal(logs.length, 11);
    assert.equal(logs.at(-1), "Received 0 serial bytes during the MGMT handshake.");
});
