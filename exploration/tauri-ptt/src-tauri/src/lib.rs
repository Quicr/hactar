// Prevents additional console window on Windows in release, DO NOT REMOVE!!
#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

use cpal::{
    traits::{DeviceTrait, HostTrait, StreamTrait},
    Stream,
};
use once_cell::sync::OnceCell;
use std::sync::{Arc, Mutex};
use tauri::Emitter;

type Sample = f32;

enum PttState {
    Idle,
    Recording(Arc<Mutex<Vec<Sample>>>, Stream),
    Playing(Arc<Mutex<Vec<Sample>>>, Stream),
}

// The streams are not safe to use across threads, but they're never actually used.  We just hold
// them to keep them alive.
unsafe impl Send for PttState {}
unsafe impl Sync for PttState {}

struct AppState {
    ptt_state: PttState,
}

impl core::fmt::Debug for AppState {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        f.write_str("AppState")
    }
}

impl AppState {
    fn new() -> Self {
        Self {
            ptt_state: PttState::Idle,
        }
    }

    fn start_transmit(&mut self) {
        println!("Start transmit");
        let app = APP_HANDLE.get().unwrap();

        self.ptt_state = match &mut self.ptt_state {
            PttState::Idle => {
                let storage = Arc::new(Mutex::new(Vec::<f32>::new()));
                let storage_clone = storage.clone();

                let data_fn = move |data: &[f32], _: &cpal::InputCallbackInfo| {
                    let mut storage = storage_clone.lock().unwrap();
                    storage.extend_from_slice(data);

                    println!("record {}", data.len());
                };

                let err_fn = move |err| {
                    eprintln!("an error occurred on stream: {}", err);
                };

                let host = cpal::default_host();
                let input_device = host.default_input_device().unwrap();
                let config = input_device.default_input_config().unwrap();
                println!("config {:?}", config);
                let input_stream = input_device
                    .build_input_stream(&config.into(), data_fn, err_fn, None)
                    .unwrap();

                input_stream.play().unwrap();

                app.emit::<&str>("State", "Recording").unwrap();
                PttState::Recording(storage, input_stream)
            }

            _ => unreachable!(),
        };
    }

    fn stop_transmit(&mut self) {
        println!("Stop transmit");
        self.start_play();
    }

    fn start_play(&mut self) {
        println!("Start play");
        let app = APP_HANDLE.get().unwrap();

        self.ptt_state = match &mut self.ptt_state {
            PttState::Recording(storage, input_stream) => {
                // Stop the stream so that we can take the storage.  It will get cleaned up at the
                // end of this method.
                input_stream.pause().unwrap();

                let storage = storage.clone();
                let storage_clone = storage.clone();
                let data_fn = move |data: &mut [f32], _: &cpal::OutputCallbackInfo| {
                    let mut storage = storage_clone.lock().unwrap();

                    let len = core::cmp::min(data.len() / 2, storage.len());
                    let stereo = storage
                        .iter()
                        .take(len)
                        .map(|x| [x, x])
                        .flatten()
                        .enumerate();
                    for (i, sample) in stereo {
                        data[i] = *sample;
                    }

                    data[(2 * len)..].fill(0.);

                    println!("play {}", len);

                    *storage = storage.split_off(len);

                    if storage.is_empty() {
                        std::thread::spawn(play_done_cb);
                    }
                };

                let err_fn = move |err| {
                    eprintln!("an error occurred on stream: {}", err);
                };

                let host = cpal::default_host();
                let output_device = host.default_output_device().unwrap();
                let config = output_device.default_output_config().unwrap();
                println!("config {:?}", config);
                let output_stream = output_device
                    .build_output_stream(&config.into(), data_fn, err_fn, None)
                    .unwrap();

                output_stream.play().unwrap();

                app.emit::<&str>("State", "Playing").unwrap();
                PttState::Playing(storage, output_stream)
            }

            _ => unreachable!(),
        };
    }

    fn return_to_idle(&mut self) {
        println!("Return to idle");
        let app = APP_HANDLE.get().unwrap();

        self.ptt_state = match &mut self.ptt_state {
            PttState::Playing(_storage, output_stream) => {
                // Stop the stream so that we shut down gracefully.
                output_stream.pause().unwrap();

                app.emit::<&str>("State", "Idle").unwrap();
                PttState::Idle
            }

            _ => unreachable!(),
        };
    }
}

static STATE: OnceCell<Mutex<AppState>> = OnceCell::new();

static APP_HANDLE: OnceCell<tauri::AppHandle> = OnceCell::new();

fn play_done_cb() {
    let mut state = STATE.get().unwrap().lock().unwrap();
    state.return_to_idle();
}

#[tauri::command]
fn button_press(name: &str) {
    println!("Received button press: {}", name);
    let mut state = STATE.get().unwrap().lock().unwrap();
    match name {
        "MouseDown" => state.start_transmit(),
        "MouseUp" => state.stop_transmit(),
        _ => state.return_to_idle(),
    }
}

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .setup(|app| {
            APP_HANDLE.set(app.handle().clone()).unwrap();
            STATE.set(Mutex::new(AppState::new())).unwrap();
            Ok(())
        })
        .invoke_handler(tauri::generate_handler![button_press])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
