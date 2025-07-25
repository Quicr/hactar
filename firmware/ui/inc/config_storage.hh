#pragma once

#include "app_main.hh"
#include "font.hh"
#include "logger.hh"
#include "m24c02_eeprom.hh"
#include <cstring>

// TODO split into header and source files
class ConfigStorage
{
public:
    // Permanent addresses
    // NOTE - this is the order in which the settings are saved to the eeprom
    enum class Config_Id
    {
        SSID,
        SSID_Password,
        Sframe_Key,
        Moq_Relay_Url
    };

    static constexpr uint32_t Ssid_Size = 32;
    static constexpr uint32_t Pwd_Size = 64;
    static constexpr uint32_t Sframe_Key_Size = 16;
    static constexpr uint32_t Moq_Relay_Url_Size = 64;

    ConfigStorage(M24C02_EEPROM<256>& eeprom) :
        eeprom(eeprom),
        ssid{0},
        pwd{0},
        sframe_key{0},
        moq_relay_url{0}
    {
    }

    ~ConfigStorage()
    {
    }

    bool LoadAll()
    {
        bool ret;
        bool loaded;
        if (loaded = GetConfig(Config_Id::SSID, (uint8_t**)&ssid, ssid_len))
        {
            UI_LOG_INFO("Loaded SSID");
        }
        else
        {
            UI_LOG_WARN("Failed to load SSID");
        }
        ret = ret && loaded;

        if (loaded = GetConfig(Config_Id::SSID_Password, (uint8_t**)&pwd, pwd_len))
        {
            UI_LOG_INFO("Loaded SSID Password");
        }
        else
        {
            UI_LOG_WARN("Failed to load SSID Password");
        }
        ret = ret && loaded;

        if (loaded = GetConfig(Config_Id::Sframe_Key, (uint8_t**)&sframe_key, sframe_key_len))
        {
            UI_LOG_INFO("Loaded SFrame Key");
        }
        else
        {
            UI_LOG_WARN("Failed to load SFrame Key");
        }
        ret = ret && loaded;

        if (loaded =
                GetConfig(Config_Id::Moq_Relay_Url, (uint8_t**)&moq_relay_url, moq_relay_url_len))
        {
            UI_LOG_INFO("Loaded Moq Relay URL");
        }
        else
        {
            UI_LOG_WARN("Failed to load Moq Relay URL");
        }
        ret = ret && loaded;

        return ret;
    }

    bool GetConfig(Config_Id config, uint8_t** buf_ptr, int16_t& len)
    {
        switch (config)
        {
        case Config_Id::SSID:
            *buf_ptr = ssid;
            len = ssid_len;
            break;
        case Config_Id::SSID_Password:
            *buf_ptr = pwd;
            len = pwd_len;
            break;
        case Config_Id::Sframe_Key:
            *buf_ptr = sframe_key;
            len = sframe_key_len;
            break;
        case Config_Id::Moq_Relay_Url:
            *buf_ptr = moq_relay_url;
            len = moq_relay_url_len;
            break;
        default:
            return false;
        }

        const uint8_t mask = 1 << (uint8_t)config;
        if (!(loaded & mask))
        {
            if (Load(config, *buf_ptr, len))
            {
                loaded |= mask;
            }
            else
            {
                return false;
            }
        }

        return true;
    }

    bool SaveConfig(const Config_Id config, const uint8_t* data, int16_t len)
    {
        uint8_t* buf_ptr = nullptr;
        switch (config)
        {
        case Config_Id::SSID:
            buf_ptr = ssid;
            break;
        case Config_Id::SSID_Password:
            buf_ptr = pwd;
            break;
        case Config_Id::Sframe_Key:
            buf_ptr = sframe_key;
            break;
        case Config_Id::Moq_Relay_Url:
            buf_ptr = moq_relay_url;
            break;
        default:
            return false;
        }

        if (!Save(config, data, len))
        {
            return false;
        }
        memcpy(buf_ptr, data, len);

        return true;
    }

private:
    bool Load(const Config_Id config, uint8_t* data, int16_t& len) const
    {

        // The config is an address on its own
        int16_t address = eeprom.ReadByte((uint8_t)config);

        if (address == 255 || address == -1)
        {
            return false;
        }

        // Get the length from the address we are going to read from
        len = eeprom.ReadByte(address);

        eeprom.Read(address + 1, data, len);

        return true;
    }

    template <typename T>
    bool Load(const Config_Id config, T* data, int16_t& len) const
    {
        // The config is an address on its own
        int16_t address = eeprom.ReadByte((uint8_t)config);

        if (address == 0xFF || address == -1)
        {
            return false;
        }

        // Get the length from the address we are going to read from
        len = eeprom.ReadByte(address);

        eeprom.Read(address + 1, data, len);

        return true;
    }

    template <typename T>
    void Save(const Config_Id config, T data, const uint16_t size)
    {
        // Get the address
        uint8_t address = eeprom.ReadByte(static_cast<uint8_t>(config));

        // The address is unset
        if (address == 0xFF)
        {
            // Get the next address available and save the data there
            address = eeprom.Write<T>(data, size);

            // Save the address to the reserved space
            eeprom.WriteByte(static_cast<uint8_t>(config), address);
        }
        else
        {
            // We have written to this address before, so overwrite it
            eeprom.Write(address, data, size);
        }
    }

    template <typename T>
    bool Save(const Config_Id config, T* data, const uint16_t size)
    {
        if (!VerifySize(config, size))
        {
            return false;
        }

        // Get the address
        uint8_t address = eeprom.ReadByte(static_cast<uint8_t>(config));

        // The address is unset
        if (address == 0xFF)
        {
            // Get the next address available and save the data there
            address = eeprom.Write<T>(data, size);

            // Save the address to the reserved space
            eeprom.WriteByte(static_cast<uint8_t>(config), address);
        }
        else
        {
            // We have written to this address before, so overwrite it
            eeprom.Write(address, data, size);
        }

        return true;
    }

    void Clear()
    {
        eeprom.Fill(0);
        ClearConfig(Config_Id::SSID);
        ClearConfig(Config_Id::SSID_Password);
        ClearConfig(Config_Id::Sframe_Key);
        ClearConfig(Config_Id::Moq_Relay_Url);
    }

    void ClearConfig(const Config_Id config)
    {
        switch (config)
        {
        case Config_Id::SSID:
            memset(ssid, 0, Ssid_Size);
            break;
        case Config_Id::SSID_Password:
            memset(pwd, 0, Pwd_Size);
            break;
        case Config_Id::Sframe_Key:
            memset(sframe_key, 0, Sframe_Key_Size);
            break;
        case Config_Id::Moq_Relay_Url:
            memset(moq_relay_url, 0, Moq_Relay_Url_Size);
            break;
        default:
            break;
        }
    }

    bool VerifySize(const Config_Id config, const uint32_t size)
    {
        switch (config)
        {
        case Config_Id::SSID:
            return size <= Ssid_Size;
        case Config_Id::SSID_Password:
            return size <= Pwd_Size;
        case Config_Id::Sframe_Key:
            return size <= Sframe_Key_Size;
        case Config_Id::Moq_Relay_Url:
            return size <= Moq_Relay_Url_Size;
        default:
            return false;
        }
    }

    M24C02_EEPROM<256>& eeprom;
    uint8_t loaded;
    uint8_t ssid[Ssid_Size];
    int16_t ssid_len;

    uint8_t pwd[Pwd_Size];
    int16_t pwd_len;

    uint8_t sframe_key[Sframe_Key_Size];
    int16_t sframe_key_len;

    uint8_t moq_relay_url[Moq_Relay_Url_Size];
    int16_t moq_relay_url_len;
};