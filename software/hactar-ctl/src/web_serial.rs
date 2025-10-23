use anyhow::{anyhow, Result};
use js_sys::Uint8Array;
use wasm_bindgen::JsCast;
use wasm_bindgen_futures::JsFuture;
use web_sys::{
    ParityType, ReadableStreamDefaultReader, SerialOptions, SerialPort, WritableStreamDefaultWriter,
};

/// WebSerialPort - Web Serial API implementation for WASM
pub struct WebSerialPort {
    port: SerialPort,
    writer: WritableStreamDefaultWriter,
}

impl WebSerialPort {
    /// Check if Web Serial API is supported
    pub fn is_supported() -> bool {
        web_sys::window()
            .and_then(|w| Some(w.navigator().serial()))
            .is_some()
    }

    /// Connect to a serial port via Web Serial API
    pub async fn connect(baud_rate: u32) -> Result<Self> {
        // Get the Serial API from the browser
        let window = web_sys::window().ok_or_else(|| anyhow!("No window found"))?;
        let navigator = window.navigator();
        let serial = navigator.serial();

        // Request a port from the user
        let promise = serial.request_port();
        let port = JsFuture::from(promise)
            .await
            .map_err(|e| anyhow!("Failed to request port: {:?}", e))?;
        let port: SerialPort = port.into();

        // Open the port
        let options = SerialOptions::new(baud_rate);
        options.set_data_bits(8);
        options.set_stop_bits(1);
        options.set_parity(ParityType::None);

        let promise = port.open(&options);
        JsFuture::from(promise)
            .await
            .map_err(|e| anyhow!("Failed to open port: {:?}", e))?;

        // Get the writer for sending data
        let writable = port.writable();
        let writer = writable
            .get_writer()
            .map_err(|e| anyhow!("Failed to get writer: {:?}", e))?
            .dyn_into::<WritableStreamDefaultWriter>()
            .map_err(|e| anyhow!("Failed to cast writer: {:?}", e))?;

        Ok(Self { port, writer })
    }
}

#[async_trait::async_trait(?Send)]
impl crate::SerialPort for WebSerialPort {
    async fn write(&mut self, data: &[u8]) -> Result<()> {
        let uint8_array = Uint8Array::from(data);
        let promise = self.writer.write_with_chunk(&uint8_array);
        JsFuture::from(promise)
            .await
            .map_err(|e| anyhow!("Write failed: {:?}", e))?;
        Ok(())
    }

    async fn read_with_timeout(&mut self, timeout_ms: u32) -> Result<Vec<u8>> {
        let readable = self.port.readable();
        let reader = readable
            .get_reader()
            .dyn_into::<ReadableStreamDefaultReader>()
            .map_err(|e| anyhow!("Failed to get reader: {:?}", e))?;

        // Create a timeout promise
        let timeout_promise = js_sys::Promise::new(&mut |resolve, _reject| {
            if let Some(window) = web_sys::window() {
                let _ = window.set_timeout_with_callback_and_timeout_and_arguments_0(
                    &resolve,
                    timeout_ms as i32,
                );
            }
        });

        // Create a read promise
        let read_promise = reader.read();

        // Race the two promises
        let race_array = js_sys::Array::new();
        race_array.push(&read_promise);
        race_array.push(&timeout_promise);

        let result = JsFuture::from(js_sys::Promise::race(&race_array))
            .await
            .map_err(|e| anyhow!("Read failed: {:?}", e))?;

        // Release the reader
        reader.release_lock();

        // Check if this is a read result or timeout
        let done = js_sys::Reflect::get(&result, &"done".into());
        if done.is_ok() {
            // This is a read result
            if done.unwrap().as_bool().unwrap_or(false) {
                return Ok(Vec::new()); // EOF
            }

            let value = js_sys::Reflect::get(&result, &"value".into())
                .map_err(|e| anyhow!("Failed to get value: {:?}", e))?;
            let uint8_array = Uint8Array::from(value);

            let mut data = vec![0u8; uint8_array.length() as usize];
            uint8_array.copy_to(&mut data);

            Ok(data)
        } else {
            // Timeout occurred
            Ok(Vec::new())
        }
    }

    async fn flush(&mut self) -> Result<()> {
        // WebSerial doesn't require explicit flushing
        Ok(())
    }
}
