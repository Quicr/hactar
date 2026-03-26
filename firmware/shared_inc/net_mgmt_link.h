#pragma once

enum Configuration
{
    Version = 0,
    Clear_Storage = 1,
    Add_Wifi = 2,   // JSON: {"ssid":"...","password":"..."}
    Get_Wifi = 3,   // Returns JSON array of wifi credentials
    Clear_Wifi = 4, // Clears all saved WiFi credentials
    // ID 5 reserved (was Clear_Ssids)
    Set_Moq_Url = 6,
    Get_Moq_Url = 7,
    Toggle_Logs = 8,
    Disable_Logs = 9,
    Enable_Logs = 10,
    Disable_Loopback = 11,
    Enable_Loopback = 12,
    Set_Language = 13,
    Get_Language = 14,
    Set_Channel = 15,
    Get_Channel = 16,
    Set_AI = 17,
    Get_AI = 18,
    Burn_Disable_USB_JTag_Efuse = 19,

    // Response types (high values to distinguish from commands)
    Response_Ack = 0x8000,
    Response_Nack = 0x8001,
    Response_Data = 0x8002,
};

// Supported language tags for Set_Language validation
// en-US, es-ES, de-DE, hi-IN, nb-NO
