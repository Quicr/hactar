# Build the application's C++ code
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

set(INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32F4xx_HAL_Driver/Inc
  ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32F4xx/Include
  ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/CMSIS/Include
  ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc
  ${CMAKE_CURRENT_SOURCE_DIR}/inc
  ${CMAKE_CURRENT_SOURCE_DIR}/inc/fonts
  ${CMAKE_CURRENT_SOURCE_DIR}/../shared_inc
)

file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc")

add_library(app STATIC ${APP_SOURCES})
target_include_directories(app PUBLIC ${INCLUDE_DIRS})
