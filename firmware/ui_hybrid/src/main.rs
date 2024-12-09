#![no_std]
#![no_main]

use core::panic::PanicInfo;

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

/*
// Provided by the STM device driver
#[no_mangle]
extern "C" fn SystemInit() {
    // TODO
}
*/

// XXX(RLB): Unclear where this is supposed to come from.  It is apparently downstream of something
// in libc (specifically fini.c), and is supposed to handle something to do with `.fini` sections
// of the program [1].  I'm leaving this as a stub on the assumption that we don't actually need this
// function to do any work.
//
// [1] https://github.com/eblot/newlib/blob/master/newlib/libc/misc/fini.c
#[no_mangle]
extern "C" fn _fini() {
    // noop
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
