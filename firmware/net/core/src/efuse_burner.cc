#include "efuse_burner.hh"
#include "esp_efuse.h"
#include "esp_efuse_table.h"
#include "esp_err.h"
#include "logger.hh"

bool BurnDisableUSBJTagEFuse()
{
    const bool is_set = esp_efuse_read_field_bit(ESP_EFUSE_DIS_USB_JTAG);

    if (is_set)
    {
        NET_LOG_INFO("Efuse for DIS_USB_JTAG is already burned");
        return true;
    }
    // TODO check efuse and skip if done
    NET_LOG_INFO("Starting efuse burning process to disable usb jtag debugger");

    esp_err_t err = esp_efuse_batch_write_begin();
    if (err != ESP_OK)
    {
        NET_LOG_ERROR("Failed to start batch write %s", esp_err_to_name(err));
        return false;
    }

    err = esp_efuse_write_field_bit(ESP_EFUSE_DIS_USB_JTAG);
    if (err != ESP_OK)
    {
        NET_LOG_ERROR("Failed to write efuse field %s", esp_err_to_name(err));
        esp_efuse_batch_write_cancel();
        return false;
    }

    err = esp_efuse_batch_write_commit();
    if (err != ESP_OK)
    {
        NET_LOG_ERROR("Failed to commit efuse changes %s", esp_err_to_name(err));
        return false;
    }

    NET_LOG_INFO("Successfully burned efuse to disable usb jtag and enable extern jtag debugging");

    return true;
}
