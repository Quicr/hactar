pub mod hactar_control;
pub mod monitor;
#[cfg(not(target_arch = "wasm32"))]
pub mod serial;
#[cfg(target_arch = "wasm32")]
mod web_serial;

// use anyhow::Result;
use std::time::Duration;

pub trait SerialPort {
    fn num_available(&mut self) -> u32;
    fn write(&mut self, data: &Vec<u8>) -> usize;
    fn read_byte(&mut self, timeout: Duration) -> Vec<u8>;
    fn read_bytes(&mut self, num_bytes: usize, timeout: Duration) -> Vec<u8>;
    fn read_line(&mut self, timeout: Duration) -> Vec<u8>;
    fn flush(&mut self);
}
