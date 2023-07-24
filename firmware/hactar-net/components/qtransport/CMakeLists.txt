cmake_minimum_required(VERSION 3.17)

set(SOURCES src/transport.cpp src/transport_udp.cpp)

idf_component_register(SRCS ${SOURCES}
                        INCLUDE_DIRS include src
                        REQUIRES pthread lwip)
