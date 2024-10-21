cmake_minimum_required(VERSION 3.16.3)

set(CMAKE_C_FLAGS "-Wno-error=format -Wno-error=pessimizing-move")
set(CMAKE_CXX_FLAGS "-Wno-error=format -Wno-error=pessimizing-move")
set(CMAKE_CXX_STANDARD 20)

set(EXTRA_COMPONENT_DIRS core components)

set(PLATFORM_ESP_IDF true)
set(USE_MBEDTLS ON)
add_definitions(-DPLATFORM_ESP_IDF)

set (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/libquicr/cmake)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(net)
