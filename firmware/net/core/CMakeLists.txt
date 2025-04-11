set(SOURCE ./src/net.cc ./src/peripherals.cc)

#idf_component_register(SRCS ${SOURCE}
#    INCLUDE_DIRS inc ../../shared_inc/
#    REQUIRES driver serial_esp net_manager logger quicr wifi)

idf_component_register(SRCS ${SOURCE}
    INCLUDE_DIRS inc ../../shared_inc/
    REQUIRES driver serial logger quicr wifi net_manager nlohmann_json pthread)
