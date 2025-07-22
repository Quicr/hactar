// #pragma once

// #include "app_main.hh"
// #include "eeprom.hh"
// #include "font.hh"

// // TODO use
// // template <typename T>
// // class Setting
// // {
// // public:
// //     const T& data() const
// //     {
// //         return _data;
// //     }

// //     T& data()
// //     {
// //         return _data;
// //     }

// //     const uint8_t& len() const
// //     {
// //         return _len;
// //     }

// //     uint8_t& len()
// //     {
// //         return _len;
// //     }

// // private:
// //     T* _data;
// //     uint8_t _len;
// // };

// class SettingManager
// {
// public:
//     // Permanent addresses
//     // NOTE - this is the order in which the settings are saved to the eeprom
//     enum class SettingAddress
//     {
//         SSID,
//         SSID_Password,
//         SFrame_Key,
//         MoQ_Relay_URL
//     };

//     SettingManager(M24C02_EEPROM& eeprom) :
//         eeprom(eeprom)
//     {
//     }

//     ~SettingManager()
//     {
//     }

//     bool Load(const SettingAddress setting_addr, uint8_t* data, int16_t& len) const
//     {
//         // The setting is an address on its own
//         int16_t address = eeprom.ReadByte((uint8_t)setting_addr);

//         if (address == 255 || address == -1)
//         {
//             return false;
//         }

//         // Get the length from the address we are going to read from
//         len = eeprom.ReadByte(address);

//         eeprom.Read(address + 1, data, len);

//         return true;
//     }

//     template <typename T>
//     bool Load(const SettingAddress setting, T* data, int16_t& len) const
//     {
//         // The setting is an address on its own
//         int16_t address = eeprom.ReadByte((uint8_t)setting);

//         if (address == 255 || address == -1)
//             return false;

//         // Get the length from the address we are going to read from
//         len = eeprom.ReadByte(address);

//         eeprom.Read(address + 1, data, len);

//         return true;
//     }

//     template <typename T>
//     void Save(const SettingAddress setting, T data, const uint16_t sz = 1)
//     {
//         // Get the address
//         uint8_t address = eeprom.ReadByte(static_cast<uint8_t>(setting));

//         // The address is unset
//         if (address == 0xFF)
//         {
//             // Get the next address available and save the data there
//             address = eeprom.Write<T>(data, sz);

//             // Save the address to the reserved space
//             eeprom.WriteByte(static_cast<uint8_t>(setting), address);
//         }
//         else
//         {
//             // Get the length of the data currently saved here
//             int16_t p_len = eeprom.ReadByte(address);

//             // If the new value is greater than the previous
//             // we want to make sure not to overwrite other data
//             if (sz > p_len)
//             {
//                 ShiftMemory(setting, address, sz);
//             }

//             // We have written to this address before, so overwrite it
//             eeprom.Write(address, data, sz);
//         }
//     }

//     template <typename T>
//     void Save(const SettingAddress setting, T* data, const uint16_t sz = 1)
//     {
//         // Get the address
//         uint8_t address = eeprom.ReadByte(static_cast<uint8_t>(setting));

//         // The address is unset
//         if (address == 0xFF)
//         {
//             // Get the next address available and save the data there
//             address = eeprom.Write<T>(data, sz);

//             // Save the address to the reserved space
//             eeprom.WriteByte(static_cast<uint8_t>(setting), address);
//         }
//         else
//         {
//             // Get the length of the data currently saved here
//             int16_t p_len = eeprom.ReadByte(address);

//             // If the new value is greater than the previous
//             // we want to make sure not to overwrite other data
//             if (sz > p_len)
//             {
//                 ShiftMemory(setting, address, sz);
//             }

//             // We have written to this address before, so overwrite it
//             eeprom.Write(address, data, sz);
//         }
//     }

//     void ClearSettings()
//     {
//         eeprom.Clear();
//     }

// private:
//     M24C02_EEPROM& eeprom;
// };