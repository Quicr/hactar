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
    // TODO rethink error codes?
    esp_err_t err = Open(ns);
    if (ESP_OK != err)
    {
        NET_LOG_ERROR("Failed to open fail for ns %s key %s", ns.c_str(), key.c_str());
        return -err;
    }

    size_t size = 0;

    err = nvs_get_blob(handle, key.c_str(), nullptr, &size);

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
    try
    {
        esp_err_t err = ESP_FAIL;
        err = Open(ns);
        if (ESP_OK != err)
        {
            NET_LOG_ERROR("Failed to open fail for ns %s key %s", ns.c_str(), key.c_str());
            return err;
        }

        err = nvs_set_blob(handle, key.c_str(), buff, len);

        if (err != ESP_OK)
        {
            Close();
            return err;
        }

        Close();
        return err;
    }
    catch (std::exception& ex)
    {
        NET_LOG_ERROR("Exception has occured %s", ex.what());
        return ESP_FAIL;
    }
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
    esp_err_t err = Open(ns);
    if (ESP_OK != err)
    {
        NET_LOG_ERROR("Failed to open fail for ns %s key %s", ns.c_str(), key.c_str());
        return err;
    }

    err = nvs_erase_key(handle, key.c_str());
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
    {
        Close();
        return err;
    }

    err = nvs_commit(handle);

    Close();

    NET_LOG_INFO("ns %s cleared key %s", ns.c_str(), key.c_str());
    return err;
}

std::string Storage::LoadStr(const std::string ns, const std::string key)
{
    esp_err_t err = Open(ns);
    if (ESP_OK != err)
    {
        NET_LOG_ERROR("Failed to open fail for ns %s key %s", ns.c_str(), key.c_str());
        return "";
    }
    // Read back the string
    size_t required_size = 0;
    std::string val = "";

    err = nvs_get_str(handle, key.c_str(), NULL, &required_size);
    if (err != ESP_OK)
    {
        Close();
        return val;
    }

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

    Close();

    return val;
}

esp_err_t Storage::SaveStr(const std::string ns, const std::string key, std::string str)
{
    esp_err_t err = Open(ns);
    if (ESP_OK != err)
    {
        NET_LOG_ERROR("Failed to open fail for ns %s key %s", ns.c_str(), key.c_str());
        return err;
    }

    // Store and read a string
    err = nvs_set_str(handle, key.c_str(), str.c_str());
    if (ESP_OK != err)
    {
        NET_LOG_ERROR("Failed to write string!");
    }

    Close();

    return err;
}

uint32_t Storage::Loadu32(const std::string ns, const std::string key)
{
    esp_err_t err = Open(ns);
    if (ESP_OK != err)
    {
        NET_LOG_ERROR("Failed to open fail for ns %s key %s", ns.c_str(), key.c_str());
        return err;
    }

    uint32_t val = 0;

    err = nvs_get_u32(handle, key.c_str(), &val);

    if (err != ESP_OK)
    {
        val = 0;
    }

    Close();

    return val;
}

esp_err_t Storage::Saveu32(const std::string ns, const std::string key, const uint32_t val)
{
    esp_err_t err = Open(ns);
    if (ESP_OK != err)
    {
        NET_LOG_ERROR("Failed to open fail for ns %s key %s", ns.c_str(), key.c_str());
        return err;
    }

    err = nvs_set_u32(handle, key.c_str(), val);
    if (err != ESP_OK)
    {
        NET_LOG_ERROR("Failed to write u32");
    }

    nvs_commit(handle);

    Close();

    return err;
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
    // NET_LOG_INFO("Opening Non-Volatile Storage (NVS) handle...");
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
