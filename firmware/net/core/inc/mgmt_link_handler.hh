#pragma once

#include "config_state.hh"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
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
                    MoqContext& moq_context,
                    Runtime& runtime_context);
    ~MgmtLinkHandler();

    void Begin();

private:
    struct BlasterTaskContext
    {
        Serial& ui_layer;
        Serial& mgmt_layer;
        MoqContext& moq_context;
        Runtime& runtime;
        Blaster& blaster;
    };

    static constexpr size_t Stack_Size = 8192;

    Serial& serial;
    Serial& ui_serial;
    Wifi& wifi;
    Storage& storage;
    ConfigState& config;
    Diagnostics& diagnostics;
    MoqContext& moq_context;
    Runtime& runtime_context;

    TaskHandle_t serial_read_handle;
    StaticTask_t serial_read_buffer;
    StackType_t* serial_read_stack;
    SemaphoreHandle_t serial_read_semaphore;
    volatile bool serial_read_running;
    BlasterTaskContext blaster_context;

    void CreateLinkPacketTask();
    static void LinkPacketTask(void* args);
    static void BlasterTask(void* arg);

    void DisableLogging();
    void EnableLogging();
};
