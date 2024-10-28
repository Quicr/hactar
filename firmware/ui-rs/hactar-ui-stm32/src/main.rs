#![no_std]
#![no_main]
#![deny(missing_docs, warnings)]

//! TODO(RLB) documentation

use core::panic::PanicInfo;
use embassy_executor::Spawner;

use hactar_platform_stm32::Stm32Platform;
use hactar_ui::App;

#[embassy_executor::main]
async fn main(_spawner: Spawner) {
    App::new(Stm32Platform::default()).run().await
}

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    // XXX(RLB) This simply halts the thread if a panic occurs.  Standard practice seems to be to
    // print the error somehow if in debug mode, and to do something like this only in release.
    loop {}
}
