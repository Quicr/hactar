
set(SOURCES  ./src/test_logger.cpp ./src/hactar-core.cpp )

idf_component_register(SRCS ${SOURCES}
                        INCLUDE_DIRS . include
                        REQUIRES logging qtransport quicr wifi nvs_flash)
