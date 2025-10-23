use crate::SerialPort;
use anyhow::Result;
use num_enum::IntoPrimitive;

#[repr(u8)]
#[derive(Copy, Clone, PartialEq, IntoPrimitive)]
enum Command {
    WhoAreYou = 0x01,
    DisableLogs = 0x0b,
    DefaultLogging = 0x0e,
}

// Hactar command protocol constants
const OK_RESPONSE: &[u8] = b"Ok\n";
const HELLO_RESPONSE: &[u8] = b"HELLO, I AM A HACTAR DEVICE";

const DRAIN_TIMEOUT_MS: u32 = 100;
const RESPONSE_TIMEOUT_MS: u32 = 200;

/// HactarControl - Business logic for Hactar device communication
/// Works with any SerialPort implementation
pub struct HactarControl<P: SerialPort> {
    port: P,
}

impl<P: SerialPort> HactarControl<P> {
    /// Create a new HactarControl with a serial port implementation
    pub fn new(port: P) -> Self {
        Self { port }
    }

    async fn write_command(&mut self, cmd: Command) -> Result<()> {
        let cmd_data: [u8; 5] = [cmd.into(), 0, 0, 0, 0];
        self.port.write(&cmd_data).await?;
        self.port.flush().await?;
        Ok(())
    }

    /// Drain any pending data from the serial port
    async fn drain_pending_data(&mut self) -> Result<()> {
        loop {
            let data = self.port.read_with_timeout(DRAIN_TIMEOUT_MS).await?;
            if data.is_empty() {
                break;
            }
        }
        Ok(())
    }

    /// Check if the connected device is a Hactar
    /// Returns true if verified, false otherwise
    pub async fn check_for_hactar(&mut self) -> Result<bool> {
        // Disable logs to silence boot messages
        self.write_command(Command::DisableLogs).await?;

        // Drain any pending data
        self.drain_pending_data().await?;

        // Send "who are you" command
        self.write_command(Command::WhoAreYou).await?;

        // Read response: "ok\n" + expected message
        let total_bytes = OK_RESPONSE.len() + HELLO_RESPONSE.len();
        let mut full_response = Vec::new();

        // Read with timeout until we have enough data
        while full_response.len() < total_bytes {
            let chunk = self.port.read_with_timeout(RESPONSE_TIMEOUT_MS).await?;
            if chunk.is_empty() {
                break;
            }

            full_response.extend_from_slice(&chunk);
        }

        // Restore default logging (even if we had an error reading)
        self.write_command(Command::DefaultLogging).await?;

        // Verify we got enough data
        if full_response.len() < total_bytes {
            return Ok(false);
        }

        // Verify response
        let (ok, hello) = full_response.split_at(OK_RESPONSE.len());
        Ok(ok == OK_RESPONSE && hello == HELLO_RESPONSE)
    }
}

// WASM-specific implementation
#[cfg(target_arch = "wasm32")]
mod wasm {
    use super::*;
    use crate::web_serial::WebSerialPort;
    use wasm_bindgen::prelude::*;

    /// HactarControl exposed to JavaScript via WASM
    #[wasm_bindgen]
    pub struct HactarControl {
        inner: Option<super::HactarControl<WebSerialPort>>,
    }

    #[wasm_bindgen]
    impl HactarControl {
        /// Create a new HactarControl instance
        #[wasm_bindgen(constructor)]
        pub fn new() -> Self {
            Self { inner: None }
        }

        /// Check if Web Serial API is supported
        #[wasm_bindgen(js_name = isSupported)]
        pub fn is_supported() -> bool {
            WebSerialPort::is_supported()
        }

        /// Check if currently connected to a serial port
        #[wasm_bindgen(js_name = isConnected)]
        pub fn is_connected(&self) -> bool {
            self.inner.is_some()
        }

        /// Connect to a serial port
        #[wasm_bindgen]
        pub async fn connect(&mut self, baud_rate: u32) -> Result<(), JsValue> {
            let port = WebSerialPort::connect(baud_rate)
                .await
                .map_err(|e| JsValue::from_str(&e.to_string()))?;

            self.inner = Some(super::HactarControl::new(port));
            Ok(())
        }

        /// Disconnect from the serial port
        #[wasm_bindgen]
        pub async fn disconnect(&mut self) -> Result<(), JsValue> {
            if let Some(control) = self.inner.take() {
                // We can't easily disconnect without consuming the port
                // For now, just drop it
                drop(control);
            }
            Ok(())
        }

        /// Check if the connected device is a Hactar
        #[wasm_bindgen(js_name = checkForHactar)]
        pub async fn check_for_hactar(&mut self) -> Result<bool, JsValue> {
            let control = self
                .inner
                .as_mut()
                .ok_or_else(|| JsValue::from_str("Not connected to a serial port"))?;

            control
                .check_for_hactar()
                .await
                .map_err(|e| JsValue::from_str(&e.to_string()))
        }
    }
}

#[cfg(target_arch = "wasm32")]
pub use wasm::HactarControl as WasmHactarControl;
