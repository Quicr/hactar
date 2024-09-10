set(SOURCES  wifi.cc)
idf_component_register(SRCS ${SOURCES}
                        INCLUDE_DIRS . ../../../shared_inc/
                        REQUIRES esp_wifi nvs_flash esp_event esp_netif logger)
