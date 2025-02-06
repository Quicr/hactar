const { invoke } = window.__TAURI__.core;
const { listen } = window.__TAURI__.event;

let pttButtonEl;
let pttLogEl;
let aiButtonEl;
let aiLogEl;

async function start() {
  await invoke("start");
}

async function ptt_button_press(name) {
  await invoke("ptt_button_press", { name });
}

async function ai_button_press(name) {
  await invoke("ai_button_press", { name });
}

async function handle_ptt(e) {
  if (e.payload === "Idle") {
    pttButtonEl.innerText = "PTT";
    pttButtonEl.disabled = false;
  } else if (e.payload === "Recording") {
    pttButtonEl.innerText = "RECORDING";
    pttButtonEl.disabled = false;
  } else if (e.payload === "Playing") {
    pttButtonEl.innerText = "PLAYING";
    pttButtonEl.disabled = true;
  }
}

async function handle_screen(e) {
  let { left, top, width, height, data } = e.payload;

  const screen = document.getElementById("screen");
  const ctx = screen.getContext("2d");

  const imageData = ctx.createImageData(width, height);

  if (data.length != imageData.data.length) {
    console.log(`malformed command ${data.length} != ${imageData.data.length}`);
    return;
  }

  for (let i = 0; i < imageData.data.length; i ++) {
    imageData.data[i] = data[i];
  }
  
  ctx.putImageData(imageData, left, top);
}

async function handle_events() {
  const ptt_state = listen('PttState', handle_ptt);
  const screen = listen('Screen', handle_screen);
  return Promise.all([ptt_state, screen])
}

window.addEventListener("DOMContentLoaded", async () => {
  console.log("content loaded");

  // Configure the PTT button
  pttButtonEl = document.querySelector("#ptt-button");

  pttButtonEl.addEventListener("mousedown", (e) => {
    ptt_button_press("mousedown");
  });

  pttButtonEl.addEventListener("mouseup", (e) => {
    ptt_button_press("mouseup");
  });

  // Configure the AI button
  aiButtonEl = document.querySelector("#ai-button");

  aiButtonEl.addEventListener("mousedown", (e) => {
    ai_button_press("mousedown");
  });

  aiButtonEl.addEventListener("mouseup", (e) => {
    ai_button_press("mouseup");
  });

  let asClass = (code) => {
    return "." + code;
  }

  // Capture keyboard events
  document.addEventListener("keydown", (e) => {
    document.querySelector(asClass(e.code)).classList.add("active");
    invoke("keydown", { code: e.code });
  });

  document.addEventListener("keyup", (e) => {
    document.querySelector(asClass(e.code)).classList.remove("active");
    invoke("keyup", { code: e.code });
  });

  // Await events
  await handle_events();

  // Notify the backend that the UI has started
  start();
});
