const { invoke } = window.__TAURI__.core;
const { listen } = window.__TAURI__.event;

let timerButtonEl;
let timerLogEl;

async function button_press(name) {
  await invoke("button_press", { name });
}

window.addEventListener("DOMContentLoaded", async () => {
  timerButtonEl = document.querySelector("#timer-button");
  timerLogEl = document.querySelector("#timer-log");

  timerButtonEl.addEventListener("mousedown", (e) => {
    e.preventDefault();
    button_press("MouseDown");
  });

  timerButtonEl.addEventListener("mouseup", (e) => {
    e.preventDefault();
    button_press("MouseUp");
  });

  const unlisten = await listen('State', (e) => {
    console.log("Got State update from listen()", e);
    if (e.payload === "Idle") {
      timerButtonEl.innerText = "PUSH TO TALK";
      timerButtonEl.disabled = false;
    } else if (e.payload === "Recording") {
      timerButtonEl.innerText = "RECORDING";
      timerButtonEl.disabled = false;
    } else if (e.payload === "Playing") {
      timerButtonEl.innerText = "PLAYING";
      timerButtonEl.disabled = true;
    }
  })
});
