#include "protector.hh"
#include "app_main.hh"
#include <cmox_crypto.h>
#include <cmox_init.h>
#include <cmox_low_level.h>

Protector::Protector(ConfigStorage& storage) :
    storage(storage),
    mls_ctx(sframe::CipherSuite::AES_GCM_128_SHA256, 1)
{
    if (cmox_initialize(nullptr) != CMOX_INIT_SUCCESS)
    {
        Error("main", "cmox failed to initialise");
    }

// Not currently in use.
#if 0
    ConfigStorage::Config config = storage.Load(ConfigStorage::Config_Id::Sframe_Key);
    if (config.loaded && config.len == 16)
    {
        UI_LOG_INFO("Using stored MLS key!");
        mls_ctx.add_epoch(
            0, sframe::input_bytes{reinterpret_cast<const uint8_t*>(config.buff), config.len});

        return;
    }
    else if (config.len != 16)
    {
        UI_LOG_ERROR("MLS key len malformed %d", (int)config.len);
        Error_Handler("Initialize MLS", "MLS key len malformed");
        return;
    }
#endif

    UI_LOG_WARN("No MLS key stored, using default");
    constexpr const char* mls_key = "sixteen byte key";
    mls_ctx.add_epoch(0, sframe::input_bytes{reinterpret_cast<const uint8_t*>(mls_key), 16});
}

Protector::~Protector()
{
}

bool Protector::TryProtect(link_packet_t* packet) noexcept
try
{
    uint8_t ct[link_packet_t::Payload_Size];
    auto payload = mls_ctx.protect(
        0, 0, ct, sframe::input_bytes{packet->payload, packet->length}.subspan(1), {});

    std::memcpy(packet->payload + 1, payload.data(), payload.size());
    packet->length = payload.size() + 1;
    return true;
}
catch (const std::exception& e)
{
    UI_LOG_ERROR("%s", e.what());
    return false;
}

bool Protector::TryUnprotect(link_packet_t* packet) noexcept
try
{
    auto payload = mls_ctx.unprotect(
        sframe::output_bytes{packet->payload, link_packet_t::Payload_Size}.subspan(1),
        sframe::input_bytes{packet->payload, packet->length}.subspan(1), {});
    packet->length = payload.size() + 1;
    return true;
}
catch (const std::exception& e)
{
    UI_LOG_ERROR("%s", e.what());
    return false;
}

bool Protector::SaveMLSKey()
{
    // TODO future functions that we have not discussed fully yet.
    Error("Protector::SaveMLSKey", "Not implemented");
    return false;
}

bool Protector::LoadMLSKey()
{
    // TODO future functions that we have not discussed fully yet.
    Error("Protector::LoadMLSKey", "Not implemented");
    return false;
}

cmox_init_retval_t cmox_ll_init(void* pArg)
{
    (void)pArg;
    /* Ensure CRC is enabled for cryptographic processing */
    __HAL_RCC_CRC_RELEASE_RESET();
    __HAL_RCC_CRC_CLK_ENABLE();
    return CMOX_INIT_SUCCESS;
}

cmox_init_retval_t cmox_ll_deInit(void* pArg)
{
    (void)pArg;
    /* Do not turn off CRC to avoid side effect on other SW parts using it */
    return CMOX_INIT_SUCCESS;
}
