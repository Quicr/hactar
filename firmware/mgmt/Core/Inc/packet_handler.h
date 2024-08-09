#ifndef PACKET_HANDLER_H
#define PACKET_HANDLER_H

#include <stdio.h>

#include "app_mgmt.h"
#include "uart_stream.h"

struct packet_t
{
    uint8_t done;
    union
    {
        struct
        {
            uint8_t type;
            uint16_t id;
            uint16_t len;
            char msg[251];
        } __attribute__((packed));
        uint8_t bytes[256];
    };
};

void HandlePackets(uart_stream_t* stream, struct packet_t* packet);
uint32_t GetData(struct packet_t* packet);

#endif