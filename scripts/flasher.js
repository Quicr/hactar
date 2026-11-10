import { SerialConnection, Management, sleep } from "./serial.js";
import { STM32 } from "./stm32.js";
import { ESP32 } from "./esp32.js";

const FILES = {
    mgmt: [{ path: "mgmt/mgmt.bin", max: 128 * 1024 }],
    ui: [{ path: "ui/ui.bin", max: 1024 * 1024 }],
    net: [
        { path: "net/bootloader.bin", address: 0, max: 0x8000 },
        { path: "net/partition-table.bin", address: 0x8000, max: 0x1000 },
        { path: "net/net.bin", address: 0x10000, max: 8 * 1024 * 1024 - 0x10000 },
    ],
};

export class HactarFlasher {
    constructor({ log, progress }) { this.log = log; this.progress = progress; }

    async binaries(chips) {
        const result = {};
        for (const chip of chips) {
            result[chip] = await Promise.all(FILES[chip].map(async file => {
                const response = await fetch(new URL(`../build/${file.path}`, import.meta.url), { cache: "no-store" });
                if (!response.ok) throw new Error(`Could not load build/${file.path}: HTTP ${response.status}.`);
                const data = new Uint8Array(await response.arrayBuffer());
                if (!data.length || data.length > file.max || response.headers.get("content-type")?.includes("text/html")) throw new Error(`Invalid firmware file: build/${file.path}.`);
                this.log(`Loaded ${file.path} (${data.length.toLocaleString()} bytes).`);
                return { ...file, name: file.path.split("/").pop(), data };
            }));
        }
        return result;
    }

    async resetMGMT(serial, bootloader) {
        await serial.port.setSignals({ dataTerminalReady: false, requestToSend: false });
        await sleep(100);
        await serial.port.setSignals({ requestToSend: bootloader });
        await sleep(50);
        await serial.port.setSignals({ dataTerminalReady: true });
        await sleep(50);
        await serial.port.setSignals({ dataTerminalReady: false });
        await sleep(100);
    }

    async flash(chips, port, manualBoot = false) {
        // Fetch all selected files before putting any chip into its bootloader.
        const firmware = await this.binaries(chips);
        const serial = new SerialConnection(port);
        const report = chip => (phase, done, total) => this.progress(chip, phase, done, total);
        try {
            await serial.open(chips.includes("mgmt") ? 115200 : 1000000);
            if (chips.includes("mgmt")) {
                this.log("Entering MGMT bootloader (115200 baud, even parity)…");
                if (!manualBoot) await this.resetMGMT(serial, true);
                await serial.drain();
                const stm = new STM32(serial);
                try { await stm.init(); }
                catch (error) {
                    throw new Error(`MGMT bootloader did not respond. For boards without automatic reset, set MGMT BOOT0 high, reset the device, then retry with manual bootloader mode. ${error.message}`);
                }
                await stm.flash(firmware.mgmt[0].data, "mgmt", report("mgmt"));
                await stm.go();
                if (!manualBoot) await this.resetMGMT(serial, false);
                this.log("MGMT flashed and verified.");
                // Web Serial requires closing/reopening to change the baud rate.
                await serial.close();
                if (chips.length > 1) { await sleep(1000); await serial.open(1000000); }
            }

            if (chips.some(chip => chip !== "mgmt")) {
                const mgmt = new Management(serial);
                this.log("Waiting for MGMT (1000000 baud, even parity)…");
                await mgmt.ready();
                if (chips.includes("ui")) {
                    this.log("Entering UI bootloader through MGMT…");
                    try {
                        await mgmt.uiBaud(115200);
                        await mgmt.pin(4, 0);
                        await mgmt.resetUI(true);
                        await sleep(100); await serial.drain();
                        const stm = new STM32(mgmt.tunnel("ui"));
                        await stm.init();
                        await stm.flash(firmware.ui[0].data, "ui", report("ui"));
                    } finally {
                        await this.restore([() => mgmt.uiBaud(1000000), () => mgmt.resetUI(), () => mgmt.resetNET()]);
                    }
                    this.log("UI flashed and verified.");
                }
                if (chips.includes("net")) {
                    this.log("Entering NET ROM bootloader through MGMT…");
                    try {
                        await mgmt.pin(2, 0);
                        await mgmt.resetNET(true);
                        await sleep(100); await serial.drain();
                        await new ESP32(mgmt.tunnel("net")).flash(firmware.net, report("net"));
                    } finally {
                        await this.restore([() => mgmt.resetNET(), () => mgmt.resetUI()]);
                    }
                    this.log("NET flashed and verified.");
                }
            }
        } finally { await serial.close(); }
    }

    async restore(steps) {
        let failure;
        for (const step of steps) {
            try { await step(); }
            catch (error) { failure ??= error; this.log(`Reset failed: ${error.message}`); }
        }
        if (failure) throw failure;
    }
}
