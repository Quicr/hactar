const { invoke } = window.__TAURI__.core;
const { listen } = window.__TAURI__.event;

let pttButtonEl;
let pttLogEl;
let aiButtonEl;
let aiLogEl;

async function ptt_button_press(name) {
  await invoke("ptt_button_press", { name });
}

async function ai_button_press(name) {
  await invoke("ai_button_press", { name });
}

window.addEventListener("load", async () => {
  // Configure the PTT button
  pttButtonEl = document.querySelector("#ptt-button");

  pttButtonEl.addEventListener("mousedown", (e) => {
    e.preventDefault();
    ptt_button_press("mousedown");
  });

  pttButtonEl.addEventListener("mouseup", (e) => {
    e.preventDefault();
    ptt_button_press("mousedown");
  });

  // Configure the AI button
  aiButtonEl = document.querySelector("#ai-button");

  aiButtonEl.addEventListener("mousedown", (e) => {
    e.preventDefault();
    ai_button_press("MouseDown");
  });

  aiButtonEl.addEventListener("mouseup", (e) => {
    e.preventDefault();
    ai_button_press("MouseUp");
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
  const unlisten = await listen('PttState', (e) => {
    console.log("Got State update from listen()", e);
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
  })
});
