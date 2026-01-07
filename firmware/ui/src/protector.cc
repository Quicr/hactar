#include "protector.hh"
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
