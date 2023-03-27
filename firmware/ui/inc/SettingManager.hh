#pragma once

#include <map>
#include "EEPROM.hh"
#include "Setting.hh"

class SettingManager
{
public:
    SettingManager(EEPROM& eeprom);

    bool RegisterSetting(const unsigned short id, const unsigned long data);
    bool RemoveSetting(const unsigned short id);

    bool GetSetting(const unsigned short id, unsigned long& data);
    bool LoadSettingFromAddress(const unsigned short address);

    void SaveSetting(const unsigned short id);

    void ClearSettings();

private:
    EEPROM& eeprom;
    // TODO settings should be pointers
    std::map<unsigned int, Setting> settings;
};