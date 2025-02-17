cmake_minimum_required(VERSION 3.13)

project(ui_hybrid
  VERSION 0.1
  LANGUAGES C CXX ASM
)

# We assume the Rust library is built independently, with `cargo build`.
# In principle, it is possible to call cargo from CMake using Corrosion, but it
# Corrosion appears to interact poorly with cross-compilation.  Setting up a
# custom target didn't work cleanly either.
set(UI_RS "${CMAKE_CURRENT_SOURCE_DIR}/ui_rs/target/thumbv7em-none-eabihf/debug/libui_rs.a")
set(ADAPTER_OBJ "${CMAKE_CURRENT_SOURCE_DIR}/ui_rs/target/thumbv7em-none-eabihf/debug/stm32f4xx_hal_msp.o")

# Build application C++ code
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../ui "${CMAKE_CURRENT_BINARY_DIR}/ui")

# Build the firmware
set(FIRMWARE "${CMAKE_PROJECT_NAME}.elf")
add_executable(${FIRMWARE} etc/startup_stm32f405xx.s)
target_link_libraries(${FIRMWARE} ${UI_RS} ${ADAPTER_OBJ} app)
target_link_options(${FIRMWARE} 
  PUBLIC 
  -mcpu=cortex-m4
  -mthumb
  -mfpu=fpv4-sp-d16
  -mfloat-abi=hard
  --specs=rdimon.specs
  -T${CMAKE_CURRENT_SOURCE_DIR}/etc/STM32F405RGTx_FLASH.ld
  -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.map,--cref 
  -Wl,--gc-sections
)
target_link_libraries(${FIRMWARE} c m nosys rdimon)
