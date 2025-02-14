idf_component_register(SRCS serial.cc ../../../shared/serial_handler/serial_handler.cc
    INCLUDE_DIRS . ../../../shared_inc/ ../../../shared/
    REQUIRES driver error logger esp_event)
