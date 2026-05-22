#include "ui_link_handler.hh"
#include "net_mgmt_link.h"
#include "ui_net_link.hh"

UiLinkHandler::UiLinkHandler(Serial& ui_layer,
                             Serial& mgmt_layer,
                             MoqContext& moq_context,
                             const Runtime& runtime) :
    ui_layer(ui_layer),
    mgmt_layer(mgmt_layer),
    moq_context(moq_context),
    runtime(runtime)
{
}

void UiLinkHandler::Begin()
{
    CreateLinkPacketTask();
    ui_layer.BeginEventTask(read_handle);
}

void UiLinkHandler::CreateLinkPacketTask()
{
    read_stack =
        (StackType_t*)heap_caps_malloc(stack_size * sizeof(StackType_t), MALLOC_CAP_SPIRAM);
    if (read_stack == NULL)
    {
        NET_LOG_INFO("Failed to allocate stack for ui link packet handler");
        return;
    }
    read_handle = xTaskCreateStatic(LinkPacketTask, "ui link packet handler", stack_size, this, 10,
                                    read_stack, &read_buffer);

    NET_LOG_INFO("Created ui link packet handler PSRAM left %ld",
                 heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}

void UiLinkHandler::LinkPacketTask(void* arg)
{
    UiLinkHandler* handler = static_cast<UiLinkHandler*>(arg);

    NET_LOG_INFO("Start ui link packet task");
    while (true)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        while (auto packet = handler->ui_layer.Read())
        {
            if (packet->type == static_cast<uint16_t>(ui_net_link::UiToNet::CircularPing))
            {
                // Forward to MGMT for circular path: MGMT -> UI -> NET -> MGMT
                handler->mgmt_layer.Reply(
                    static_cast<uint16_t>(NetToCtl::CircularPing),
                    std::span<const uint8_t>(packet->payload.data(), packet->length));
                continue;
            }

            if (packet->type != static_cast<uint16_t>(ui_net_link::UiToNet::AudioFrame))
            {
                NET_LOG_ERROR("Got unexpected packet type %d", (int)packet->type);
                continue;
            }

            uint8_t channel_id = packet->payload[0];
            uint32_t ext_bytes = 1;
            uint32_t length = packet->length;

            // Remove the bytes already read from the payload length (channel_id)
            length -= ext_bytes;

            handler->moq_context.PushAudioFrame(channel_id, packet->payload.data() + 1, length,
                                                handler->runtime.curr_audio_isr_time);
        }
    }
}
