cmake_minimum_required(VERSION 3.16.3)

set(CMAKE_C_FLAGS -Wno-error=format)
set(CMAKE_CXX_FLAGS -Wno-error=format)
set(CMAKE_CXX_STANDARD 20)

set(EXTRA_COMPONENT_DIRS core components)

set(PLATFORM_ESP_IDF true)
set(USE_MBEDTLS ON)
add_definitions(-DLWIP_IPV6)

set (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/libquicr/cmake)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(net)
