#![no_std]

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {}
}

extern "C" {
    fn stm32_main();
}

#[no_mangle]
extern "C" fn main() {
    unsafe { stm32_main() };
}
