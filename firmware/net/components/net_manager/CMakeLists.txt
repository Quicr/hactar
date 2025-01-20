set(priv_requires
    pthread
    lwip
    wifi
    quicr
)

set (SOURCES
      moq_track_reader.cc
      moq_audio_track_reader.cc
      moq_track_writer.cc
      moq_session.cc
    #net_manager.cc
)

set(CMAKE_CXX_FLAGS "-mlongcalls  -Wno-error=format -Wno-error=pessimizing-move")

idf_component_register(SRCS ${SOURCES}
                    INCLUDE_DIRS . ../../../shared_inc/ ../../core/inc
                    PRIV_REQUIRES ${priv_requires}
                    REQUIRES wifi quicr)
