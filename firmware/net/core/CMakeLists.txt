set(SOURCE ./src/TestLogger.cc ./src/net.cc)

idf_component_register(SRCS ${SOURCE}
    INCLUDE_DIRS .
    REQUIRES driver serial_esp net_manager)
