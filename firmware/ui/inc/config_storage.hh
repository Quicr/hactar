#pragma once

#include "app_main.hh"
#include "font.hh"
#include "logger.hh"
#include "m24c02_eeprom.hh"
#include <algorithm>
#include <cstddef>
#include <cstring>

// EEPROM layout (aligned with Link protocol):
// Offset 0:  Version (4 bytes, big-endian u32)
// Offset 16: SFrame Key (16 bytes)
class ConfigStorage
{
public:
    static constexpr uint8_t Version_Address = 0;
    static constexpr uint8_t Version_Size = 4;
    static constexpr uint8_t Sframe_Key_Address = 16;
    static constexpr uint8_t Sframe_Key_Size = 16;

    ConfigStorage(I2C_HandleTypeDef& i2c) :
        eeprom(i2c, 256)
    {
    }

    ~ConfigStorage()
    {
    }

    uint32_t GetVersion()
    {
        uint8_t buf[4];
        if (eeprom.Read(Version_Address, buf, 4) != HAL_OK)
        {
            return 0xFFFFFFFF;
        }
        return (static_cast<uint32_t>(buf[0]) << 24) | (static_cast<uint32_t>(buf[1]) << 16)
             | (static_cast<uint32_t>(buf[2]) << 8) | static_cast<uint32_t>(buf[3]);
    }

    bool SetVersion(uint32_t version)
    {
        uint8_t buf[4];
        buf[0] = static_cast<uint8_t>((version >> 24) & 0xFF);
        buf[1] = static_cast<uint8_t>((version >> 16) & 0xFF);
        buf[2] = static_cast<uint8_t>((version >> 8) & 0xFF);
        buf[3] = static_cast<uint8_t>(version & 0xFF);
        return eeprom.Write(Version_Address, buf, 4) == HAL_OK;
    }

    bool GetSframeKey(uint8_t* key_out)
    {
        return eeprom.Read(Sframe_Key_Address, key_out, Sframe_Key_Size) == HAL_OK;
    }

    bool SetSframeKey(const uint8_t* key, uint16_t len)
    {
        if (len != Sframe_Key_Size)
        {
            return false;
        }
        uint8_t buf[Sframe_Key_Size];
        memcpy(buf, key, Sframe_Key_Size);
        return eeprom.Write(Sframe_Key_Address, buf, Sframe_Key_Size) == HAL_OK;
    }

    void Clear()
    {
        eeprom.Fill(0x00);
    }

    // Legacy compatibility - these match the old API signatures
    enum class Config_Id
    {
        Version,
        Sframe_Key,
        NumConfig
    };

    struct Config
    {
        Config_Id id;
        bool loaded;
        int16_t len;
        uint8_t buff[Sframe_Key_Size];
    };

    Config Load(const Config_Id config_id)
    {
        Config config{Config_Id::NumConfig, false, 0, {0}};
        config.id = config_id;

        switch (config_id)
        {
        case Config_Id::Version:
        {
            if (eeprom.Read(Version_Address, config.buff, Version_Size) == HAL_OK)
            {
                config.loaded = true;
                config.len = Version_Size;
            }
            break;
        }
        case Config_Id::Sframe_Key:
        {
            if (GetSframeKey(config.buff))
            {
                config.loaded = true;
                config.len = Sframe_Key_Size;
            }
            break;
        }
        case Config_Id::NumConfig:
            break;
        }
        return config;
    }

    template <typename T>
    bool Save(const Config_Id config_id, T* data, const uint16_t size)
    {
        switch (config_id)
        {
        case Config_Id::Version:
            if (size != Version_Size)
            {
                return false;
            }
            return SetVersion((static_cast<uint32_t>(reinterpret_cast<uint8_t*>(data)[0]) << 24)
                              | (static_cast<uint32_t>(reinterpret_cast<uint8_t*>(data)[1]) << 16)
                              | (static_cast<uint32_t>(reinterpret_cast<uint8_t*>(data)[2]) << 8)
                              | static_cast<uint32_t>(reinterpret_cast<uint8_t*>(data)[3]));
        case Config_Id::Sframe_Key:
            return SetSframeKey(reinterpret_cast<const uint8_t*>(data), size);
        case Config_Id::NumConfig:
            return false;
        }
        return false;
    }

private:
    M24C02_EEPROM eeprom;
};