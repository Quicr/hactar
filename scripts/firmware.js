export const DEFAULT_FIRMWARE_FOLDER = "main";

export function validateFirmwareFolder(folder) {
    if (typeof folder !== "string" || !folder.trim() || folder === "." || folder === ".." || /[\\/\x00-\x1f]/.test(folder)) {
        throw new Error("Invalid firmware folder.");
    }
    return folder;
}

export async function loadFirmwareFolders() {
    const url = new URL("../build/manifest.json", import.meta.url);
    let response;
    try {
        response = await fetch(url, { cache: "no-store" });
    } catch (error) {
        throw new Error(`Could not fetch firmware versions from ${url.href}: ${error.message}`, { cause: error });
    }
    if (!response.ok) throw new Error(`Could not load firmware versions from ${url.href}: HTTP ${response.status}.`);
    let folders;
    try {
        folders = await response.json();
    } catch (error) {
        throw new Error(`Firmware manifest at ${url.href} is not valid JSON.`, { cause: error });
    }
    if (!Array.isArray(folders) || !folders.includes(DEFAULT_FIRMWARE_FOLDER)) {
        throw new Error(`Firmware folder list at ${url.href} must include main.`);
    }
    folders.forEach(validateFirmwareFolder);
    return [DEFAULT_FIRMWARE_FOLDER, ...[...new Set(folders)].filter(folder => folder !== DEFAULT_FIRMWARE_FOLDER).sort()];
}
