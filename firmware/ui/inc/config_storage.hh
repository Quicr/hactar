#pragma once

#include "app_main.hh"
#include "font.hh"
#include "logger.hh"
#include "m24c02_eeprom.hh"
#include <algorithm>
#include <cstddef>
#include <cstring>

#ifndef CONFIG_VERSION
#define CONFIG_VERSION 0
#endif

// TODO split into header and source files
class ConfigStorage
{
public:
    // Permanent addresses
    // NOTE - this is the order in which the settings are saved to the eeprom
    enum class Config_Id
    {
        Version,
        Sframe_Key,
        NumConfig
    };

    static constexpr uint32_t Version_Size = 1;
    static constexpr uint32_t Sframe_Key_Size = 16;

    static constexpr uint32_t Version_Address = 0;

    static constexpr uint32_t Sframe_Key_Address = Version_Address + Version_Size;

    static constexpr uint32_t Largest_Size = std::max({Version_Size, Sframe_Key_Size});

    static constexpr uint32_t Config_Len_Size = 1;
    static constexpr uint32_t Total_Usage =
        Version_Size + Sframe_Key_Size + ((uint32_t)Config_Id::NumConfig - 1) * Config_Len_Size;
    static_assert(Total_Usage < 256);

    static constexpr uint8_t sizes[(size_t)Config_Id::NumConfig] = {
        Version_Size,
        Sframe_Key_Size,
    };
    static constexpr uint8_t addresses[(size_t)Config_Id::NumConfig] = {
        Version_Address,
        Sframe_Key_Address,
    };

    struct Config
    {
        Config_Id id;
        bool loaded;
        int16_t len;
        uint8_t buff[Largest_Size];
    };

    ConfigStorage(I2C_HandleTypeDef& i2c) :
        eeprom(i2c, 256)
    {
        Initialize();
    }

    ~ConfigStorage()
    {
    }

    Config Load(const Config_Id config_id)
    {
        Config config{Config_Id::NumConfig, false, 0, {0}};

        switch (config_id)
        {
        case Config_Id::NumConfig:
        {
            break;
        }
        default:
        {
            // Get the address
            uint8_t address = addresses[static_cast<size_t>(config_id)];
            config.id = config_id;

            // Get the length
            config.len = eeprom.ReadByte(address);

            if (config.len == 255 || config.len == -1)
            {
                config.len = 0;
                config.loaded = false;
            }
            else
            {
                auto res = eeprom.Read(address + 1, config.buff, config.len);

                if (res != HAL_OK)
                {
                    // failed
                    config.loaded = true;
                }
                else
                {
                    config.loaded = true;
                }
            }
            break;
        }
        }
        return config;
    }

    template <typename T>
    bool Save(const Config_Id config, T* data, const uint16_t size)
    {
        if (size > sizes[static_cast<size_t>(config)])
        {
            return false;
        }

        // Get the address
        uint8_t address = addresses[static_cast<size_t>(config)];

        eeprom.Write(address, data, size);

        return true;
    }

    void Clear()
    {
        eeprom.Fill(255);
    }

private:
    void Initialize()
    {
        // Determine if the eeprom is unwritten to
        // If version is a zero or 255 then it is unset.
        int16_t version = eeprom.ReadByte(static_cast<uint8_t>(Version_Address));

        if (version == -1)
        {
            // An error has occured TODO
            return;
        }

        if (version != 255 && version != -1)
        {
            // Already set return
            return;
        }

        if (version != CONFIG_VERSION && version != 255)
        {
            // Different version todo?
            return;
        }

        if (version == CONFIG_VERSION)
        {
            // Same version number
            return;
        }

        // First write the version number
        eeprom.WriteByte(Version_Address, CONFIG_VERSION);

        // Set the length of each config to zero.
        for (size_t i = 0; i < static_cast<size_t>(Config_Id::NumConfig); ++i)
        {
            eeprom.WriteByte(addresses[i], 0);
        }
    }

    M24C02_EEPROM eeprom;
};