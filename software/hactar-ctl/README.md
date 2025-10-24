# hactar-ctl

A cross-platform tool for checking, flashing, and configuring Hactar devices. Runs both as a command-line
utility and as a WASM program leveraging the Web Serial API.

## Prerequisites

- Rust (install from https://rustup.rs/)
- wasm-pack (install with `cargo install wasm-pack`)

## Building and Running

### Command-line Binary

Build and run the binary:

```bash
# Check if a device is a Hactar
cargo run -- check --port /dev/ttyUSB0

# Use custom baud rate
cargo run -- check --port /dev/ttyUSB0 --baud-rate 9600

# Show help
cargo run -- --help
cargo run -- check --help
```

Or build in release mode:

```bash
cargo build --release
./target/release/hactar-ctl check --port /dev/ttyUSB0
```

### HTML + WASM

Build the WASM library:

```bash
wasm-pack build --target web --out-dir pkg
```

This creates a `pkg` directory with the compiled WASM module and JavaScript
bindings. After building the WASM library, serve the project directory with a
local web server:

```bash
python -m http.server 8000
```

Then open http://localhost:8000/index.html in your browser and use the GUI to connect to
a serial port and check if the attached device is a Hactar.

## Project Structure

```
hactar-ctl/
├── Cargo.toml                    # Project configuration with dual targets
├── src/
│   ├── main.rs                   # Command-line binary entry point (with clap)
│   ├── lib.rs                    # WASM library entry point
│   ├── web_serial.rs             # WebSerial implementation (WASM only)
│   ├── tokio_serial_port.rs      # Tokio-serial implementation (native only)
│   └── hactar_control.rs         # Business logic for Hactar protocol
├── index.html                    # HTML interface with Web Serial UI
└── README.md                     # This file
```

## Architecture

This project uses a trait-based architecture to separate serial port I/O from Hactar-specific business logic:

### Core Components

1. **`SerialPort` trait** (`port_trait.rs`)
   - Platform-agnostic abstraction for serial communication
   - Methods: `write()`, `read_with_timeout()`, `flush()`
   - Implemented differently for WASM and native platforms

2. **Platform Implementations**
   - **`WebSerialPort`** (`web_serial.rs`) - Uses Web Serial API via `web-sys` (WASM only)
   - **`TokioSerialPort`** (`tokio_serial_port.rs`) - Uses `tokio-serial` (native only)

3. **`HactarControl`** (`hactar_control.rs`)
   - Contains all Hactar protocol logic (commands, response parsing)
   - Generic over any `SerialPort` implementation
   - Same business logic works on both WASM and native platforms

## Command-Line Interface

The CLI uses `clap` with a subcommand structure:

### Commands

#### `check`
Check if a serial device is a Hactar device.

**Options:**
- `-p, --port <PORT>` - Serial port path (e.g., `/dev/ttyUSB0`, `COM3`)
- `-b, --baud-rate <BAUD_RATE>` - Baud rate for serial communication (default: 115200)

**Examples:**
```bash
# Check device on specific port
hactar-ctl check --port /dev/ttyUSB0

# Check with custom baud rate
hactar-ctl check --port COM3 --baud-rate 9600

# Auto-scan (not yet implemented)
hactar-ctl check
```

## WASM API

The WASM module exports a `HactarControl` class that encapsulates all Hactar protocol logic.

### JavaScript API

```javascript
import init, { HactarControl } from './pkg/hactar_ctl.js';

await init();

// Create instance
const control = new HactarControl();

// Check if Web Serial is supported
if (!HactarControl.isSupported()) {
    console.error('Web Serial API not supported');
}

// Connect to a port
await control.connect(115200);

// Check if device is a Hactar
const isHactar = await control.checkForHactar();
console.log(isHactar ? '✓ Device is a Hactar' : '✗ Not a Hactar');

// Disconnect
await control.disconnect();
```

### Browser Compatibility

The Web Serial API is supported on:
- Chrome/Edge 89+ (all desktop platforms)
- Opera 75+

**Note**: Web Serial API requires a secure context (HTTPS or localhost) and user interaction to request port access.
