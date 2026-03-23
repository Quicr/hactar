
set(priv_requires
    pthread
    lwip
    mbedtls
    )

idf_component_register(
    PRIV_INCLUDE_DIRS
    PRIV_REQUIRES ${priv_requires}
    REQUIRED_IDF_TARGETS esp32s3
)

add_subdirectory(lib)

target_compile_options(picoquic-core PRIVATE -include "${CMAKE_CURRENT_SOURCE_DIR}/ipv6_compat.h")

target_link_libraries(${COMPONENT_LIB} INTERFACE quicr)
