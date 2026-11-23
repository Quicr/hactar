cmake_minimum_required(VERSION 3.22)
project(hactar_emulator LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_SYSTEM_NAME MATCHES "^(Linux|Darwin)$")
    message(FATAL_ERROR "The emulator currently supports Linux and macOS.")
endif()

option(HACTAR_FETCH_SDL2 "Download and build a pinned SDL2 if it is not installed" OFF)
find_package(SDL2 2.0.18 CONFIG QUIET)
if(TARGET SDL2::SDL2)
    set(HACTAR_SDL_TARGET SDL2::SDL2)
elseif(TARGET SDL2::SDL2-static)
    set(HACTAR_SDL_TARGET SDL2::SDL2-static)
else()
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(SDL2 QUIET IMPORTED_TARGET sdl2>=2.0.18)
    endif()
    if(TARGET PkgConfig::SDL2)
        set(HACTAR_SDL_TARGET PkgConfig::SDL2)
    elseif(HACTAR_FETCH_SDL2)
        include(FetchContent)
        set(SDL_SHARED OFF CACHE BOOL "" FORCE)
        set(SDL_STATIC ON CACHE BOOL "" FORCE)
        set(SDL_TEST OFF CACHE BOOL "" FORCE)
        set(SDL_TESTS OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(sdl2
            GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
            # SDL 2.32.10, pinned to the release commit.
            GIT_TAG 5d249570393f7a37e037abf22cd6012a4cc56a71
        )
        FetchContent_MakeAvailable(sdl2)
        set(HACTAR_SDL_TARGET SDL2::SDL2-static)
    else()
        message(FATAL_ERROR
            "SDL2 development files are required. Install libsdl2-dev (Linux) or "
            "brew install sdl2 (macOS), or configure with -DHACTAR_FETCH_SDL2=ON.")
    endif()
endif()

set(UI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../firmware/ui")
add_library(hactar_emulator_core
    src/error.cc
    src/sdl_display_driver.cc
    src/ui.cc
    "${UI_DIR}/src/battery.cc"
    "${UI_DIR}/src/renderer.cc"
    "${UI_DIR}/src/screen.cc"
    "${UI_DIR}/src/graphics/graphics.cc"
    "${UI_DIR}/src/graphics/tile_map.cc"
)
target_include_directories(hactar_emulator_core PUBLIC
    include
    "${UI_DIR}/inc"
    "${UI_DIR}/inc/fonts"
    "${UI_DIR}/inc/graphics"
    "${UI_DIR}/inc/graphics/icons"
    "${UI_DIR}/../shared_inc"
)
target_compile_definitions(hactar_emulator_core PUBLIC SDL_MAIN_HANDLED)
target_compile_options(hactar_emulator_core PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(hactar_emulator_core PUBLIC ${HACTAR_SDL_TARGET})

add_executable(hactar-ui src/main.cc)
target_compile_options(hactar-ui PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(hactar-ui PRIVATE hactar_emulator_core)

include(CTest)
if(BUILD_TESTING)
    add_executable(display_test tests/display_test.cc)
    target_compile_options(display_test PRIVATE -Wall -Wextra -Wpedantic)
    target_link_libraries(display_test PRIVATE hactar_emulator_core)
    add_test(NAME display_pipeline
        COMMAND display_test "${CMAKE_CURRENT_BINARY_DIR}/display-test.bmp")
    set_tests_properties(display_pipeline PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy"
        TIMEOUT 15
    )
    add_test(NAME headless_startup COMMAND hactar-ui --headless --view startup --frames 2)
    add_test(NAME headless_chat COMMAND hactar-ui --headless --view chat --frames 2)
    add_test(NAME headless_menu COMMAND hactar-ui --headless --view menu --frames 2)
    set_tests_properties(headless_startup headless_chat headless_menu PROPERTIES TIMEOUT 15)
endif()
