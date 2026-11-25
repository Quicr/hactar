cmake_minimum_required(VERSION 3.22)
project(ui_host_tests LANGUAGES CXX)

include(FetchContent)
set(DOCTEST_WITH_TESTS OFF)
set(DOCTEST_WITH_MAIN_IN_STATIC_LIB OFF)
set(DOCTEST_NO_INSTALL ON)
FetchContent_Declare(doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG 2d0a9359a60c51affe2a9bebb1be1dca47868151 # v2.5.3
)
FetchContent_MakeAvailable(doctest)

add_executable(ui_tests main.cc)
target_compile_features(ui_tests PRIVATE cxx_std_20)
target_link_libraries(ui_tests PRIVATE doctest::doctest)
