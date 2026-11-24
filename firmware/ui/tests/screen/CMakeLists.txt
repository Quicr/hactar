cmake_minimum_required(VERSION 3.22)
project(screen_tests LANGUAGES CXX)

enable_testing()
add_executable(screen_test
    screen_test.cc
    ../../src/screen.cc
    ../../src/battery.cc
    ../../src/graphics/graphics.cc
    ../../src/graphics/tile_map.cc)
target_compile_features(screen_test PRIVATE cxx_std_20)
target_include_directories(screen_test PRIVATE
    platform
    ../../inc
    ../../inc/fonts
    ../../inc/graphics
    ../../inc/graphics/icons
    ../../../shared_inc)
add_test(NAME screen_driver_transfer COMMAND screen_test)
