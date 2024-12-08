cmake_minimum_required(VERSION 3.13)

# CMAKE_TOOLCHAIN_FILE=toolchain.cmake cmake -B build

project(hactar_ui
  VERSION 0.1
  LANGUAGES C CXX
)

# Global compiler config
add_compile_definitions("-DUSE_HAL_DRIVER -DSTM32F405xx")

# STM-provided HAL
set(HAL_SOURCES
  ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c
  ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_adc.c
  ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c
  ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c
  ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma_ex.c
  ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_exti.c
  ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash.c
  ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ex.c
  ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ramfunc.c
  ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c
  ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2c.c
  ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2s.c
  ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2s_ex.c
  ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c
  ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c
  ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_spi.c
  ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim.c
  ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim_ex.c
  ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c
  ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c
  ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c
  ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rng.c
  ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_crc.c
)
add_library(hal ${HAL_SOURCES})
target_include_directories(hal
  PRIVATE
    Drivers/STM32F4xx_HAL_Driver/Inc
    Drivers/STM32F4xx_HAL_Driver/Inc/Legacy
    Drivers/CMSIS/Device/ST/STM32F4xx/Include
    Drivers/CMSIS/Include
    Core/Inc
)

# STM-template-based HAL adaptation layer
set(HAL_ADAPTER_SOURCES
  ./Core/Src/main.c
  ./Core/Src/stm32f4xx_hal_msp.c
  ./Core/Src/stm32f4xx_it.c
  ./Core/Src/system_stm32f4xx.c
)
add_library(hal_adapter ${HAL_ADAPTER_SOURCES})
target_include_directories(hal_adapter
  PRIVATE
    Drivers/STM32F4xx_HAL_Driver/Inc
    # Drivers/STM32F4xx_HAL_Driver/Inc/Legacy
    Drivers/CMSIS/Device/ST/STM32F4xx/Include
    Drivers/CMSIS/Include
    Core/Inc
    inc
)

# Application code
file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc")
add_library(app ${APP_SOURCES})
target_include_directories(app
  PRIVATE
    Drivers/STM32F4xx_HAL_Driver/Inc
    Drivers/CMSIS/Device/ST/STM32F4xx/Include
    Drivers/CMSIS/Include
    Core/Inc
    inc
    inc/fonts
    ../shared_inc
)
