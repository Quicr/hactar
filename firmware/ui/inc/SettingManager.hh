// #pragma once

// #include <map>
// #include "EEPROM.hh"
// #include "Setting.hh"

// class SettingManager
// {
// public:
//     SettingManager(EEPROM& eeprom);

//     template <typename T>
//     bool RegisterSetting(const unsigned short id,
//                          const T& data,
//                          const unsigned int sz=1)
//     {
//         if (settings.find(id) != settings.end())
//             return false;

//         // Save to eeprom
//         // address_t address = eeprom.Write<T>(&setting.data(), sz);
//         // settings[id] = address;

//         return true;
//     }

//     bool LoadSetting(const unsigned short id,
//                      unsigned long& data,
//                      const unsigned int sz=1)
//     {
//         if (settings.find(id) == settings.end())
//             return false;

//         eeprom.Read(settings[id], data, sz);
//     }

//     void UpdateSetting(const unsigned short id)
//     {

//     }

//     void ClearSettings()
//     {

//     }

// private:
//     EEPROM& eeprom;
//     // TODO settings should be pointers
//     typedef unsigned short id_t;
//     typedef unsigned short address_t;
//     std::map<id_t, address_t> settings;
// };