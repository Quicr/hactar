import { HactarFlasher } from "./flasher.js";

const updateButton = document.getElementById("updateButton");
const advancedToggle = document.getElementById("advancedToggle");
const advancedSection = document.getElementById("advancedSection");
const consoleDiv = document.getElementById("console");
const themeToggle = document.getElementById("themeToggle");
const selection = document.getElementById("chipSelection");
const manualBoot = document.getElementById("manualBoot");
const status = document.getElementById("status");
const progress = document.getElementById("progress");

let savedTheme;
try { savedTheme = localStorage.getItem("theme"); } catch { /* Storage may be disabled. */ }
themeToggle.checked = savedTheme ? savedTheme === "dark" : window.matchMedia("(prefers-color-scheme: dark)").matches;
document.documentElement.dataset.theme = themeToggle.checked ? "dark" : "light";
window.requestAnimationFrame(() => document.documentElement.classList.remove("no-transition"));
themeToggle.addEventListener("change", () => {
    const theme = themeToggle.checked ? "dark" : "light";
    document.documentElement.dataset.theme = theme;
    try { localStorage.setItem("theme", theme); } catch { /* Storage may be disabled. */ }
});
advancedToggle.addEventListener("change", () => advancedSection.classList.toggle("hidden", !advancedToggle.checked));

function log(message) {
    consoleDiv.textContent += `[${new Date().toLocaleTimeString()}] ${message}\n`;
    consoleDiv.scrollTop = consoleDiv.scrollHeight;
}

let lastPhase;
const flasher = new HactarFlasher({
    log,
    progress(chip, phase, done, total) {
        const percent = Math.floor(done / total * 100);
        status.textContent = `${chip.toUpperCase()}: ${phase} — ${percent}%`;
        progress.value = percent;
        const label = `${chip.toUpperCase()}: ${phase}`;
        if (label !== lastPhase) { log(label); lastPhase = label; }
    },
});

const supported = window.isSecureContext && "serial" in navigator;
if (!supported) {
    document.getElementById("compatibility").textContent = "Web Serial is unavailable. Open this page in desktop Chrome or Edge using HTTPS (or localhost for development).";
    status.textContent = "This browser cannot flash the device.";
}
function updateSelection() {
    updateButton.disabled = !supported || !selection.querySelector("input:checked");
}
selection.addEventListener("change", updateSelection);
updateSelection();

updateButton.addEventListener("click", async () => {
    const chips = [...selection.querySelectorAll("input:checked")].map(input => input.value);
    if (!chips.length) return;
    updateButton.disabled = true;
    selection.disabled = true;
    manualBoot.disabled = true;
    progress.value = 0;
    lastPhase = null;
    try {
        // Port selection must happen within the user's click activation.
        const port = await navigator.serial.requestPort();
        status.textContent = "Loading firmware…";
        log(`Starting update: ${chips.join(" → ").toUpperCase()}.`);
        await flasher.flash(chips, port, manualBoot.checked);
        progress.value = 100;
        status.textContent = "Update complete. All selected firmware has been verified.";
        log(status.textContent);
    } catch (error) {
        status.textContent = error.name === "NotFoundError" ? "No serial port selected." : `Update failed: ${error.message}`;
        log(status.textContent);
        advancedToggle.checked = true;
        advancedSection.classList.remove("hidden");
    } finally {
        selection.disabled = false;
        manualBoot.disabled = false;
        updateSelection();
    }
});
