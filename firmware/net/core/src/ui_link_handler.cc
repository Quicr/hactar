#include "ui_link_handler.hh"
#include "net.hh"
#include "net_mgmt_link.h"
#include "ui_net_link.hh"

UiLinkHandler::UiLinkHandler(Serial& ui_layer,
                             Serial& mgmt_layer,
                             MoqContext& moq_context,
                             const Runtime& runtime,
                             const Diagnostics& diagnostics) :
    ui_layer(ui_layer),
    mgmt_layer(mgmt_layer),
    moq_context(moq_context),
    runtime(runtime),
    diagnostics(diagnostics),
    read_handle(nullptr),
    read_buffer(),
    read_stack(),
    read_semaphore(nullptr),
    read_running(false)
{
}

UiLinkHandler::~UiLinkHandler()
{
    read_running = false;

    if (read_handle)
    {
        // Tell the task to wake up so it can be stopped
        xTaskNotifyGive(read_handle);
        xSemaphoreTake(read_semaphore, portMAX_DELAY);
        read_handle = nullptr;
    }

    if (read_semaphore)
    {
        vSemaphoreDelete(read_semaphore);
        read_semaphore = nullptr;
    }

    if (read_stack)
    {
        heap_caps_free(read_stack);
        read_stack = nullptr;
    }
}

void UiLinkHandler::Begin()
{
    CreateLinkPacketTask();
    ui_layer.BeginEventTask(read_handle);
}

void UiLinkHandler::CreateLinkPacketTask()
{
    NET_LOG_INFO("Creating ui link packet task");

    read_semaphore = xSemaphoreCreateBinary();
    if (read_semaphore == NULL)
    {
        esp_system_abort("Failed to allocate semaphore for mgmt link packet handler");
        return;
    }

    read_stack =
        (StackType_t*)heap_caps_malloc(Stack_Size * sizeof(StackType_t), MALLOC_CAP_INTERNAL);
    if (read_stack == NULL)
    {
        esp_system_abort("Failed to allocate stack for mgmt link packet handler");
        return;
    }

    read_running = true;
    read_handle = xTaskCreateStatic(LinkPacketTask, "mgmt link packet handler", Stack_Size, this,
                                    10, read_stack, &read_buffer);

    NET_LOG_INFO("Created mgmt link packet handler Internal RAM left %ld",
                 heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
}

void UiLinkHandler::LinkPacketTask(void* arg)
{
    UiLinkHandler* handler = static_cast<UiLinkHandler*>(arg);

    NET_LOG_INFO("Start ui link packet task");
    while (handler->read_running)
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

            if (handler->diagnostics.loopback == NetLoopbackMode::Off
                || handler->diagnostics.loopback == NetLoopbackMode::Moq)
            {

                handler->moq_context.PushAudioFrame(channel_id, packet->payload.data() + 1, length,
                                                    handler->runtime.curr_audio_isr_time);
            }
            else if (handler->diagnostics.loopback == NetLoopbackMode::Raw)
            {
                // TODO there is a bug somewhere in this, causing an error in the sync word
                // not sure what exactly or why
                handler->ui_layer.Write(link_packet_t::Sync_Word, sizeof(link_packet_t::Sync_Word));
                const uint16_t type = static_cast<uint16_t>(ui_net_link::NetToUi::AudioFrame);
                handler->ui_layer.Write((uint8_t*)&type, sizeof(type));
                const uint32_t len = length + 1;
                handler->ui_layer.Write((uint8_t*)&len, sizeof(len));
                handler->ui_layer.Write(0); // todo Channel id
                handler->ui_layer.Write(packet->payload.data() + 1, len);
            }
        }
    }
}
