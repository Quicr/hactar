#![no_std]
#![deny(missing_docs, warnings)]
#![allow(async_fn_in_trait)]

//! This crate provides core traits that describe the functionality that a Hactar platform
//! provides.  The design is primarily intended to work well on STM32 chips, while allowing the
//! possibility of macOS and mock implementations for development and testing purposes.

/// The [`Platform`] trait encapsulates the capabilities that we expect to be exposed by a device
/// that supports the Hactar UI application.
pub trait Platform: 'static {
    /// A human-readable name for the platform
    fn name() -> &'static str;

    /// The board's status indicator
    type StatusLed<'d>: Led;

    /// An LED that can be used as a status indicator
    fn status_led<'d>(&'d mut self) -> Self::StatusLed<'d>;

    /// Waits until `millis` milliseconds have elapsed
    async fn after_millis(millis: u64);
}

/// An LED on the board
pub trait Led {
    /// Turn on the LED
    fn set_high(&mut self);

    /// Turn off the LED
    fn set_low(&mut self);
}
