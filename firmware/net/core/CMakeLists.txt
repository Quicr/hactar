set(SOURCE ./src/net.cc ./src/Subscriptions.cc)

idf_component_register(SRCS ${SOURCE}
    INCLUDE_DIRS inc ../../shared_inc/
    REQUIRES driver serial_esp net_manager logging quicr wifi)
