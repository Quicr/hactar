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

#[tokio::main]
async fn main() -> Result<()> {
    let args = Args::parse();

    // Open serial port
    let port = match args.port {
        Some(port) => TokioSerialPort::open(&port, args.baud_rate).await?,
        None => todo!("Scan through ports looking for Hactars"),
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
