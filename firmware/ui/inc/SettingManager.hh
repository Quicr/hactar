#pragma once

#include <map>
#include <any>
#include "EEPROM.hh"
#include "Vector.hh"
#include "main.hh"
#include "Font.hh"

// TODO use
// template <typename T>
// class Setting
// {
// public:
//     const T& data() const
//     {
//         return _data;
//     }

//     T& data()
//     {
//         return _data;
//     }

//     const uint8_t& len() const
//     {
//         return _len;
//     }

//     uint8_t& len()
//     {
//         return _len;
//     }

// private:
//     T* _data;
//     uint8_t _len;
// };

class SettingManager
{
public:
    // Permanent addresses
    // NOTE - this is the order in which the settings are saved to the eeprom
    enum class SettingAddress
    {
        Firstboot,
        Usr_Font,
        Menu_Font,
        Fg,
        Bg,
        Username,
        Password,
        SSID,
        SSID_Password,
    };

    SettingManager(EEPROM& eeprom):
        eeprom(eeprom),
        first_boot(nullptr),
        usr_font(nullptr),
        menu_font(nullptr),
        fg(nullptr),
        bg(nullptr),
        username(nullptr),
        password(nullptr),
        ssid(nullptr),
        ssid_password(nullptr)
    {

    }

    ~SettingManager()
    {
        if (first_boot)
        {
            delete first_boot;
        }

        if (usr_font)
        {
            delete usr_font;
        }

        if (menu_font)
        {
            delete menu_font;
        }

        if (fg)
        {
            delete fg;
        }

        if (bg)
        {
            delete bg;
        }

        if (username)
        {
            delete username;
        }

        if (password)
        {
            delete password;
        }

        if (ssid)
        {
            delete ssid;
        }

        if (ssid_password)
        {
            delete ssid_password;
        }
    }

    const bool* FirstBoot()
    {
        LoadInteger(&first_boot, SettingAddress::Firstboot);
        return first_boot;
    }

    const Font& UserFont()
    {
        LoadInteger(&usr_font, SettingAddress::Usr_Font);
        return GetFont((Fonts)(*usr_font));
    }

    const uint16_t* Fg()
    {
        LoadInteger(&fg, SettingAddress::Fg);
        return fg;
    }

    const uint16_t* Bg()
    {
        LoadInteger(&bg, SettingAddress::Bg);
        return bg;
    }

    const String* Username()
    {
        LoadString(&username, SettingAddress::Username);
        return username;
    }

    const String* Password()
    {
        LoadString(&password, SettingAddress::Password);
        return password;
    }

    const String* SSID()
    {
        LoadString(&ssid, SettingAddress::SSID);
        return ssid;
    }

    const String* SSIDPassword()
    {
        LoadString(&ssid_password, SettingAddress::SSID_Password);
        return ssid_password;
    }

    inline void LoadString(String** str, SettingAddress addr)
    {
        if (*str != nullptr)
        {
            // Was already loaded
            return;
        }

        *str = new String();

        // Load the data
        uint8_t* data;
        int16_t len;

        if (!LoadSetting(addr, &data, len))
        {
            delete* str;
            *str = nullptr;
            return;
        }

        // Copy the data into the the string
        for (int16_t i = 0; i < len; ++i)
        {
            (*str)->push_back((char)data[i]);
        }

        delete [] data;
    }

    template <typename T, typename = typename std::enable_if<std::is_integral<T>::value>::type>
    inline void LoadInteger(T** val, SettingAddress addr)
    {
        if (*val != nullptr)
        {
            // Was already loaded
            return;
        }

        *val = new T();

        // Load the data
        int16_t len;

        if (LoadSetting(addr, &val, len))
        {
            delete* val;
            *val = nullptr;
            return;
        }
    }

    bool LoadSetting(const SettingAddress setting_addr,
        uint8_t** data,
        int16_t& len) const
    {
        // The setting is an address on its own
        int16_t address = eeprom.ReadByte((uint8_t)setting_addr);

        if (address == 255 || address == -1) return false;

        // Get the length from the address we are going to read from
        len = eeprom.ReadByte(address);

        eeprom.Read(address + 1, data, len);

        return true;
    }

    template <typename T>
    bool LoadSetting(const SettingAddress setting,
        T** data,
        int16_t& len) const
    {
        // The setting is an address on its own
        int16_t address = eeprom.ReadByte((uint8_t)setting);

        if (address == 255 || address == -1) return false;

        // Get the length from the address we are going to read from
        len = eeprom.ReadByte(address);

        eeprom.Read(address + 1, data, len);

        return true;
    }

    int16_t ReadSetting(const SettingAddress setting) const
    {
        int16_t address = eeprom.ReadByte((uint8_t)setting);

        if (address == 255) return (int16_t)-1;

        return eeprom.ReadByte(address + 1);
    }

    template <typename T>
    void SaveSetting(const SettingAddress setting,
        T data,
        const uint16_t sz = 1)
    {
        // Get the address
        uint8_t address = eeprom.ReadByte(
            static_cast<uint8_t>(setting));

        // The address is unset
        if (address == 0xFF)
        {
            // Get the next address available and save the data there
            address = eeprom.Write<T>(data, sz);

            // Save the address to the reserved space
            eeprom.WriteByte(static_cast<uint8_t>(setting), address);
        }
        else
        {
            // Get the length of the data currently saved here
            int16_t p_len = eeprom.ReadByte(address);

            // If the new value is greater than the previous
            // we want to make sure not to overwrite other data
            if (sz > p_len)
            {
                ShiftMemory(setting, address, sz);
            }

            // We have written to this address before, so overwrite it
            eeprom.Write(address, data, sz);
        }

        ClearSetting(setting);
    }

    template <typename T>
    void SaveSetting(const SettingAddress setting,
        T* data,
        const uint16_t sz = 1)
    {
        // Get the address
        uint8_t address = eeprom.ReadByte(
            static_cast<uint8_t>(setting));

        // The address is unset
        if (address == 0xFF)
        {
            // Get the next address available and save the data there
            address = eeprom.Write<T>(data, sz);

            // Save the address to the reserved space
            eeprom.WriteByte(static_cast<uint8_t>(setting), address);
        }
        else
        {
            // Get the length of the data currently saved here
            int16_t p_len = eeprom.ReadByte(address);

            // If the new value is greater than the previous
            // we want to make sure not to overwrite other data
            if (sz > p_len)
            {
                ShiftMemory(setting, address, sz);
            }

            // We have written to this address before, so overwrite it
            eeprom.Write(address, data, sz);
        }

        ClearSetting(setting);
    }

    void ShiftMemory(const SettingAddress setting, const int16_t addr, const int16_t len)
    {
        // Stuff the address that can change size into an array
        const SettingAddress dynamic_setting [] = {
            // SettingAddress::Username,
            // SettingAddress::Password,
            SettingAddress::SSID,
            SettingAddress::SSID_Password
        };
        // HAL_GPIO_WritePin(LED_R_Port, LED_R_Pin, GPIO_PIN_RESET);


    // We save the settings in order according to the enum order.

    // Loop over each address
    // uint16_t num_settings = sizeof(dynamic_setting) / sizeof(*dynamic_setting);
    // for (uint16_t i = 0; i < num_settings; ++i)

        int16_t curr_addr = addr;
        int16_t curr_len = len;
        int16_t o_addr = 0;
        int16_t o_len = 0;
        uint8_t* o_data = nullptr;
        for (auto o_setting : dynamic_setting)
        {
            if (setting == o_setting) continue;

            // Get the address
            o_addr =
                eeprom.ReadByte(static_cast<uint8_t>(o_setting));

            o_len = eeprom.ReadByte(
                static_cast<uint8_t>(o_addr));

            // Check if the saved setting is between this setting and
            // the other setting's addresses + len
            if (curr_addr > o_addr + o_len
                || curr_addr + curr_len < o_addr)
                continue;

            if (o_setting == SettingAddress::SSID_Password)
                HAL_GPIO_WritePin(LED_R_Port, LED_R_Pin, GPIO_PIN_RESET);

            // Overlaps, move the next setting and update the
            // curr_addr and curr_len

            // Get the data from the eeprom
            eeprom.Read(o_addr + 1, &o_data, o_len);

            // Write the new address
            eeprom.WriteByte(static_cast<uint8_t>(o_setting),
                static_cast<uint8_t>(1 + curr_addr + curr_len));
            eeprom.Write(2 + curr_addr + curr_len, o_data, o_len);

            // Update the curr_addr and curr_len
            curr_addr = curr_addr + curr_len;
            curr_len = curr_len;
        }
    }

    void ClearEeprom()
    {
        eeprom.Clear();
    }

private:
    void ClearSetting(const SettingAddress setting)
    {
        switch (setting)
        {
            case SettingAddress::Firstboot:
            {
                if (first_boot)
                {
                    delete first_boot;
                    first_boot = nullptr;
                }
                break;
            }
            case SettingAddress::Usr_Font:
            {
                if (usr_font)
                {
                    delete usr_font;
                    usr_font = nullptr;
                }
                break;
            }
            case SettingAddress::Menu_Font:
            {
                if (menu_font)
                {
                    delete menu_font;
                    menu_font = nullptr;
                }
                break;
            }
            case SettingAddress::Fg:
            {
                if (fg)
                {
                    delete fg;
                    fg = nullptr;
                }
                break;
            }
            case SettingAddress::Bg:
            {
                if (bg)
                {
                    delete bg;
                    bg = nullptr;
                }
                break;
            }
            case SettingAddress::Username:
            {
                if (username)
                {
                    delete username;
                    username = nullptr;
                }
                break;
            }
            case SettingAddress::Password:
            {
                if (password)
                {
                    delete password;
                    password = nullptr;
                }
                break;
            }
            case SettingAddress::SSID:
            {
                if (ssid)
                {
                    delete ssid;
                    ssid = nullptr;
                }
                break;
            }
            case SettingAddress::SSID_Password:
            {
                if (ssid_password)
                {
                    delete ssid_password;
                    ssid_password = nullptr;
                }
                break;
            }
            default:
            {
                break;
            }
        }
    }

    EEPROM& eeprom;
    bool* first_boot;
    uint8_t* usr_font;
    uint8_t* menu_font;
    uint16_t* fg;
    uint16_t* bg;
    String* username;
    String* password;
    String* ssid;
    String* ssid_password;
};