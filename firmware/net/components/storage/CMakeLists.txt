set(SOURCES storage.cc)
idf_component_register(SRCS ${SOURCES}
                        INCLUDE_DIRS . ../../../shared_inc/
                        REQUIRES nvs_flash esp_event logger)
