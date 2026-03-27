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
    Get_Logs_Enabled,
    Set_Logs_Enabled,

    // Response types (high values to distinguish from commands)
    Response_Ack = 0x8000,
    Response_Error = 0x8001,

    // Typed responses (self-describing payloads, matches Link's UiToCtl)
    Response_SframeKey = 0x8002,   // 16 bytes: SFrame encryption key
    Response_Loopback = 0x8003,    // 1 byte: UiLoopbackMode
    Response_LogsEnabled = 0x8004, // 1 byte: 0=disabled, 1=enabled
    Response_StackInfo =
        0x8005, // JSON: {"stack_base":...,"stack_top":...,"stack_size":...,"stack_used":...}
};

// Loopback modes for UI chip (matches Rust UiLoopbackMode)
enum struct UiLoopbackMode : uint8_t
{
    Off = 0,    // Normal operation - audio sent to NET
    Raw = 1,    // Before A-law encoding (stereo PCM directly to speaker)
    Alaw = 2,   // After encoding, before SFrame (encode then decode)
    Sframe = 3, // Full encryption round-trip (encode → encrypt → decrypt → decode)
};