use anyhow::Result;
use clap::{Parser, Subcommand};
use hactar_ctl::{hactar_control::HactarControl, /*monitor::Monitor,*/ serial::Serial};
use std::io;
use std::thread;

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
    tool: Tool,
}

#[derive(Subcommand, Debug)]
enum Tool {
    /// Check if a serial device is a Hactar device
    Check,
    Monitor,
    Flash,
    Configurator,
}

fn main() {
    let args = Args::parse();

    // Open serial port
    let port = match args.port {
        Some(port) => BlockingSerialPort::open(&port, args.baud_rate)?,
        None => {
            let selected_port =
                get_available_hactars().expect("Failed to get available serial port");
            BlockingSerialPort::open(selected_port.as_str(), args.baud_rate)?
        }
    };

    // Create HactarControl with the port
    let mut control = HactarControl::new(port);

    match args.tool {
        Tool::Check => check(&mut control),
        Tool::Monitor => Ok(()),
        Tool::Flash => Ok(()),
        Tool::Configurator => Ok(()),
    }
}

fn get_available_hactars() -> Result<String, String> {
    let ports = serialport::available_ports().map_err(|e| format!("Error: {}", e))?;

    if ports.is_empty() {
        println!("No serial ports found");
        return Err(String::from("No serial ports found"));
    }

    let mut hactar_ports: Vec<String> = Vec::new();
    for p in ports {
        match BlockingSerialPort::open(p.port_name.as_str(), 115200) {
            Ok(serial) => {
                let mut control: HactarControl = HactarControl::new(serial);

                match control.check_for_hactar() {
                    Ok(true) => {
                        println!("{} is a hactar!", p.port_name.as_str());
                        hactar_ports.push(p.port_name);
                    }
                    Ok(false) => {
                        // Not a hactar, skip
                    }
                    Err(_) => {
                        // Error checking, skip
                    }
                }
            }
            Err(_) => {
                // Failed to open port, skip
            }
        }
    }

    if hactar_ports.is_empty() {
        return Err(String::from("No hactars found"));
    }

    loop {
        // Select a port
        println!("Select a hactar");
        for (idx, p) in hactar_ports.iter().enumerate() {
            println!("[{}] - {}", idx, p);
        }

        let mut usr_input = String::new();
        io::stdin()
            .read_line(&mut usr_input)
            .expect("Failed to read user input");

        usr_input = usr_input.trim().to_string();

        if usr_input.eq_ignore_ascii_case("quit") {
            return Err(String::from("Quit"));
        }

        match usr_input.parse::<usize>() {
            Ok(n) if n < hactar_ports.len() => {
                return Ok(hactar_ports[n].clone());
            }
            _ => println!("Invalid input. Try again"),
        }
    }
}

fn check(ctl: &mut HactarControl) -> Result<()> {
    // Check if device is a Hactar
    let is_hactar = ctl.check_for_hactar()?;

    if is_hactar {
        println!("✓ Device is a Hactar");
        Ok(())
    } else {
        eprintln!("✗ Not a Hactar device");
        std::process::exit(1);
    }
}

// fn run_monitor(ctl: HactarControl) -> Result<()> {
//     let mut monitor = Monitor::new(ctl);
//
//     // Create a thread for reading
//     let handle = thread::spawn(|| {
//         let mon = &monitor;
//         // print_serial(mon);
//     });
//
//     handle.join().unwrap();
//
//     Ok(())
// }
