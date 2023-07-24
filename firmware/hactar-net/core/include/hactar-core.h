#pragma once

#include "logging.h"
#include "wifi.h"
#include "quicr/quicr_client.h"
#include "quicr/quicr_common.h"

namespace hactar_net {

// Main class for hactar-net 
class HactarApp {
public:
    void setup();
    void run();

private:
    void wifi_monitor();
    // Wifi 
    hactar_utils::Wifi wifi;

    // Logger
    hactar_utils::LogManager* logger = nullptr;

    // QuicR
    quicr::QuicRClient* qclient = nullptr;
    
};

} // namespace for hactar-net