#![no_std]
#![no_main]

use core::panic::PanicInfo;

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

extern "C" {
    fn HAL_Init();
    fn SystemClock_Config();
    fn MX_GPIO_Init();
    fn MX_DMA_Init();
    fn MX_ADC1_Init();
    fn MX_I2C1_Init();
    fn MX_I2S3_Init();
    fn MX_SPI1_Init();
    fn MX_USART1_UART_Init();
    fn MX_USART2_UART_Init();
    fn MX_TIM2_Init();
    fn MX_RNG_Init();
    fn MX_CRC_Init();
    fn MX_TIM3_Init();
    fn app_main();
}

// User main function
#[no_mangle]
extern "C" fn rs_main() -> ! {
    unsafe {
        HAL_Init();
        SystemClock_Config();
        MX_GPIO_Init();
        MX_DMA_Init();
        MX_ADC1_Init();
        MX_I2C1_Init();
        MX_I2S3_Init();
        MX_SPI1_Init();
        MX_USART1_UART_Init();
        MX_USART2_UART_Init();
        MX_TIM2_Init();
        MX_RNG_Init();
        MX_CRC_Init();
        MX_TIM3_Init();
        app_main();
    }

    loop {}
}

// Interrupt handlers
#[repr(C)]
struct Opaque {
    _unused: [u8; 0],
}

extern "C" {
    static hdma_spi3_tx: Opaque;
    static hdma_i2s3_ext_rx: Opaque;
    static hi2s3: Opaque;
    static hdma_spi1_tx: Opaque;
    static hspi1: Opaque;
    static htim2: Opaque;
    static htim3: Opaque;
    static hdma_usart1_rx: Opaque;
    static hdma_usart1_tx: Opaque;
    static hdma_usart2_rx: Opaque;
    static hdma_usart2_tx: Opaque;
    static huart1: Opaque;
    static huart2: Opaque;

    fn HAL_IncTick();
    fn HAL_DMA_IRQHandler(ptr: *const Opaque);
    fn HAL_I2S_IRQHandler(ptr: *const Opaque);
    fn HAL_SPI_IRQHandler(ptr: *const Opaque);
    fn HAL_TIM_IRQHandler(ptr: *const Opaque);
    fn HAL_UART_IRQHandler(ptr: *const Opaque);
}

#[no_mangle]
unsafe extern "C" fn NMI_Handler() {
    loop {}
}

#[no_mangle]
unsafe extern "C" fn HardFault_Handler() {
    loop {}
}

#[no_mangle]
unsafe extern "C" fn MemManage_Handler() {
    loop {}
}

#[no_mangle]
unsafe extern "C" fn BusFault_Handler() {
    loop {}
}

#[no_mangle]
unsafe extern "C" fn UsageFault_Handler() {
    loop {}
}

#[no_mangle]
unsafe extern "C" fn SVC_Handler() {}

#[no_mangle]
unsafe extern "C" fn DebugMon_Handler() {}

#[no_mangle]
unsafe extern "C" fn PendSV_Handler() {}

#[no_mangle]
unsafe extern "C" fn SysTick_Handler() {
    HAL_IncTick();
}

#[no_mangle]
unsafe extern "C" fn RCC_IRQHandler() {}

#[no_mangle]
unsafe extern "C" fn DMA1_Stream0_IRQHandler() {
    HAL_DMA_IRQHandler(&hdma_i2s3_ext_rx);
}

#[no_mangle]
unsafe extern "C" fn DMA1_Stream5_IRQHandler() {
    HAL_DMA_IRQHandler(&hdma_usart2_rx);
}

#[no_mangle]
unsafe extern "C" fn DMA1_Stream6_IRQHandler() {
    HAL_DMA_IRQHandler(&hdma_usart2_tx);
}

#[no_mangle]
unsafe extern "C" fn TIM2_IRQHandler() {
    HAL_TIM_IRQHandler(&htim2);
}

#[no_mangle]
unsafe extern "C" fn TIM3_IRQHandler() {
    HAL_TIM_IRQHandler(&htim3);
}

#[no_mangle]
unsafe extern "C" fn SPI1_IRQHandler() {
    HAL_SPI_IRQHandler(&hspi1);
}

#[no_mangle]
unsafe extern "C" fn USART1_IRQHandler() {
    HAL_UART_IRQHandler(&huart1);
}

#[no_mangle]
unsafe extern "C" fn USART2_IRQHandler() {
    HAL_UART_IRQHandler(&huart2);
}

#[no_mangle]
unsafe extern "C" fn DMA1_Stream7_IRQHandler() {
    HAL_DMA_IRQHandler(&hdma_spi3_tx);
}

#[no_mangle]
unsafe extern "C" fn SPI3_IRQHandler() {
    HAL_I2S_IRQHandler(&hi2s3);
}

#[no_mangle]
unsafe extern "C" fn DMA2_Stream3_IRQHandler() {
    HAL_DMA_IRQHandler(&hdma_spi1_tx);
}

#[no_mangle]
unsafe extern "C" fn DMA2_Stream5_IRQHandler() {
    HAL_DMA_IRQHandler(&hdma_usart1_rx);
}

#[no_mangle]
unsafe extern "C" fn DMA2_Stream7_IRQHandler() {
    HAL_DMA_IRQHandler(&hdma_usart1_tx);
}

//////////
