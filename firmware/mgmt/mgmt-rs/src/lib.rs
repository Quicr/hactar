#![no_std]

use core::ffi::c_void;
use core::panic::PanicInfo;
use panic_halt as _;

pub mod drivers;

#[unsafe(no_mangle)]
pub extern "C" fn app_main() -> i32 {
    unsafe {
        // let port = drivers::gpio::PortB();
        // drivers::gpio::HAL_GPIO_WritePin(port, 1 << 4, drivers::uart::test());

        let led_r = drivers::gpio::led_r();
        drivers::gpio::HAL_GPIO_WritePin(led_r.port, led_r.pin, 1);
    }
    return 0;
}
