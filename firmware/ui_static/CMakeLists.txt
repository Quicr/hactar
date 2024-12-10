cmake_minimum_required(VERSION 3.13)

# cmake -B build -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake

project(ui_static
  VERSION 0.1
  LANGUAGES C CXX ASM
)

# Global compiler config
add_compile_definitions(-DUSE_HAL_DRIVER -DSTM32F405xx)
add_compile_options(
  -mcpu=cortex-m4
  -mthumb
  -mfpu=fpv4-sp-d16
  -mfloat-abi=hard
  -std=c++17
	-Wall
  -Wpedantic
  -Wextra
	-fdata-sections -ffunction-sections -fexceptions
)
set(OBJCOPY "${TOOLCHAIN_BIN_DIR}/arm-none-eabi-objcopy")

### FIRST TRY: Separate libraries ###
### RESULT: Failure               ###
#
# # STM-provided HAL
# set(HAL_SOURCES
#   ./Core/Src/main.c
#   ./Core/Src/stm32f4xx_hal_msp.c
#   ./Core/Src/stm32f4xx_it.c
#   ./Core/Src/system_stm32f4xx.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_adc.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_adc_ex.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma_ex.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_exti.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ex.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ramfunc.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2c.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2s.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2s_ex.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_spi.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim_ex.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rng.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_crc.c
# )
# add_library(hal ${HAL_SOURCES})
# target_include_directories(hal
#   PUBLIC 
#     Drivers/STM32F4xx_HAL_Driver/Inc
#     Drivers/CMSIS/Device/ST/STM32F4xx/Include
#     Drivers/CMSIS/Include
#     Core/Inc
#     inc
# )
# 
# # Application code
# file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc")
# add_library(app ${APP_SOURCES})
# target_link_libraries(app hal)
# target_include_directories(app
#   PUBLIC
#     inc/fonts
#     ../shared_inc
# )
# 
# # The executable
# set(FIRMWARE "${CMAKE_PROJECT_NAME}.elf")
# add_executable(${FIRMWARE} startup_stm32f405xx.s)
# target_link_options(${FIRMWARE} 
#   PUBLIC 
#   -mcpu=cortex-m4
#   -mthumb
#   -mfpu=fpv4-sp-d16
#   -mfloat-abi=hard
#   --specs=rdimon.specs
#   -T${CMAKE_CURRENT_SOURCE_DIR}/STM32F405RGTx_FLASH.ld
#   -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.map,--cref 
#   -Wl,--gc-sections
# )
# target_link_libraries(${FIRMWARE} hal app)
# 
# # Install the libraries for consumption by Rust
# install(TARGETS hal app)

### THIRD TRY: Split out .s       ###
### RESULT: Failure               ###
# set(HAL_SOURCES
#   ./Core/Src/main.c
#   ./Core/Src/stm32f4xx_hal_msp.c
#   ./Core/Src/stm32f4xx_it.c
#   ./Core/Src/system_stm32f4xx.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_adc.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_adc_ex.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma_ex.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_exti.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ex.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ramfunc.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2c.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2s.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2s_ex.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_spi.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim_ex.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rng.c
#   ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_crc.c
# )
# file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc")
# add_library(hal_and_app ${HAL_SOURCES} ${APP_SOURCES})
# target_include_directories(hal_and_app
#   PUBLIC 
#     Drivers/STM32F4xx_HAL_Driver/Inc
#     Drivers/CMSIS/Device/ST/STM32F4xx/Include
#     Drivers/CMSIS/Include
#     Core/Inc
#     inc
#     inc/fonts
#     ../shared_inc
# )
# 
# set(FIRMWARE "${CMAKE_PROJECT_NAME}.elf")
# add_executable(${FIRMWARE} startup_stm32f405xx.s)
# target_link_libraries(${FIRMWARE} hal_and_app)
# target_link_options(${FIRMWARE} 
#   PUBLIC 
#   -mcpu=cortex-m4
#   -mthumb
#   -mfpu=fpv4-sp-d16
#   -mfloat-abi=hard
#   --specs=rdimon.specs
#   -T${CMAKE_CURRENT_SOURCE_DIR}/STM32F405RGTx_FLASH.ld
#   -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.map,--cref 
#   -Wl,--gc-sections
# )
# target_link_libraries(${FIRMWARE} c m nosys rdimon)

### FOURTH TRY: Object libraries  ###
### RESULT: Success               ###
set(HAL_SOURCES
  ./Core/Src/main.c
  ./Core/Src/stm32f4xx_hal_msp.c
  ./Core/Src/stm32f4xx_it.c
  ./Core/Src/system_stm32f4xx.c
  ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c
  ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_adc.c
  ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_adc_ex.c
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
file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc")
add_library(hal OBJECT ${HAL_SOURCES} ${APP_SOURCES})
target_include_directories(hal
  PUBLIC 
    Drivers/STM32F4xx_HAL_Driver/Inc
    Drivers/CMSIS/Device/ST/STM32F4xx/Include
    Drivers/CMSIS/Include
    Core/Inc
    inc
    inc/fonts
    ../shared_inc
)

add_library(startup OBJECT startup_stm32f405xx.s)

set(FIRMWARE "${CMAKE_PROJECT_NAME}.elf")
add_executable(${FIRMWARE} src/noop.c)
target_link_libraries(${FIRMWARE} hal startup)
target_link_options(${FIRMWARE} 
  PUBLIC 
  -mcpu=cortex-m4
  -mthumb
  -mfpu=fpv4-sp-d16
  -mfloat-abi=hard
  --specs=rdimon.specs
  -T${CMAKE_CURRENT_SOURCE_DIR}/STM32F405RGTx_FLASH.ld
  -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.map,--cref 
  -Wl,--gc-sections
)
target_link_libraries(${FIRMWARE} c m nosys rdimon)

# Upload to the device
set(WRITE_START "0x08000000")
set(PORT "/dev/tty.usbserial-110")
set(BAUD 115200)
add_custom_target(upload
  COMMAND 
  STM32_Programmer_CLI -c port=swd freq=2400 mode=normal ap=0 speed=Reliable -w ${CMAKE_CURRENT_BINARY_DIR}/${FIRMWARE} ${WRITE_START} -v -g)
add_dependencies(upload ${FIRMWARE})
