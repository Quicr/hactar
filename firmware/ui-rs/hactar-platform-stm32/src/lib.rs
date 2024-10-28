#![no_std]
#![deny(missing_docs, warnings)]

//! TODO(RLB) documentation

use hactar_platform::Platform;

/// TODO(RLB) documentation
#[derive(Default)]
pub struct Stm32Platform;

impl Platform for Stm32Platform {
    fn name() -> &'static str {
        "stm32"
    }
}
