#pragma once

#include <cstdint>

// UI Chip Commands (MGMT to UI)
enum struct CtlToUi : uint16_t
{
    Ping = 0x0020,
    CircularPing,
    GetVersion,
    SetVersion,
    GetSframeKey,
    SetSframeKey,
    GetLoopback,
    SetLoopback,
    GetStackInfo,
    RepaintStack,
    GetLogsEnabled,
    SetLogsEnabled,
    ClearStorage,
    GetVolume,
    SetVolume,
    AdjVolume,
    GetMicPreamp,
    SetMicPreamp,
    AdjMicPreamp
};

// UI Chip Responses (UI to MGMT)
enum struct UiToCtl : uint16_t
{
    Pong = 0x0030,
    CircularPing,
    Version,
    SframeKey,
    Ack,
    Error,
    Loopback,
    Log,
    StackInfo,
    LogsEnabled,
    Volume,
    MicPreamp,
};

enum struct UiLoopbackMode : uint8_t
{
    Off = 0,
    Raw,
    Alaw,
    Sframe,
    Count,
};
