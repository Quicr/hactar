#![deny(missing_docs, warnings)]

//! This crate defines an implementation of the Hactar Platform trait on macOS.  The intent is to
//! allow development of changes to the Hactar firmware without having to continuously update a
//! hardware device.

use tokio::time::{sleep, Duration};

use hactar_platform::{Led, Platform};

/// An instantiation of the platform abstraction on macOS
#[derive(Default)]
pub struct MacOsPlatform;

impl Platform for MacOsPlatform {
    fn name() -> &'static str {
        "macos"
    }

    type StatusLed<'d> = StatusLed;

    fn status_led<'d>(&mut self) -> Option<Self::StatusLed<'d>> {
        Some(StatusLed)
    }

    async fn after_millis(millis: u64) {
        sleep(Duration::from_millis(millis.into())).await
    }
}

/// An ersatz status LED that just prints when it is turned on and off
pub struct StatusLed;

impl Led for StatusLed {
    fn set_high(&mut self) {
        println!("set_high()");
    }

    fn set_low(&mut self) {
        println!("set_low()");
    }
}
