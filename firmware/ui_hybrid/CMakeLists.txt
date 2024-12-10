cmake_minimum_required(VERSION 3.13)

# cmake -B build -DCMAKE_TOOLCHAIN_FILE=etc/toolchain.cmake

project(ui_hybrid
  VERSION 0.1
  LANGUAGES C CXX ASM
)

set(C_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../ui_static")
set(ETC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/etc")

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

# Build the C parts
add_subdirectory(${C_DIR} "${CMAKE_CURRENT_BINARY_DIR}/ui_c")

# Expose the library to Rust
install(TARGETS ui_c)

# # Build the startup script
# add_library(startup OBJECT etc/startup_stm32f405xx.s)
# 
# # Build the Rust parts
# set(UI_RS ${CMAKE_CURRENT_SOURCE_DIR}/target/thumbv7em-none-eabihf/debug/libui_rs.a)
# 
# add_custom_command(OUTPUT  ${UI_RS}
#                    COMMAND cargo build 
#                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
# 
# # Build the firmware
# set(FIRMWARE "${CMAKE_PROJECT_NAME}.elf")
# add_executable(${FIRMWARE} ${ETC_DIR}/noop.c)
# target_link_libraries(${FIRMWARE} ui_c ${UI_RS} startup)
# target_link_options(${FIRMWARE} 
#   PUBLIC 
#   -mcpu=cortex-m4
#   -mthumb
#   -mfpu=fpv4-sp-d16
#   -mfloat-abi=hard
#   --specs=rdimon.specs
#   -T${ETC_DIR}/STM32F405RGTx_FLASH.ld
#   -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.map,--cref 
#   -Wl,--gc-sections
# )
# target_link_libraries(${FIRMWARE} c m nosys rdimon)
# 
# # Upload to the device
# set(WRITE_START "0x08000000")
# add_custom_target(upload
#   COMMAND 
#   STM32_Programmer_CLI -c port=swd freq=2400 mode=normal ap=0 speed=Reliable -w ${CMAKE_CURRENT_BINARY_DIR}/${FIRMWARE} ${WRITE_START} -v -g)
# add_dependencies(upload ${FIRMWARE})
