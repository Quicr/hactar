# WASM Demo

A Rust crate that demonstrates dual-target compilation: command-line binary and WebAssembly library.

## Features

- Shared business logic in `src/logic.rs` with a `hello()` function that generates random greetings
- Command-line binary that prints to stdout
- WebAssembly library that logs to the browser console
- Interactive HTML demo with a button to trigger the WASM function
- **Web Serial API integration** - Access serial ports from WASM code
  - Request and manage serial ports
  - Read and write data through serial connections
  - JavaScript wrapper with simplified async API

## Prerequisites

- Rust (install from https://rustup.rs/)
- wasm-pack (install with `cargo install wasm-pack`)

## Building and Running

### Command-line Binary

Build and run the binary:

```bash
cargo run
```

Or build in release mode:

```bash
cargo build --release
./target/release/wasm-demo
```

### WebAssembly Library

Build the WASM library:

```bash
wasm-pack build --target web
```

This creates a `pkg` directory with the compiled WASM module and JavaScript bindings.

### Running the HTML Demo

After building the WASM library, serve the project directory with a local web server:

```bash
# Using Python 3
python3 -m http.server 8000

# Or using Python 2
python -m SimpleHTTPServer 8000

# Or using Node.js http-server (npm install -g http-server)
http-server
```

Then open http://localhost:8000 in your browser and click the "Say Hello" button. Check the browser console to see the output.

## Project Structure

```
wasm-demo/
├── .cargo/
│   └── config.toml     # Enables web_sys_unstable_apis flag
├── Cargo.toml          # Project configuration with dual targets
├── src/
│   ├── logic.rs        # Shared business logic
│   ├── main.rs         # Command-line binary entry point
│   ├── lib.rs          # WASM library entry point
│   └── serial.rs       # Web Serial API bindings using web-sys
├── index.html          # HTML demo with serial port UI
└── README.md           # This file
```

## How It Works

The `logic::hello()` function generates a random number (1-100) and returns a greeting string. This function is shared by both targets:

- **Binary**: `main.rs` calls `hello()` and prints to stdout using `println!`
- **WASM**: `lib.rs` exports `say_hello()` which calls `hello()` and passes the result to JavaScript's `console.log()`

The HTML demo loads the WASM module and provides a button that triggers the `say_hello()` function when clicked.

## Web Serial API Integration

This project demonstrates how to access the Web Serial API from WASM code using `web-sys` bindings directly.

### Architecture

**Rust Bindings (`src/serial.rs`)**
   - Uses `web-sys` to access the Web Serial API directly from Rust
   - No JavaScript wrapper needed - `web-sys` provides type-safe bindings
   - Converts between Rust types (Vec<u8>, String) and JavaScript types (Uint8Array)
   - Uses `wasm-bindgen-futures` for async/await support with Promises

**Configuration**
   - Requires `--cfg=web_sys_unstable_apis` flag (set in `.cargo/config.toml`)
   - Web Serial API is marked as unstable in `web-sys`

### Available Functions

The following functions are exported to JavaScript:

- `serial_request_port()` - Request a serial port from the user, returns SerialPort object
- `serial_open_port(port, baud_rate)` - Open a port with specified baud rate
- `serial_close_port(port)` - Close a port
- `serial_read_bytes(port)` - Read available data as Vec<u8>
- `serial_write_bytes(port, data)` - Write bytes to the port
- `serial_write_string(port, text)` - Write a string to the port
- `serial_get_ports()` - Get previously granted ports

### Usage Example

```javascript
import init, {
    serial_request_port,
    serial_open_port,
    serial_write_string
} from './pkg/wasm_demo.js';

await init();

// Request and open a port
const port = await serial_request_port();
await serial_open_port(port, 9600);

// Send data
await serial_write_string(port, "Hello from WASM!\n");
```

### Browser Compatibility

The Web Serial API is supported on:
- Chrome/Edge 89+ (all desktop platforms)
- Opera 75+

**Note**: Web Serial API requires a secure context (HTTPS or localhost) and user interaction to request port access.

### Testing

To test the serial port functionality:

1. Build the WASM library: `wasm-pack build --target web`
2. Start a local server: `python3 -m http.server 8000`
3. Open http://localhost:8000 in Chrome or Edge
4. Click "Request Serial Port" and select a device (e.g., Arduino, USB-to-serial adapter)
5. Click "Open Port" to connect
6. Use "Send" to write data and "Read Data" to receive data
