cmake_minimum_required(VERSION 3.24)

set(CMAKE_CXX_STANDARD 20)

set(EXTRA_COMPONENT_DIRS core components)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(hactar-net)