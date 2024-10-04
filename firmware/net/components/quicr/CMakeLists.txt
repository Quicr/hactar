
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

# target_compile_options(quicr PRIVATE -Wno-error=format)
target_link_libraries(${COMPONENT_LIB} INTERFACE quicr)
