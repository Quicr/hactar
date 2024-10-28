#![deny(missing_docs, warnings)]

//! This crate defines an implementation of the Hactar Platform trait on macOS.  The intent is to
//! allow development of changes to the Hactar firmware without having to continuously update a
//! hardware device.

use tokio::time::{sleep, Duration};

use hactar_platform::{Led, Platform};

/// An instantiation of the platform abstraction on macOS
pub struct MacOsPlatform {
    status_led: StatusLed,
}

impl Default for MacOsPlatform {
    fn default() -> Self {
        Self {
            status_led: StatusLed,
        }
    }
}

impl Platform for MacOsPlatform {
    fn name() -> &'static str {
        "macos"
    }

    type StatusLed<'d> = &'d StatusLed;

    fn status_led<'d>(&'d mut self) -> Self::StatusLed<'d> {
        &self.status_led
    }

    async fn after_millis(millis: u64) {
        sleep(Duration::from_millis(millis.into())).await
    }
}

/// An ersatz status LED that just prints when it is turned on and off
pub struct StatusLed;

impl<'d> Led for &'d StatusLed {
    fn set_high(&mut self) {
        println!("set_high()");
    }

    fn set_low(&mut self) {
        println!("set_low()");
    }
}
