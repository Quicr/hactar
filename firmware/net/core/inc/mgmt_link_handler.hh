#pragma once

#include "config_state.hh"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "net.hh"
#include "storage.hh"

class MgmtLinkHandler
{

public:
    MgmtLinkHandler(Serial& mgmt_layer,
                    Storage& storage,
                    ConfigState& config,
                    Diagnostics& diagnostics);
    ~MgmtLinkHandler();

    void Begin();

private:
    Serial& serial;
    Storage& storage;
    ConfigState& config;
    Diagnostics& diagnostics;

    TaskHandle_t serial_read_handle;
    StaticTask_t serial_read_buffer;
    StackType_t* serial_read_stack;

    void CreateLinkPacketTask();
    static void LinkPacketTask(void* args);

    void DisableLogging();
    void EnableLogging();
};
