cmake_minimum_required(VERSION 3.13)

set(SOURCES src/message_buffer.cpp src/encode.cpp src/quicr_client.cpp src/quicr_client_raw_session.cpp src/quicr_name.cpp src/quicr_namespace.cpp)


idf_component_register(SRCS ${SOURCES}
                        INCLUDE_DIRS include
                        REQUIRES qtransport)


#idf_component_register( INCLUDE_DIRS include ../../components/qtransport/include
#                        REQUIRES qtransport)

###
### Build the quicr library
###
#add_subdirectory(src)

#set(QUICR_INSTALL_DIR ../../build/esp-idf/quicr/src)

#add_prebuilt_library(quicr_lib "${QUICR_INSTALL_DIR}/libquicr.a"
#                     PRIV_REQUIRES cxx)

#target_link_libraries(${COMPONENT_LIB} INTERFACE quicr_lib)