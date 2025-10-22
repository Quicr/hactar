use js_sys::{Array, Uint8Array};
use wasm_bindgen::prelude::*;
use wasm_bindgen::JsCast;
use wasm_bindgen_futures::JsFuture;
use web_sys::{
    ParityType, ReadableStreamDefaultReader, SerialOptions, SerialPort, WritableStreamDefaultWriter,
};

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
