use js_sys::{Array, Uint8Array};
use wasm_bindgen::prelude::*;
use wasm_bindgen::JsCast;
use wasm_bindgen_futures::JsFuture;
use web_sys::{
    ParityType, ReadableStreamDefaultReader, SerialOptions, SerialPort, WritableStreamDefaultWriter,
};

// Hactar command protocol constants
const CMD_WHO_ARE_YOU: [u8; 5] = [1, 0, 0, 0, 0];
const CMD_DISABLE_LOGS: [u8; 5] = [11, 0, 0, 0, 0];
const CMD_DEFAULT_LOGGING: [u8; 5] = [14, 0, 0, 0, 0];

const EXPECTED_RESPONSE: &str = "HELLO, I AM A HACTAR DEVICE";
const OK_PREFIX_LEN: usize = 3; // "ok\n"

const DRAIN_TIMEOUT_MS: i32 = 100;
const RESPONSE_TIMEOUT_MS: i32 = 2000;

/// Get the Serial API from the browser
fn get_serial() -> Result<web_sys::Serial, JsValue> {
    let window = web_sys::window().ok_or("No window found")?;
    let navigator = window.navigator();
    Ok(navigator.serial())
}

/// Request a serial port from the user
/// Returns the SerialPort object directly
#[wasm_bindgen]
pub async fn serial_request_port() -> Result<SerialPort, JsValue> {
    let serial = get_serial()?;
    let promise = serial.request_port();
    let port = JsFuture::from(promise).await?;
    Ok(port.into())
}

/// Open a serial port with the specified baud rate
#[wasm_bindgen]
pub async fn serial_open_port(port: &SerialPort, baud_rate: u32) -> Result<(), JsValue> {
    let options = SerialOptions::new(baud_rate);
    options.set_data_bits(8);
    options.set_stop_bits(1);
    options.set_parity(ParityType::None);

    let promise = port.open(&options);
    JsFuture::from(promise).await?;
    Ok(())
}

/// Close a serial port
#[wasm_bindgen]
pub async fn serial_close_port(port: &SerialPort) -> Result<(), JsValue> {
    let promise = port.close();
    JsFuture::from(promise).await?;
    Ok(())
}

/// Read bytes from a serial port
/// Returns a Vec<u8> containing the data read
#[wasm_bindgen]
pub async fn serial_read_bytes(port: &SerialPort) -> Result<Vec<u8>, JsValue> {
    let readable = port.readable();
    let reader = readable
        .get_reader()
        .dyn_into::<ReadableStreamDefaultReader>()?;

    // Read one chunk
    let promise = reader.read();
    let result = JsFuture::from(promise).await?;

    // Release the reader lock
    reader.release_lock();

    // Extract the value from the result
    let done = js_sys::Reflect::get(&result, &"done".into())?;
    if done.as_bool().unwrap_or(false) {
        return Ok(Vec::new()); // EOF
    }

    let value = js_sys::Reflect::get(&result, &"value".into())?;
    let uint8_array = Uint8Array::from(value);

    // Convert to Vec<u8>
    let mut data = vec![0u8; uint8_array.length() as usize];
    uint8_array.copy_to(&mut data);

    Ok(data)
}

/// Write bytes to a serial port
#[wasm_bindgen]
pub async fn serial_write_bytes(port: &SerialPort, data: &[u8]) -> Result<(), JsValue> {
    let writable = port.writable();
    let writer = writable
        .get_writer()?
        .dyn_into::<WritableStreamDefaultWriter>()?;

    // Convert data to Uint8Array
    let uint8_array = Uint8Array::from(data);

    // Write the data
    let promise = writer.write_with_chunk(&uint8_array);
    JsFuture::from(promise).await?;

    // Release the writer lock
    writer.release_lock();

    Ok(())
}

/// Write a string to a serial port
#[wasm_bindgen]
pub async fn serial_write_string(port: &SerialPort, text: &str) -> Result<(), JsValue> {
    serial_write_bytes(port, text.as_bytes()).await
}

/// Get list of previously granted ports
/// Returns an Array of SerialPort objects
#[wasm_bindgen]
pub async fn serial_get_ports() -> Result<Array, JsValue> {
    let serial = get_serial()?;
    let promise = serial.get_ports();
    let ports = JsFuture::from(promise).await?;
    Ok(Array::from(&ports))
}

/// Read from serial port with timeout
/// Returns data read or empty vector if timeout
async fn read_with_timeout(port: &SerialPort, timeout_ms: i32) -> Result<Vec<u8>, JsValue> {
    let readable = port.readable();
    let reader = readable
        .get_reader()
        .dyn_into::<ReadableStreamDefaultReader>()?;

    // Create a timeout promise
    let timeout_promise = js_sys::Promise::new(&mut |resolve, _reject| {
        web_sys::window()
            .unwrap()
            .set_timeout_with_callback_and_timeout_and_arguments_0(&resolve, timeout_ms)
            .unwrap();
    });

    // Create a read promise
    let read_promise = reader.read();

    // Race the two promises
    let race_array = js_sys::Array::new();
    race_array.push(&read_promise);
    race_array.push(&timeout_promise);

    let result = JsFuture::from(js_sys::Promise::race(&race_array)).await?;

    // Release the reader
    reader.release_lock();

    // Check if this is a read result or timeout
    let done = js_sys::Reflect::get(&result, &"done".into());
    if done.is_ok() {
        // This is a read result
        if done.unwrap().as_bool().unwrap_or(false) {
            return Ok(Vec::new()); // EOF
        }

        let value = js_sys::Reflect::get(&result, &"value".into())?;
        let uint8_array = Uint8Array::from(value);

        let mut data = vec![0u8; uint8_array.length() as usize];
        uint8_array.copy_to(&mut data);

        Ok(data)
    } else {
        // Timeout occurred
        Ok(Vec::new())
    }
}

/// Test if the connected device is a Hactar device
/// Writes "who are you", reads the response, and verifies it matches "HELLO, I AM A HACTAR DEVICE"
/// Returns true if verified, false otherwise
#[wasm_bindgen]
pub async fn test_for_hactar(port: &SerialPort) -> Result<bool, JsValue> {
    // Write the query
    serial_write_string(port, "who are you").await?;

    // Read response with timeout (up to 2000ms total)
    let mut response = Vec::new();
    let start = js_sys::Date::now();
    let total_timeout = 2000.0; // 2 seconds total
    let idle_timeout = 100; // 100ms idle timeout

    loop {
        // Check total timeout
        if js_sys::Date::now() - start > total_timeout {
            break;
        }

        // Try to read with idle timeout
        let data = read_with_timeout(port, idle_timeout).await?;

        if data.is_empty() {
            // No data received within idle timeout, assume device is done sending
            break;
        }

        response.extend_from_slice(&data);
    }

    // Convert response to string and verify
    let response_str = String::from_utf8_lossy(&response);
    let expected = "HELLO, I AM A HACTAR DEVICE";

    Ok(response_str.trim() == expected)
}

/// HactarSerial - Encapsulates Web Serial API logic for communicating with Hactar devices
#[wasm_bindgen]
pub struct HactarSerial {
    port: Option<SerialPort>,
    writer: Option<WritableStreamDefaultWriter>,
}

#[wasm_bindgen]
impl HactarSerial {
    /// Create a new HactarSerial instance
    #[wasm_bindgen(constructor)]
    pub fn new() -> Self {
        Self {
            port: None,
            writer: None,
        }
    }

    /// Check if Web Serial API is supported
    #[wasm_bindgen(js_name = isSupported)]
    pub fn is_supported() -> bool {
        web_sys::window()
            .and_then(|w| Some(w.navigator().serial()))
            .is_some()
    }

    /// Check if currently connected to a serial port
    #[wasm_bindgen(js_name = isConnected)]
    pub fn is_connected(&self) -> bool {
        self.port.is_some() && self.writer.is_some()
    }

    /// Connect to a serial port
    #[wasm_bindgen]
    pub async fn connect(&mut self, baud_rate: u32) -> Result<(), JsValue> {
        if !Self::is_supported() {
            return Err(JsValue::from_str("Web Serial API is not supported in this browser"));
        }

        // Request a port from the user
        let serial = get_serial()?;
        let promise = serial.request_port();
        let port = JsFuture::from(promise).await?;
        let port: SerialPort = port.into();

        // Open the port
        let options = SerialOptions::new(baud_rate);
        options.set_data_bits(8);
        options.set_stop_bits(1);
        options.set_parity(ParityType::None);

        let promise = port.open(&options);
        JsFuture::from(promise).await?;

        // Get the writer for sending data
        let writable = port.writable();
        let writer = writable
            .get_writer()?
            .dyn_into::<WritableStreamDefaultWriter>()?;

        self.port = Some(port);
        self.writer = Some(writer);

        Ok(())
    }

    /// Disconnect from the serial port
    #[wasm_bindgen]
    pub async fn disconnect(&mut self) -> Result<(), JsValue> {
        if let Some(writer) = self.writer.take() {
            writer.release_lock();
        }

        if let Some(port) = self.port.take() {
            let promise = port.close();
            JsFuture::from(promise).await?;
        }

        Ok(())
    }

    /// Read data from the serial port with a timeout
    async fn read_with_timeout(&self, timeout_ms: i32) -> Result<Vec<u8>, JsValue> {
        let port = self.port.as_ref()
            .ok_or_else(|| JsValue::from_str("Port is not readable"))?;

        let readable = port.readable();
        let reader = readable
            .get_reader()
            .dyn_into::<ReadableStreamDefaultReader>()?;

        // Create a timeout promise
        let timeout_promise = js_sys::Promise::new(&mut |resolve, _reject| {
            web_sys::window()
                .unwrap()
                .set_timeout_with_callback_and_timeout_and_arguments_0(&resolve, timeout_ms)
                .unwrap();
        });

        // Create a read promise
        let read_promise = reader.read();

        // Race the two promises
        let race_array = js_sys::Array::new();
        race_array.push(&read_promise);
        race_array.push(&timeout_promise);

        let result = JsFuture::from(js_sys::Promise::race(&race_array)).await?;

        // Release the reader
        reader.release_lock();

        // Check if this is a read result or timeout
        let done = js_sys::Reflect::get(&result, &"done".into());
        if done.is_ok() {
            // This is a read result
            if done.unwrap().as_bool().unwrap_or(false) {
                return Ok(Vec::new()); // EOF
            }

            let value = js_sys::Reflect::get(&result, &"value".into())?;
            let uint8_array = Uint8Array::from(value);

            let mut data = vec![0u8; uint8_array.length() as usize];
            uint8_array.copy_to(&mut data);

            Ok(data)
        } else {
            // Timeout occurred
            Ok(Vec::new())
        }
    }

    /// Drain any pending data from the serial port
    async fn drain_pending_data(&self) -> Result<(), JsValue> {
        loop {
            let data = self.read_with_timeout(DRAIN_TIMEOUT_MS).await?;
            if data.is_empty() {
                break;
            }
        }
        Ok(())
    }

    /// Check if the connected device is a Hactar
    #[wasm_bindgen(js_name = checkForHactar)]
    pub async fn check_for_hactar(&self) -> Result<bool, JsValue> {
        if !self.is_connected() {
            return Err(JsValue::from_str("Not connected to a serial port"));
        }

        let writer = self.writer.as_ref().unwrap();
        let port = self.port.as_ref().unwrap();

        // Disable logs to silence boot messages
        let uint8_array = Uint8Array::from(&CMD_DISABLE_LOGS[..]);
        let promise = writer.write_with_chunk(&uint8_array);
        JsFuture::from(promise).await?;

        // Drain any pending data
        self.drain_pending_data().await?;

        // Send "who are you" command
        let uint8_array = Uint8Array::from(&CMD_WHO_ARE_YOU[..]);
        let promise = writer.write_with_chunk(&uint8_array);
        JsFuture::from(promise).await?;

        // Read response: "ok\n" + expected message
        let total_bytes = OK_PREFIX_LEN + EXPECTED_RESPONSE.len();
        let readable = port.readable();
        let reader = readable
            .get_reader()
            .dyn_into::<ReadableStreamDefaultReader>()?;

        let mut full_response = vec![0u8; total_bytes];
        let mut bytes_read = 0;

        let start = js_sys::Date::now();
        while bytes_read < total_bytes && (js_sys::Date::now() - start) < RESPONSE_TIMEOUT_MS as f64 {
            let promise = reader.read();
            let result = JsFuture::from(promise).await?;

            let done = js_sys::Reflect::get(&result, &"done".into())?;
            if done.as_bool().unwrap_or(false) {
                break;
            }

            let value = js_sys::Reflect::get(&result, &"value".into())?;
            let uint8_array = Uint8Array::from(value);

            let bytes_to_copy = std::cmp::min(uint8_array.length() as usize, total_bytes - bytes_read);
            uint8_array.copy_to(&mut full_response[bytes_read..bytes_read + bytes_to_copy]);
            bytes_read += bytes_to_copy;
        }

        // Release the reader
        reader.release_lock();

        // Restore default logging (even if we had an error reading)
        let uint8_array = Uint8Array::from(&CMD_DEFAULT_LOGGING[..]);
        let promise = writer.write_with_chunk(&uint8_array);
        let _ = JsFuture::from(promise).await; // Ignore errors when restoring logging

        // Verify response
        let response_str = String::from_utf8_lossy(&full_response[OK_PREFIX_LEN..]);

        Ok(response_str == EXPECTED_RESPONSE)
    }
}
