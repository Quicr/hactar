#pragma once

#include "config_state.hh"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "moq_context.hh"
#include "net.hh"
#include "storage.hh"

class MgmtLinkHandler
{
public:
    MgmtLinkHandler(Serial& mgmt_layer,
                    Serial& ui_layer,
                    Wifi& wifi,
                    Storage& storage,
                    ConfigState& config,
                    Diagnostics& diagnostics,
                    MoqContext& moq_context);
    ~MgmtLinkHandler();

    void Begin();

private:
    Serial& serial;
    Serial& ui_serial;
    Wifi& wifi;
    Storage& storage;
    ConfigState& config;
    Diagnostics& diagnostics;
    MoqContext& moq_context;

    TaskHandle_t serial_read_handle;
    StaticTask_t serial_read_buffer;
    StackType_t* serial_read_stack;

    void CreateLinkPacketTask();
    static void LinkPacketTask(void* args);

    void DisableLogging();
    void EnableLogging();
};
