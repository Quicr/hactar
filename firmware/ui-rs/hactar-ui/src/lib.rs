#![no_std]
#![deny(missing_docs, warnings)]

//! This crate captures the platform-independent application logic for the Hactar UI chip.

use hactar_platform::{Led, Platform};

/// An App instance for a platform P encapsulates the logic required to implement the application
/// on the specified platform.
pub struct App<P> {
    #[allow(unused)]
    platform: P,
}

impl<P> App<P>
where
    P: Platform,
{
    /// Instantiate the app on the specified platform
    pub fn new(platform: P) -> Self {
        Self { platform }
    }

    /// Run the app
    pub async fn run(mut self) -> ! {
        let delay = 1_000;
        let mut status_led = self.platform.status_led();

        loop {
            P::after_millis(delay).await;
            status_led.set_low();

            P::after_millis(delay).await;
            status_led.set_high();
        }
    }
}
