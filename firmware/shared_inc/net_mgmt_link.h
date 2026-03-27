#pragma once

#include <cstdint>

enum Configuration
{
    Version = 0,
    Clear_Storage = 1,
    Add_Wifi = 2,   // JSON: {"ssid":"...","password":"..."}
    Get_Wifi = 3,   // Returns JSON array of wifi credentials
    Clear_Wifi = 4, // Clears all saved WiFi credentials
    Set_Relay_Url = 5,
    Get_Relay_Url = 6,
    Get_Loopback = 7,
    Set_Loopback = 8, // NetLoopbackMode value
    Get_Logs_Enabled = 9,
    Set_Logs_Enabled = 10, // 0=disabled, 1=enabled
    Set_Language = 11,
    Get_Language = 12,
    Set_Channel = 13,
    Get_Channel = 14,
    Set_AI = 15,
    Get_AI = 16,
    Burn_Disable_USB_JTag_Efuse = 17,

    // Response types (high values to distinguish from commands)
    Response_Ack = 0x8000,
    Response_Error = 0x8001,

    // Typed responses (self-describing payloads, matches Link's NetToCtl)
    Response_WifiSsids = 0x8002,   // JSON array: [{"ssid":"...","password":"..."},...]
    Response_RelayUrl = 0x8003,   // UTF-8 URL string
    Response_Loopback = 0x8004,   // 1 byte: NetLoopbackMode
    Response_LogsEnabled = 0x8005, // 1 byte: 0=disabled, 1=enabled
    Response_Language = 0x8006,   // UTF-8 language tag (e.g. "en-US")
    Response_Channel = 0x8007,    // JSON array of namespace parts
    Response_AiConfig = 0x8008,   // JSON: {"query":[...],"audio":[...],"cmd":[...]}
};

// Loopback modes for NET chip (matches Rust NetLoopbackMode)
enum struct NetLoopbackMode : uint8_t
{
    Off = 0, // Normal operation - audio to MoQ, filter self-echo
    Raw = 1, // Local bypass - audio directly back to UI (no MoQ)
    Moq = 2, // MoQ loopback - audio to MoQ, DON'T filter self-echo
};

// Supported language tags for Set_Language validation
// en-US, es-ES, de-DE, hi-IN, nb-NO
