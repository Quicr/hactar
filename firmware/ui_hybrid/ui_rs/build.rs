use std::{env, path::PathBuf};

fn main() {
    // Compile the things that can be in a library into a library.  This will also cause them to
    // get included in the output library of this application, so that they are available to
    // downstream consumers.
    let hal_files = [
        // Base HAL
        "../../ui/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c",
        "../../ui/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_adc.c",
        "../../ui/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c",
        "../../ui/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c",
        "../../ui/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma_ex.c",
        "../../ui/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_exti.c",
        "../../ui/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash.c",
        "../../ui/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ex.c",
        "../../ui/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ramfunc.c",
        "../../ui/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c",
        "../../ui/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2c.c",
        "../../ui/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2s.c",
        "../../ui/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2s_ex.c",
        "../../ui/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c",
        "../../ui/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c",
        "../../ui/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_spi.c",
        "../../ui/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim.c",
        "../../ui/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim_ex.c",
        "../../ui/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c",
        "../../ui/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c",
        "../../ui/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c",
        "../../ui/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rng.c",
        "../../ui/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_crc.c",
        // HAL adaptation
        "../../ui/Core/Src/main.c",
        "../../ui/Core/Src/system_stm32f4xx.c",
        "../../ui/Core/Src/sysmem.c",
        "../../ui/Core/Src/syscalls.c",
        "../../ui/Core/Src/stm32f4xx_it.c",
    ];

    let include_dirs = [
        "../../ui/Drivers/STM32F4xx_HAL_Driver/Inc",
        "../../ui/Drivers/CMSIS/Device/ST/STM32F4xx/Include",
        "../../ui/Drivers/CMSIS/Include",
        "../../ui/Drivers/CMSIS/RTOS2/Include",
        "../../ui/Drivers/CMSIS/Core_A/Include",
        "",
        "../../ui/Core/Inc",
        "../../ui/inc",
        "../../ui/inc/fonts",
        "../../ui/../shared_inc",
    ];

    cc::Build::new()
        .cpp(true)
        .define("USE_HAL_DRIVER", None)
        .define("STM32F405xx", None)
        .includes(include_dirs)
        .files(hal_files)
        .compile("hal");

    // Compile the things that have to be object files into object files
    let direct_files = ["../../ui/Core/Src/stm32f4xx_hal_msp.c"];

    let object_files = cc::Build::new()
        .define("USE_HAL_DRIVER", None)
        .define("STM32F405xx", None)
        .includes(include_dirs)
        .files(direct_files)
        .compile_intermediates();

    // Move the object files to a predictable location
    let output_dir = get_output_path();
    for object in object_files {
        // The file name is something like <hash>-<actual-filename>.o
        let file_name = object.file_name().unwrap().to_str().unwrap().to_owned();
        let (_, file_name) = file_name.split_once("-").unwrap();

        std::fs::copy(object, output_dir.join(file_name)).unwrap();
    }
}

// Ascend until we get to a `build` directory, and then one further, e.g.:
//
// In: target/thumbv7em-none-eabihf/debug/build/ui_rs-89af71c117ed0a82/out
// Out: target/thumbv7em-none-eabihf/debug/
fn get_output_path() -> PathBuf {
    let mut out = PathBuf::from(env::var("OUT_DIR").unwrap());

    loop {
        match out.file_name() {
            Some(name) if name == "build" => {
                break;
            }
            _ => out = out.parent().unwrap().to_path_buf(),
        }
    }

    out.parent().unwrap().to_path_buf()
}
