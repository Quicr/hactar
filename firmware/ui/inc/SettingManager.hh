#pragma once

#include <map>
#include "EEPROM.hh"
#include "Setting.hh"

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

    SettingManager(EEPROM& eeprom) :
        eeprom(eeprom)
    {

    }

    template <typename T>
    void LoadSetting(const SettingAddress setting, T* data)
    {
        // The setting is an address on its own
        unsigned char address = eeprom.Read(setting);

        // Get the length from the address we are going to read from
        unsigned char len = eeprom.Read(address);

        eeprom.Read(address+1, data, len);
    }

    template <typename T>
    void SaveSetting(const SettingAddress setting,
                     T data,
                     const unsigned short sz = 1)
    {
        // Get the address
        unsigned char address = eeprom.ReadByte(setting);

        // The address is unset
        if (address == 0xFF)
        {
            address = eeprom.Write<T>(data, sz);

            // Save the address to the reserved space
            eeprom.Write(setting, address, 1);
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
        unsigned char address = eeprom.ReadByte(setting);

        // The address is unset
        if (address == 0xFF)
        {
            address = eeprom.Write<T>(data, sz);

            // Save the address to the reserved space
            eeprom.Write(setting, address, 1);
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

};