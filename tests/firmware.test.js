import test from "node:test";
import assert from "node:assert/strict";
import { readFile, readdir } from "node:fs/promises";
import { DEFAULT_FIRMWARE_FOLDER, loadFirmwareFolders } from "../scripts/firmware.js";
import { HactarFlasher } from "../scripts/flasher.js";

test("firmware folder list puts main first, sorts alternatives and removes duplicates", async () => {
    const original = globalThis.fetch;
    globalThis.fetch = async (url, options) => {
        assert.ok(url.pathname.endsWith("/build/manifest.json"));
        assert.equal(options.cache, "no-store");
        return Response.json(["preview", "main", "beta", "preview"]);
    };
    try {
        assert.equal(DEFAULT_FIRMWARE_FOLDER, "main");
        assert.deepEqual(await loadFirmwareFolders(), ["main", "beta", "preview"]);
    } finally { globalThis.fetch = original; }
});

test("firmware folder list reports missing or invalid manifests", async () => {
    const original = globalThis.fetch;
    try {
        globalThis.fetch = async () => new Response("Missing", { status: 404 });
        await assert.rejects(loadFirmwareFolders(), /HTTP 404/);
        for (const list of [[], ["preview"], { main: true }, ["main", "../preview"], ["main", null]]) {
            globalThis.fetch = async () => Response.json(list);
            await assert.rejects(loadFirmwareFolders(), /folder/i);
        }
    } finally { globalThis.fetch = original; }
});

test("firmware loading errors expose the manifest URL and actual request failure", async () => {
    const original = globalThis.fetch;
    try {
        globalThis.fetch = async () => { throw new TypeError("Failed to fetch"); };
        await assert.rejects(loadFirmwareFolders(), /manifest\.json: Failed to fetch/);
        globalThis.fetch = async () => new Response("Missing", { status: 404 });
        await assert.rejects(loadFirmwareFolders(), /manifest\.json: HTTP 404/);
        globalThis.fetch = async () => new Response("<!DOCTYPE html><html></html>");
        await assert.rejects(loadFirmwareFolders(), /manifest\.json is not valid JSON/);
    } finally { globalThis.fetch = original; }
});

test("all firmware downloads default to build/main", async () => {
    const original = globalThis.fetch, paths = [];
    globalThis.fetch = async url => {
        paths.push(url.pathname.split("/build/")[1]);
        return new Response(Uint8Array.of(1, 2, 3));
    };
    try {
        await new HactarFlasher({ log() {}, progress() {} }).binaries(["mgmt", "ui", "net"]);
        assert.deepEqual(paths, [
            "main/mgmt/mgmt.bin", "main/ui/ui.bin", "main/net/bootloader.bin",
            "main/net/partition-table.bin", "main/net/net.bin",
        ]);
    } finally { globalThis.fetch = original; }
});

test("selected folder is URL-encoded as one directory component", async () => {
    const original = globalThis.fetch;
    globalThis.fetch = async url => {
        assert.ok(url.pathname.endsWith("/build/release%20%231/mgmt/mgmt.bin"));
        assert.equal(url.hash, "");
        return new Response(Uint8Array.of(1));
    };
    try {
        await new HactarFlasher({ log() {}, progress() {} }).binaries(["mgmt"], "release #1");
    } finally { globalThis.fetch = original; }
});

test("missing selected firmware never falls back to main or opens the serial port", async () => {
    const original = globalThis.fetch, paths = [];
    globalThis.fetch = async url => {
        paths.push(url.pathname);
        return new Response("Missing", { status: 404 });
    };
    try {
        await assert.rejects(new HactarFlasher({ log() {}, progress() {} }).flash(["mgmt"], {
            open() { assert.fail("Do not open a device with missing firmware"); },
        }, false, 17, "preview"), /build\/preview\/mgmt\/mgmt.bin: HTTP 404/);
        assert.equal(paths.length, 1);
        assert.ok(paths[0].includes("/build/preview/"));
    } finally { globalThis.fetch = original; }
});

test("invalid firmware directories are rejected before any download or serial access", async () => {
    const original = globalThis.fetch;
    globalThis.fetch = async () => assert.fail("Do not download invalid directories");
    const port = { open() { assert.fail("Do not open the port"); } };
    try {
        const loader = new HactarFlasher({ log() {}, progress() {} });
        for (const folder of ["", " ", ".", "..", "../main", "other/main", "other\\main", "main\0", null]) {
            await assert.rejects(loader.flash(["net"], port, false, null, folder), /Invalid firmware folder/);
        }
    } finally { globalThis.fetch = original; }
});

test("published firmware manifest matches build folders and main contains all required binaries", async () => {
    const build = new URL("../build/", import.meta.url);
    const entries = await readdir(build, { withFileTypes: true });
    const folders = entries.filter(entry => entry.isDirectory() && !entry.name.startsWith(".")).map(entry => entry.name).sort();
    const listed = JSON.parse(await readFile(new URL("manifest.json", build), "utf8"));
    assert.equal(listed[0], "main");
    assert.deepEqual([...listed].sort(), folders, "Regenerate with python3 scripts/generate-firmware-manifest.py");
    for (const path of ["mgmt/mgmt.bin", "ui/ui.bin", "net/bootloader.bin", "net/partition-table.bin", "net/net.bin"]) {
        assert.ok((await readFile(new URL(`main/${path}`, build))).length > 0, path);
    }
});
