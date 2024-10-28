#![no_std]
#![deny(missing_docs, warnings)]

//! This crate defines an implementation of the Hactar Platform trait on the EV11 board, centered
//! on an STM32F405RG MCU.

use embassy_stm32::{
    gpio::{Level, Output, Speed},
    peripherals, Peripherals,
};
use embassy_time::Timer;

use hactar_platform::{Led, Platform};

/// An instantiation of the platform abstraction on STM32
pub struct Stm32Platform {
    peripherals: Peripherals,
}

impl Default for Stm32Platform {
    fn default() -> Self {
        Self {
            peripherals: embassy_stm32::init(Default::default()),
        }
    }
}

impl Platform for Stm32Platform {
    fn name() -> &'static str {
        "stm32"
    }

    type StatusLed<'d> = StatusLed<'d>;

    fn status_led<'d>(&'d mut self) -> Self::StatusLed<'d> {
        StatusLed(Output::new(
            &mut self.peripherals.PC5,
            Level::High,
            Speed::Low,
        ))
    }

    async fn after_millis(millis: u64) {
        Timer::after_millis(millis).await
    }
}

/// The status LED on the EV11 board, connected to peripheral PC5 of the MCU
pub struct StatusLed<'d>(Output<'d, peripherals::PC5>);

impl<'d> Led for StatusLed<'d> {
    fn set_high(&mut self) {
        self.0.set_high();
    }

    fn set_low(&mut self) {
        self.0.set_low();
    }
}
