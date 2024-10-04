set(priv_requires
    pthread
    lwip
    wifi
    quicr
    numero_uri
)

set (SOURCES
    hactar_subscribe_track_handler.cpp
    hactar_publish_track_handler.cpp
    net_manager.cc
)

idf_component_register(SRCS ${SOURCES}
                    INCLUDE_DIRS . ../../../shared_inc/ ../../core/inc
                    PRIV_REQUIRES ${priv_requires}
                    REQUIRES wifi quicr numero_uri)
