#pragma once

#include "moq_context.hh"
#include "net.hh"
#include "serial.hh"

class UiLinkHandler
{
public:
    UiLinkHandler(Serial& ui_layer,
                  Serial& mgmt_layer,
                  MoqContext& moq_context,
                  const Runtime& runtime);
    void Begin();

private:
    void CreateLinkPacketTask();
    static void LinkPacketTask(void* arg);

    static constexpr size_t stack_size = 16184;

    Serial& ui_layer;
    Serial& mgmt_layer;
    MoqContext& moq_context;
    const Runtime& runtime;

    TaskHandle_t read_handle;
    StaticTask_t read_buffer;
    StackType_t* read_stack;
};
