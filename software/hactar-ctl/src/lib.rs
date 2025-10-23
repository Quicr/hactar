pub mod hactar_control;

#[cfg(target_arch = "wasm32")]
mod web_serial;

#[cfg(not(target_arch = "wasm32"))]
pub mod tokio_serial_port;

use anyhow::Result;

#[async_trait::async_trait(?Send)]
pub trait SerialPort {
    async fn write(&mut self, data: &[u8]) -> Result<()>;
    async fn read_with_timeout(&mut self, timeout_ms: u32) -> Result<Vec<u8>>;
    async fn flush(&mut self) -> Result<()>;
}
