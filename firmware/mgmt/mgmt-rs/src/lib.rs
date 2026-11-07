#![no_std]

use core::panic::PanicInfo;
use panic_halt as _;

#[unsafe(no_mangle)]
pub extern "C" fn app_main() {
    loop {}
}
