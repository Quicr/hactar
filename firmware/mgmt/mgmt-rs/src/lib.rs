#![no_std]

// use core::ffi::c_void;
// use core::panic::PanicInfo;
use panic_halt as _;

use crate::drivers::uart;

pub mod drivers;

#[unsafe(no_mangle)]
pub extern "C" fn app_main() -> i32 {
    unsafe {
        let led_r = drivers::gpio::led_r();
        drivers::gpio::HAL_GPIO_WritePin(led_r.port, led_r.pin, 1);

        let led_b = drivers::gpio::led_b();
        drivers::gpio::HAL_GPIO_TogglePin(led_b.port, led_b.pin);

        let mut input: [u8; 1024] = [0; 1024];
        let mut len = 0;
        let data = "Hello\n\0";
        let usb_serial = uart::usb_uart();

        uart::HAL_UART_Receive(usb_serial, input.as_ptr(), 1, 0xFFFF);

        uart::HAL_UART_Transmit(usb_serial, input.as_ptr(), 1, 0xFFFF);
    }
    return 0;
}
