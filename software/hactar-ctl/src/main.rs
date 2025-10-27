use anyhow::Result;
use clap::{Parser, Subcommand};
use hactar_ctl::{
    hactar_control::HactarControl as GenericHactarControl, tokio_serial_port::TokioSerialPort,
};

type HactarControl = GenericHactarControl<TokioSerialPort>;

/// Hactar device control utility
#[derive(Parser, Debug)]
#[command(name = "hactar-ctl")]
#[command(author, version, about, long_about = None)]
struct Args {
    /// Serial port path (e.g., /dev/ttyUSB0 or COM3)
    #[arg(short, long)]
    port: Option<String>,

    /// Baud rate for serial communication
    #[arg(short, long, default_value_t = 115200)]
    baud_rate: u32,

    #[command(subcommand)]
    command: Command,
}

// TODO other comands
#[derive(Subcommand, Debug)]
enum Command {
    /// Check if a serial device is a Hactar device
    Check,
}

async fn get_available_serial_port() -> Result<String, String> {
    let ports: Result<Vec<tokio_serial::SerialPortInfo>, tokio_serial::Error> =
        tokio_serial::available_ports();

    let ports: Vec<tokio_serial::SerialPortInfo> = match ports {
        Ok(v) => v,
        Err(e) => {
            eprintln!("Error: {}", e);
            return Err(format!("Error: {}", e));
        }
    };

    if ports.is_empty() {
        println!("No serial ports found");
        return Err(String::from("No serial ports found"));
    }

    // println!("Available serial ports");

    let mut hactar_ports: Vec<String> = Vec::new();
    for p in ports {
        // println!("  - {}", p.port_name);
        let serial = TokioSerialPort::open(p.port_name.as_str(), 115200)
            .await
            .expect("Failed to open port");
        let mut control: HactarControl = HactarControl::new(serial);

        let is_hactar: bool = control
            .check_for_hactar()
            .await
            .expect("Failed to check if hactar");

        if is_hactar {
            println!("{} Is a hactar!", p.port_name.as_str());
            hactar_ports.push(p.port_name);
            // push it into an array
        } else {
            println!("{} is not a hactar", p.port_name.as_str());
        }
    }

    return Ok("/dev/ttyUSB0".to_string());
}

#[tokio::main]
async fn main() -> Result<()> {
    let args = Args::parse();

    // Open serial port
    let port = match args.port {
        Some(port) => TokioSerialPort::open(&port, args.baud_rate).await?,
        None => {
            // let selected_port: Result<String, String> = get_available_serial_port().await;
            let selected_port = get_available_serial_port()
                .await
                .expect("Failed to get available serial port");
            TokioSerialPort::open(selected_port.as_str(), args.baud_rate).await?
        }
    };

    // Create HactarControl with the port
    let mut control = HactarControl::new(port);

    match args.command {
        Command::Check => check(&mut control).await,
    }
}

async fn check(ctl: &mut HactarControl) -> Result<()> {
    // Check if device is a Hactar
    let is_hactar = ctl.check_for_hactar().await?;

    if is_hactar {
        println!("✓ Device is a Hactar");
        Ok(())
    } else {
        eprintln!("✗ Not a Hactar device");
        std::process::exit(1);
    }
}
