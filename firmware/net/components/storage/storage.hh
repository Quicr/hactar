#pragma once

#include "nvs_flash.h"
#include <string>

class Storage
{
public:
    Storage();
    ~Storage();

    ssize_t Load(const std::string ns, const std::string key, void* buff, const size_t len);
    esp_err_t Save(const std::string ns, const std::string key, const void* buff, const size_t len);
    esp_err_t Clear(const std::string key);
    esp_err_t ClearKey(const std::string ns, const std::string key);

    std::string LoadStr(const std::string ns, const std::string key);
    esp_err_t SaveStr(const std::string ns, const std::string key, std::string str);
    uint32_t Loadu32(const std::string ns, const std::string key);
    esp_err_t Saveu32(const std::string ns, const std::string key, const uint32_t val);

private:
    esp_err_t Initialize();
    esp_err_t Open(const std::string& ns);
    void Close();
    nvs_handle_t handle;
    bool opened;
};
