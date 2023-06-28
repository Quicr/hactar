#pragma once

#include <esp_err.h>
#include <nvs_flash.h>
#include <nvs.h>


namespace hactar_utils {

// Simple class to store and retrieve things from NVS Flash Storage
// TODO: Make it a generic interface
class FlashStorage {

public:

    esp_err_t Open();
};

}