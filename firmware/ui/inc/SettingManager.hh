#pragma once

#include <map>
#include "EEPROM.hh"
#include "Vector.hh"

// TODO use
template <typename T>
class Setting
{
    public:
        const T& data() const
        {
            return _data;
        }

        T& data()
        {
            return _data;
        }

        const unsigned char& len() const
        {
            return _len;
        }

        unsigned char& len()
        {
            return _len;
        }

    private:
        T* _data;
        unsigned char _len;
};

class SettingManager
{
public:
    // Permanent addresses
    enum SettingAddress {
        Firstboot,
        Username,
        Password,
        SSID,
        SSID_Password,
        Usr_Font,
        Fg,
        Bg
    };

    // TODO use
    typedef struct {
        unsigned char* data;
        unsigned char len;
    } setting_t;

    SettingManager(EEPROM& eeprom) :
        eeprom(eeprom),
        settings()
    {

    }

// TODO make the loading of settings all internal
    template <typename T>
    void LoadSetting(const SettingAddress setting,
                     T** data,
                     unsigned char& len) const
    {
        // Check if the setting has been loaded already
        // if (settings.find(setting) != settings.end())
        // {
        //     // Found the setting
        //     return
        // }

        // The setting is an address on its own
        unsigned char address = eeprom.ReadByte(setting); // TODO 255 is error

        // Get the length from the address we are going to read from
        len = eeprom.ReadByte(address);

        eeprom.Read(address+1, data, len);
    }

    unsigned char LoadSetting(const SettingAddress setting) const
    {
        // Check if the setting has been loaded already
        // if (settings.find(setting) != settings.end())
        // {
        //     // Found the setting
        //     return
        // }

        // The setting is an address on its own
        unsigned char address = eeprom.ReadByte(setting); // TODO 255 is error

        return eeprom.ReadByte(address+1);
    }

    template <typename T>
    void SaveSetting(const SettingAddress setting,
                     T data,
                     const unsigned short sz = 1)
    {
        // Get the address
        unsigned char address = eeprom.ReadByte(
            static_cast<unsigned char>(setting));

        // The address is unset
        if (address == 0xFF)
        {
            // Get the next address available and save the data there
            address = eeprom.Write<T>(data, sz);

            // Save the address to the reserved space
            eeprom.WriteByte(static_cast<unsigned char>(setting), address);
        }
        else
        {
            // We have written to this address before, so overwrite it
            eeprom.Write(address, data, sz);
        }
    }

    template <typename T>
    void SaveSetting(const SettingAddress setting,
                     T* data,
                     const unsigned short sz = 1)
    {
        // Get the address
        unsigned char address = eeprom.ReadByte(
            static_cast<unsigned char>(setting));

        // The address is unset
        if (address == 0xFF)
        {
            // Get the next address available and save the data there
            address = eeprom.Write<T>(data, sz);

            // Save the address to the reserved space
            eeprom.WriteByte(static_cast<unsigned char>(setting), address);
        }
        else
        {
            // We have written to this address before, so overwrite it
            eeprom.Write(address, data, sz);
        }
    }

    void ClearEeprom()
    {
        eeprom.Clear();
    }

private:
    EEPROM& eeprom;
    std::map<SettingAddress, setting_t> settings;
};