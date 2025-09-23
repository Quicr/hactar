#include "storage.hh"
#include "logger.hh"

Storage::Storage() :
    handle(),
    opened(false)
{
    Initialize();
}

Storage::~Storage()
{
    Close();

    nvs_flash_deinit();
}

ssize_t Storage::Load(const std::string ns, const std::string key, void* buff, const size_t len)
{
    Open(ns);

    if (!opened)
    {

        return -1;
    }

    size_t size = 0;

    esp_err_t err = nvs_get_blob(handle, key.c_str(), nullptr, &size);

    if (err != ESP_OK)
    {
        Close();
        return -2;
    }

    if (size > len)
    {
        Close();
        return -3;
    }

    err = nvs_get_blob(handle, key.c_str(), buff, &size);

    if (err != ESP_OK)
    {
        Close();
        return -err;
    }

    Close();

    return static_cast<ssize_t>(size);
}

esp_err_t
Storage::Save(const std::string ns, const std::string key, const void* buff, const size_t len)
{
    Open(ns);

    NET_LOG_INFO("Opened");

    if (!opened)
    {
        return ESP_ERR_INVALID_STATE;
    }

    NET_LOG_INFO("Save blob at %s len %d data %s", key.c_str(), len, (char*)buff);
    esp_err_t err = nvs_set_blob(handle, key.c_str(), buff, len);
    NET_LOG_INFO("Set blob");

    if (err != ESP_OK)
    {
        Close();
        return err;
    }

    err = nvs_commit(handle);
    NET_LOG_INFO("committed");

    Close();
    NET_LOG_INFO("Closed");

    return err;
}

esp_err_t Storage::Clear(const std::string ns)
{
    if (!opened)
    {
        Close();
    }
    esp_err_t err = nvs_flash_erase();
    if (err != ESP_OK)
    {
        NET_LOG_ERROR("ERROR, failed to erase nvs flash");
        return err;
    }

    err = Initialize();

    return err;
}

esp_err_t Storage::ClearKey(const std::string ns, const std::string key)
{
    Open(ns);

    if (!opened)
    {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = nvs_erase_key(handle, key.c_str());

    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
    {
        Close();
        return err;
    }

    err = nvs_commit(handle);

    Close();

    return err;
}

std::string Storage::LoadStr(const std::string ns, const std::string key)
{
    Open(ns);

    // Read back the string
    size_t required_size = 0;
    std::string val = "";

    NET_LOG_INFO("Reading string from NVS...");
    esp_err_t err = nvs_get_str(handle, key.c_str(), NULL, &required_size);
    if (err == ESP_OK)
    {
        val.resize(required_size);
        err = nvs_get_str(handle, key.c_str(), val.data(), &required_size);
        if (err != ESP_OK)
        {
            val = "";
        }
        else
        {
            NET_LOG_WARN("Read string: %s", val.c_str());
        }
    }

    Close();

    return val;
}

void Storage::SaveStr(const std::string ns, const std::string key, std::string str)
{
    Open(ns);

    // Store and read a string
    NET_LOG_INFO("Writing string to NVS...");
    esp_err_t err = nvs_set_str(handle, key.c_str(), str.c_str());
    if (err != ESP_OK)
    {
        NET_LOG_ERROR("Failed to write string!");
    }

    Close();
}

uint32_t Storage::Loadu32(const std::string ns, const std::string key)
{
    Open(ns);

    uint32_t val = 0;

    esp_err_t err = nvs_get_u32(handle, key.c_str(), &val);
    if (err == ESP_OK)
    {
        NET_LOG_WARN("Read u32 %lld", (uint64_t)val);
    }

    Close();

    return val;
}

void Storage::Saveu32(const std::string ns, const std::string key, const uint32_t val)
{
    Open(ns);

    NET_LOG_INFO("Writing %lld", (uint64_t)val);

    esp_err_t err = nvs_set_u32(handle, key.c_str(), val);
    if (err != ESP_OK)
    {
        NET_LOG_ERROR("Failed to write u32");
    }

    nvs_commit(handle);

    Close();
}

esp_err_t Storage::Initialize()
{
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    return err;
}

esp_err_t Storage::Open(const std::string& ns)
{
    if (opened)
    {

        return ESP_OK;
    }

    // Open NVS handle
    NET_LOG_INFO("Opening Non-Volatile Storage (NVS) handle...");
    esp_err_t err = nvs_open(ns.c_str(), NVS_READWRITE, &handle);
    if (err != ESP_OK)
    {
        NET_LOG_ERROR("Error (%s) opening NVS handle!", esp_err_to_name(err));
        opened = false;
    }

    opened = true;
    return err;
}

void Storage::Close()
{
    if (!opened)
    {

        return;
    }

    // Close NVS handle
    nvs_close(handle);

    opened = false;
}
