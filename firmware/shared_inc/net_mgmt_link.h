#pragma once

enum Configuration
{
    Version = 0,
    Clear_Storage = 1,
    Add_Wifi = 2,   // JSON: {"ssid":"...","password":"..."}
    Get_Wifi = 3,   // Returns JSON array of wifi credentials
    Clear_Wifi = 4, // Clears all saved WiFi credentials
    Set_Relay_Url = 5,
    Get_Relay_Url = 6,
    Toggle_Logs = 7,
    Disable_Logs = 8,
    Enable_Logs = 9,
    Disable_Loopback = 10,
    Enable_Loopback = 11,
    Set_Language = 12,
    Get_Language = 13,
    Set_Channel = 14,
    Get_Channel = 15,
    Set_AI = 16,
    Get_AI = 17,
    Burn_Disable_USB_JTag_Efuse = 18,

    // Response types (high values to distinguish from commands)
    Response_Ack = 0x8000,
    Response_Nack = 0x8001,
    Response_Data = 0x8002,
};

// Supported language tags for Set_Language validation
// en-US, es-ES, de-DE, hi-IN, nb-NO
