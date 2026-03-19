#pragma once

enum Configuration
{
    Version,
    Clear_Storage,
    Set_Ssid,
    Get_Ssid_Names,
    Get_Ssid_Passwords,
    Clear_Ssids,
    Set_Moq_Url,
    Get_Moq_Url,
    Toggle_Logs,
    Disable_Logs,
    Enable_Logs,
    Disable_Loopback,
    Enable_Loopback,
    Set_Language,
    Get_Language,
    Set_Channel,
    Get_Channel,
    Set_AI,
    Get_AI,
    Burn_Disable_USB_JTag_Efuse,

    // Response types (high values to distinguish from commands)
    Response_Ack = 0x8000,
    Response_Nack = 0x8001,
    Response_Data = 0x8002,
};

// Supported language tags for Set_Language validation
// en-US, es-ES, de-DE, hi-IN, nb-NO
