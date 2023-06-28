set(SOURCES  nvs_utils.cpp)

idf_component_register(SRCS ${SOURCES}
                        INCLUDE_DIRS .
                        REQUIRES nvs_flash)
