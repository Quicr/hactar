set(SOURCES Logging.cc SerialLogger.cc)
idf_component_register(SRCS ${SOURCES}
    INCLUDE_DIRS .)
