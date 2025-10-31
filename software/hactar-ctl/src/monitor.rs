use crate::hactar_control::HactarControl;
use crate::SerialPort;
use anyhow::Result;
use std::sync::Arc;

pub struct Monitor<P: SerialPort> {
    port: P,
    // controller: Arc<Mutex<HactarControl<P>>>,
}

impl<P: SerialPort> Monitor<P> {
    pub fn new(port: P) -> Self {
        Self {
            port: port,
            // controller: Arc::new(Mutex::new(controller)),
        }
    }

    pub fn start_read_thread(&mut self) -> Result<()> {
        // let controller = Arc::clone(&self.controller);

        // self.read_thread = Some(tokio::spawn(async move {
        //     loop {
        //         let mut ctrl = controller.lock().await;
        //         if let Err(e) = Self::read_serial_inner(&mut ctrl).await {
        //             eprintln!("Error reading serial: {}", e);
        //         }
        //
        //         drop(ctrl);
        //         tokio::time::sleep(tokio::time::Duration::from_millis(10)).await;
        //     }
        // }));
        return Ok(());
    }

    // pub async fn read_serial(&mut self) -> Result<()> {
    //     if self.controller.data_available().await? {
    //         let data = self.controller.read_line().await?;
    //
    //         match String::from_utf8(data) {
    //             Ok(decoded) => {
    //                 println!("{}", decoded)
    //             }
    //             Err(e) => {
    //                 println!("Failed to decode: {}", e)
    //             }
    //         }
    //     }
    //
    //
    //
    //     return Ok(());
    // }

    // fn read_serial_inner(controller: &mut HactarControl<P>) -> Result<()> {
    //     if controller.data_available().await? {
    //         let data = controller.read_line().await?;
    //         match String::from_utf8(data) {
    //             Ok(decoded) => {
    //                 println!("{}", decoded)
    //             }
    //             Err(e) => {
    //                 println!("Failed to decode: {}", e)
    //             }
    //         }
    //     }
    //
    //     return Ok(());
    // }
}
