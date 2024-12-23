idf_component_register(SRCS "serial.cc"
    INCLUDE_DIRS . ../../../shared_inc/
    REQUIRES driver error logger esp_event)
