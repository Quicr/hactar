#![no_std]
#![no_main]

use core::panic::PanicInfo;
use cortex_m_rt::{entry, exception};
use stm32f4xx_hal::pac::interrupt;

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

/////
///// Main entry point
/////

extern "C" {
    fn stm32_main() -> !;
}

// User main function
#[entry]
fn main() -> ! {
    unsafe { stm32_main() };
}

extern "C" fn _fini() {}

/////
///// Interrupt handlers
/////

// https://docs.rs/cortex-m-rt/0.7.5/cortex_m_rt/attr.exception.html
// https://docs.rs/stm32f4xx-hal/latest/stm32f4xx_hal/enum.interrupt.html

extern "C" {
    fn HAL_IncTick();
}

// The following handlers loop forever (the default behavior):
// * NMI_Handler
// * HardFault_Handler
// * MemManage_Handler
// * BusFault_Handler
// * UsageFault_Handler
//
// XXX(RLB): We might be able to eliminate this default.
#[exception]
unsafe fn DefaultHandler(_irqn: i16) -> ! {
    loop {}
}

// The following handlers return immediately:
// * SVC_Handler
// * DebugMon_Handler
// * PendSV_Handler
#[exception]
fn SVCall() {}

#[exception]
fn DebugMonitor() {}

#[exception]
fn PendSV() {}

// The following handler has actual behavior:
// * SysTick_Handler
#[exception]
fn SysTick() {
    unsafe { HAL_IncTick() };
}

#[interrupt]
fn RCC() {}

#[repr(C)]
pub struct Opaque {
    _data: [u8; 0],
}

extern "C" {
    static hdma_i2s3_ext_rx: Opaque;
    static hdma_usart2_rx: Opaque;
    static hdma_usart2_tx: Opaque;
    static hdma_spi3_tx: Opaque;
    static hdma_spi1_tx: Opaque;
    static hdma_usart1_rx: Opaque;
    static hdma_usart1_tx: Opaque;
    static htim2: Opaque;
    static htim3: Opaque;
    static hspi1: Opaque;
    static hi2s3: Opaque;
    static huart1: Opaque;
    static huart2: Opaque;

    fn HAL_DMA_IRQHandler(ptr: *const Opaque);
    fn HAL_TIM_IRQHandler(ptr: *const Opaque);
    fn HAL_SPI_IRQHandler(ptr: *const Opaque);
    fn HAL_I2S_IRQHandler(ptr: *const Opaque);
    fn HAL_UART_IRQHandler(ptr: *const Opaque);
}

#[interrupt]
fn DMA1_STREAM0() {
    unsafe { HAL_DMA_IRQHandler(&hdma_i2s3_ext_rx) };
}

#[interrupt]
fn DMA1_STREAM5() {
    unsafe { HAL_DMA_IRQHandler(&hdma_usart2_rx) };
}

#[interrupt]
fn DMA1_STREAM6() {
    unsafe { HAL_DMA_IRQHandler(&hdma_usart2_tx) };
}

#[interrupt]
fn DMA1_STREAM7() {
    unsafe { HAL_DMA_IRQHandler(&hdma_spi3_tx) };
}

#[interrupt]
fn DMA2_STREAM3() {
    unsafe { HAL_DMA_IRQHandler(&hdma_spi1_tx) };
}

#[interrupt]
fn DMA2_STREAM5() {
    unsafe { HAL_DMA_IRQHandler(&hdma_usart1_rx) };
}

#[interrupt]
fn DMA2_STREAM7() {
    unsafe { HAL_DMA_IRQHandler(&hdma_usart1_tx) };
}

#[interrupt]
fn TIM2() {
    unsafe { HAL_TIM_IRQHandler(&htim2) };
}

#[interrupt]
fn TIM3() {
    unsafe { HAL_TIM_IRQHandler(&htim3) };
}

#[interrupt]
fn SPI1() {
    unsafe { HAL_SPI_IRQHandler(&hspi1) };
}

#[interrupt]
fn SPI3() {
    unsafe { HAL_I2S_IRQHandler(&hi2s3) };
}

#[interrupt]
fn USART1() {
    unsafe { HAL_UART_IRQHandler(&huart1) };
}

#[interrupt]
fn USART2() {
    unsafe { HAL_UART_IRQHandler(&huart2) };
}
