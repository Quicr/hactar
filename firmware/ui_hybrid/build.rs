fn main() {
    // Build the C components
    let source_files = [
        "../ui_static/Core/Src/main.c",
        "../ui_static/Core/Src/stm32f4xx_hal_msp.c",
        "../ui_static/Core/Src/stm32f4xx_it.c",
        "../ui_static/Core/Src/system_stm32f4xx.c",
        "../ui_static/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c",
        "../ui_static/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_adc.c",
        "../ui_static/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_adc_ex.c",
        "../ui_static/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c",
        "../ui_static/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c",
        "../ui_static/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma_ex.c",
        "../ui_static/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_exti.c",
        "../ui_static/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash.c",
        "../ui_static/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ex.c",
        "../ui_static/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ramfunc.c",
        "../ui_static/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c",
        "../ui_static/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2c.c",
        "../ui_static/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2s.c",
        "../ui_static/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2s_ex.c",
        "../ui_static/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c",
        "../ui_static/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c",
        "../ui_static/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_spi.c",
        "../ui_static/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim.c",
        "../ui_static/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim_ex.c",
        "../ui_static/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c",
        "../ui_static/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c",
        "../ui_static/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c",
        "../ui_static/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rng.c",
        "../ui_static/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_crc.c",
    ];
    let include_dirs = [
        "../ui_static/Drivers/STM32F4xx_HAL_Driver/Inc",
        "../ui_static/Drivers/CMSIS/Device/ST/STM32F4xx/Include",
        "../ui_static/Drivers/CMSIS/Include",
        "../ui_static/Core/Inc",
        "../ui_static/inc",
        "../ui_static/inc/fonts",
        "../ui_static/../shared_inc",
    ];

    cc::Build::new()
        .define("USE_HAL_DRIVER", None)
        .define("STM32F405xx", None)
        .files(source_files)
        .includes(include_dirs)
        .compile("ui_c");
    println!("cargo:rustc-link-lib=static=ui_c");

    // Apply the proper linker script
    println!("cargo:rustc-link-arg=-T{}", "./etc/STM32F405RGTx_FLASH.ld");
}
