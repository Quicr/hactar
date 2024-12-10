#![no_std]
#![no_main]

use core::panic::PanicInfo;

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

extern "C" {
    fn main();
}

// User main function
#[no_mangle]
extern "C" fn rs_main() {
    unsafe {
        main();
    }
}
