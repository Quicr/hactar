idf_component_register(SRCS uart.cc ../../../shared/uart_handler/uart_handler.cc
    INCLUDE_DIRS . ../../../shared_inc/ ../../../shared/
    REQUIRES driver error logger esp_event)
