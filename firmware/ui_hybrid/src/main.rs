#![no_std]
#![no_main]

use core::panic::PanicInfo;

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

// Provided by the STM device driver
#[no_mangle]
extern "C" fn SystemInit() {
    // noop
}

// Unclear where this is supposed to come from.  It is apparently downstream of the libc
// __init_array function, but does not appear in any of the linked libraries or C code.  Perhaps it
// is inserted in libraries that link against libc, so that once we have such a library, it will
// provide this function.
#[no_mangle]
extern "C" fn _init() {
    // noop
}

// User main function
#[no_mangle]
extern "C" fn main() {
    loop {}
}
