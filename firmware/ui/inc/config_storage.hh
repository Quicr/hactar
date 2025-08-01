#pragma once

#include "app_main.hh"
#include "font.hh"
#include "logger.hh"
#include "m24c02_eeprom.hh"
#include <algorithm>
#include <cstring>

// TODO split into header and source files
class ConfigStorage
{
public:
    // Permanent addresses
    // NOTE - this is the order in which the settings are saved to the eeprom
    enum class Config_Id
    {
        Version,
        SSID_0,
        SSID_Password_0,
        SSID_1,
        SSID_Password_1,
        SSID_2,
        SSID_Password_2,
        Sframe_Key,
        Moq_Relay_Url,
        Ext_Reserved
    };

    static constexpr uint32_t Version_Size = 1;
    static constexpr uint32_t Ssid_Size = 23;
    static constexpr uint32_t Pwd_Size = 32;
    static constexpr uint32_t Sframe_Key_Size = 16;
    static constexpr uint32_t Moq_Relay_Url_Size = 64;

    static constexpr uint32_t Version_Address = 0;

    static constexpr uint32_t SSID_0_Address = Version_Address + Version_Size;
    static constexpr uint32_t SSID_Password_0_Address = SSID_0_Address + Ssid_Size;

    static constexpr uint32_t SSID_1_Address = SSID_Password_0_Address + Pwd_Size;
    static constexpr uint32_t SSID_Password_1_Address = SSID_1_Address + Ssid_Size;

    static constexpr uint32_t SSID_2_Address = SSID_Password_1_Address + Pwd_Size;
    static constexpr uint32_t SSID_Password_2_Address = SSID_2_Address + Ssid_Size;

    static constexpr uint32_t Sframe_Key_Address = SSID_Password_2_Address + Pwd_Size;

    static constexpr uint32_t Moq_Relay_Url_Address = Sframe_Key_Address + Moq_Relay_Url_Size;

    static constexpr uint32_t Largest_Size =
        std::max({Ssid_Size, Pwd_Size, Sframe_Key_Size, Moq_Relay_Url_Size});

    static constexpr uint32_t Config_Len_Size = 1;
    static constexpr uint32_t Total_Usage =
        Version_Size
        + ((Ssid_Size * 3) + (Pwd_Size * 3) + Sframe_Key_Size + Moq_Relay_Url_Size
           + ((uint32_t)Config_Id::Ext_Reserved - 1) * Config_Len_Size);
    static_assert(Total_Usage < 256);

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
    }

    ~ConfigStorage()
    {
    }

    Config LoadConfig(const Config_Id config_id)
    {
        Config config{Config_Id::Ext_Reserved, false, 0, {0}};

        switch (config_id)
        {
        case Config_Id::Version:
        {
            // TODO
            break;
        }
        case Config_Id::Ext_Reserved:
        {
            // TODO
            return config;
        }
        default:
        {
            config.id = config_id;
            config.loaded = Load(config_id, config.buff, config.len);

            return config;
        }
        }
    }

    bool SaveConfig(const Config_Id config_id, const uint8_t* data, int16_t len)
    {
        if (!VerifySize(config_id, len))
        {
            return false;
        }

        return Save(config_id, data, len);
    }

    void Clear()
    {
        eeprom.Fill(255);
    }

private:
    // TODO finish rewriting this
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
        // Get the address
        int16_t address = eeprom.ReadByte(static_cast<uint8_t>(config));

        // The address is unset
        if (address == -1)
        {
            // An error has occurred
            return false;
        }
        else if (address == 0xFF)
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

    bool VerifySize(const Config_Id config, const uint32_t size)
    {
        switch (config)
        {
        case Config_Id::SSID_0:
        case Config_Id::SSID_1:
        case Config_Id::SSID_2:
            return size <= Ssid_Size;
        case Config_Id::SSID_Password_0:
        case Config_Id::SSID_Password_1:
        case Config_Id::SSID_Password_2:
            return size <= Pwd_Size;
        case Config_Id::Sframe_Key:
            return size <= Sframe_Key_Size;
        case Config_Id::Moq_Relay_Url:
            return size <= Moq_Relay_Url_Size;
        default:
            return false;
        }
    }

    M24C02_EEPROM eeprom;
};