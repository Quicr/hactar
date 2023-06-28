set(SOURCES  logging.cpp serial_logger.cpp)
idf_component_register(SRCS ${SOURCES}
                        INCLUDE_DIRS .)
