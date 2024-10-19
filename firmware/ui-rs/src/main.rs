#![no_std]
#![no_main]

use defmt::*;
use embassy_executor::Spawner;
use embassy_stm32::gpio::{Level, Output, Speed};
use embassy_time::Timer;
use {defmt_rtt as _, panic_probe as _};

mod cmox;

#[embassy_executor::main]
async fn main(_spawner: Spawner) {
    let p = embassy_stm32::init(Default::default());

    // Initialize the CMOX library
    if !cmox::init(p.CRC) {
        error!("CMOX initialization failed");
        return;
    }

    // Verify that we can get build info from the CMOX library
    let cmox_info = cmox::get_info();
    info!("CMOX info: {} {:x}", cmox_info.version, cmox_info.build);

    // Verify that we can correctly call SHA-256 in CMOX
    let hash_input = b"hello";
    let mut hash = [0u8; 32];
    cmox::sha256(&hash_input[..], &mut hash);

    info!("SHA-256 test: {:x}", hash);

    // Blink the LED
    let mut led = Output::new(p.PC5, Level::High, Speed::Low);
    loop {
        info!("high");
        led.set_high();
        Timer::after_millis(1_000).await;

        info!("low");
        led.set_low();
        Timer::after_millis(1_000).await;
    }
}
