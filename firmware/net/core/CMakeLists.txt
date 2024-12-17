# set(SOURCE ./src/net.cc ./src/subscriptions.cc)

# idf_component_register(SRCS ${SOURCE}
#     INCLUDE_DIRS inc ../../shared_inc/
#     REQUIRES driver serial_esp net_manager logger quicr wifi)

set(SOURCE src/net.cc src/peripherals.cc)
idf_component_register(SRCS ${SOURCE}
    INCLUDE_DIRS inc ../../shared_inc/
    REQUIRES driver nvs_flash esp_event serial)
