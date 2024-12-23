set(priv_requires
    pthread
    lwip
    wifi
    quicr
    numero_uri
)

idf_component_register(SRCS sub_delegate.cpp pub_delegate.cpp qsession.cpp net_manager.cc
                    INCLUDE_DIRS . ../../../shared_inc/ ../../core/inc
                    PRIV_REQUIRES ${priv_requires}
                    REQUIRES wifi quicr numero_uri)
