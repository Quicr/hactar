use std::time::Duration;
use tokio::io::{AsyncReadExt, AsyncWriteExt};
use tokio::time::timeout;
use tokio_serial::SerialPortBuilderExt;

// Hactar command protocol constants
const CMD_WHO_ARE_YOU: [u8; 5] = [1, 0, 0, 0, 0];
const CMD_DISABLE_LOGS: [u8; 5] = [11, 0, 0, 0, 0];
const CMD_DEFAULT_LOGGING: [u8; 5] = [14, 0, 0, 0, 0];

const EXPECTED_RESPONSE: &[u8] = b"HELLO, I AM A HACTAR DEVICE";
const OK_PREFIX_LEN: usize = 3; // "ok\n"

const BAUD_RATE: u32 = 115200;
const DRAIN_TIMEOUT_MS: u64 = 100;
const RESPONSE_TIMEOUT_SECS: u64 = 2;

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args: Vec<String> = std::env::args().collect();
    if args.len() != 2 {
        eprintln!("Usage: {} <serial_port_path>", args[0]);
        std::process::exit(1);
    }

    // Open serial port
    let mut port = tokio_serial::new(&args[1], BAUD_RATE)
        .data_bits(tokio_serial::DataBits::Eight)
        .stop_bits(tokio_serial::StopBits::One)
        .parity(tokio_serial::Parity::None)
        .open_native_async()?;

    tokio::time::sleep(Duration::from_millis(100)).await;

    // Disable logs to silence boot messages
    port.write_all(&CMD_DISABLE_LOGS).await?;
    port.flush().await?;

    // Drain any pending data
    let mut drain_buffer = [0u8; 1024];
    loop {
        match timeout(Duration::from_millis(DRAIN_TIMEOUT_MS), port.read(&mut drain_buffer)).await {
            Ok(Ok(n)) if n > 0 => continue,
            _ => break,
        }
    }

    // Send "who are you" command
    port.write_all(&CMD_WHO_ARE_YOU).await?;
    port.flush().await?;

    // Read response: "ok\n" + expected message
    let total_bytes = OK_PREFIX_LEN + EXPECTED_RESPONSE.len();
    let mut full_response = vec![0u8; total_bytes];
    timeout(Duration::from_secs(RESPONSE_TIMEOUT_SECS), port.read_exact(&mut full_response)).await??;

    let response = &full_response[OK_PREFIX_LEN..];

    // Restore default logging
    port.write_all(&CMD_DEFAULT_LOGGING).await?;
    port.flush().await?;

    // Verify response
    if response == EXPECTED_RESPONSE {
        println!("✓ Device is a Hactar");
        Ok(())
    } else {
        eprintln!("✗ Not a Hactar device");
        std::process::exit(1);
    }
}
