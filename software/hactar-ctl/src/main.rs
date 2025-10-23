use hactar_ctl::hactar_control::HactarControl;
use hactar_ctl::tokio_serial_port::TokioSerialPort;

const BAUD_RATE: u32 = 115200;

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args: Vec<String> = std::env::args().collect();
    if args.len() != 2 {
        eprintln!("Usage: {} <serial_port_path>", args[0]);
        std::process::exit(1);
    }

    // Open serial port
    let port = TokioSerialPort::open(&args[1], BAUD_RATE).await?;

    // Create HactarControl with the port
    let mut control = HactarControl::new(port);

    // Check if device is a Hactar
    let is_hactar = control.check_for_hactar().await?;

    if is_hactar {
        println!("✓ Device is a Hactar");
        Ok(())
    } else {
        eprintln!("✗ Not a Hactar device");
        std::process::exit(1);
    }
}
