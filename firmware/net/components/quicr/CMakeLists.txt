
set(priv_requires
    pthread
    lwip
    )

idf_component_register(
    PRIV_INCLUDE_DIRS
    PRIV_REQUIRES ${priv_requires}
    REQUIRED_IDF_TARGETS esp32s3
)

add_subdirectory(lib)
target_include_directories(quicr-transport PRIVATE ${IDF_PATH}/components/pthread/include)
target_link_libraries(${COMPONENT_LIB} INTERFACE quicr)
