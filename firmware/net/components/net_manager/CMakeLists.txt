set(priv_requires
    pthread
    lwip
    wifi
    quicr
    numero_uri
)

idf_component_register(SRCS sub_delegate.cpp pub_delegate.cpp qsession.cpp NetManager.cc
                    INCLUDE_DIRS . ../../../shared_inc/
                    PRIV_REQUIRES ${priv_requires}
                    REQUIRES wifi quicr numero_uri) 
