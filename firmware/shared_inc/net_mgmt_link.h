#pragma once

enum Configuration
{
    Version = 0,
    Clear_Storage = 1,
    Set_Ssid = 2,
    Get_Ssid_Names = 3,
    Get_Ssid_Passwords = 4,
    Clear_Ssids = 5,
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
    Response_Data = 0x8002, // Deprecated: use typed responses below

    // Typed responses (self-describing payloads)
    Response_Ssid_Names = 0x8003,     // Comma-separated SSID names
    Response_Ssid_Passwords = 0x8004, // Comma-separated passwords
    Response_Moq_Url = 0x8005,        // UTF-8 URL string
    Response_Language = 0x8006,       // UTF-8 language tag (e.g. "en-US")
    Response_Channel = 0x8007,        // JSON array of namespace parts
    Response_AI_Config = 0x8008,      // JSON: {"query":[...],"audio":[...],"cmd":[...]}
};

// Supported language tags for Set_Language validation
// en-US, es-ES, de-DE, hi-IN, nb-NO
