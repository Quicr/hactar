#pragma once

#include <cstdint>

enum Configuration
{
    Ping, // Replaces Version stub - echoes payload back
    Clear,
    Set_Sframe_Key,
    Get_Sframe_Key,
    Toggle_Logs,
    Disable_Logs,
    Enable_Logs,
    Get_Stack_Info,
    Repaint_Stack,
    Get_Loopback,
    Set_Loopback,
};

// Loopback modes for UI chip (matches Rust UiLoopbackMode)
enum struct UiLoopbackMode : uint8_t
{
    Off = 0,    // Normal operation - audio sent to NET
    Raw = 1,    // Before A-law encoding (stereo PCM directly to speaker)
    Alaw = 2,   // After encoding, before SFrame (encode then decode)
    Sframe = 3, // Full encryption round-trip (encode → encrypt → decrypt → decode)
};