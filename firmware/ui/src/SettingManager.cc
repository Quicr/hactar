#include "SettingManager.hh"

SettingManager::SettingManager(EEPROM& eeprom) :
    eeprom(eeprom)
{

}

// TODO split data into a bunch of bytes
// TODO serialization of data
bool SettingManager::RegisterSetting(const unsigned short id,
                                     const unsigned long data)
{
    if (settings.find(id) != settings.end())
        return false;

    Setting setting(id, data);

    // Add to the map
    settings[id] = setting;

    // Save to eeprom
    unsigned short address = eeprom.Write(&setting.data());
    setting.address() = address;

    return true;
}

void SettingManager::SaveSetting(const unsigned short id)
{
    eeprom.Write(&settings[id].data());
}

bool SettingManager::GetSetting(const unsigned short id, unsigned long& data)
{
    if (settings.find(id) == settings.end())
        return false;

    data = settings[id].data();

    return true;
}

bool SettingManager::LoadSettingFromAddress(const unsigned short address)
{
    // Address
}

void SettingManager::ClearSettings()
{
    settings.clear();

    // Clear the eeprom
    unsigned int max_size = eeprom.Size();
    const unsigned int data_size = max_size / 16;
    unsigned char data[data_size + 1];
    for (unsigned int i = 0; i < data_size + 0; ++i)
    {
        data[i] = 0xFFU;
    }

    for (unsigned int i = 0; i < data_size; ++i)
    {
        eeprom.Write<unsigned char>(data, data_size);
    }
}