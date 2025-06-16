import { flasher } from "./flasher.js";


const updateButton = document.getElementById('updateButton');
const advancedToggle = document.getElementById('advancedToggle');
const advancedSection = document.getElementById('advancedSection');
const consoleDiv = document.getElementById('console');
const debugInput = document.getElementById('debugInput');
const sendButton = document.getElementById('sendButton');
const themeToggle = document.getElementById('themeToggle');

// Detect system theme
const systemPrefersDark = window.matchMedia('(prefers-color-scheme: dark)').matches;
const theme = systemPrefersDark ? 'dark' : 'light';
document.documentElement.setAttribute('data-theme', theme);

// Detect system theme
document.documentElement.setAttribute('data-theme', systemPrefersDark ? 'dark' : 'light');

// Match toggle to system preference
document.addEventListener('DOMContentLoaded', () => {
  document.getElementById('themeToggle').checked = systemPrefersDark;
});

// Remove transition-blocking class after first render
window.requestAnimationFrame(() => {
  document.documentElement.classList.remove('no-transition');
});


updateButton.addEventListener('click', async () => {
  logToConsole("Starting device update...");
  await flasher.Flash();
  setTimeout(() => {
    logToConsole("✅ Update complete!");
  }, 2000);
});

advancedToggle.addEventListener('change', () => {
  advancedSection.classList.toggle('hidden', !advancedToggle.checked);
});

sendButton.addEventListener('click', () => {
  const command = debugInput.value.trim();
  if (command) {
    logToConsole(`> ${command}`);
    debugInput.value = '';
    setTimeout(() => {
      logToConsole(`Response: "${command}" processed.`);
    }, 500);
  }
});

themeToggle.addEventListener('change', () => {
  const newTheme = themeToggle.checked ? 'dark' : 'light';
  document.documentElement.setAttribute('data-theme', newTheme);
  localStorage.setItem('theme', newTheme);
});


function logToConsole(message) {
  const timestamp = new Date().toLocaleTimeString();
  consoleDiv.textContent += `[${timestamp}] ${message}\n`;
  consoleDiv.scrollTop = consoleDiv.scrollHeight;
}