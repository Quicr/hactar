#pragma once

#include <cstdint>

// UI Chip Commands (MGMT to UI)
enum struct CtlToUi : uint16_t
{
    Ping = 0x0020,
    CircularPing = 0x0021,
    GetVersion = 0x0022,
    SetVersion = 0x0023,
    GetSframeKey = 0x0024,
    SetSframeKey = 0x0025,
    SetLoopback = 0x0026,
    GetLoopback = 0x0027,
    GetStackInfo = 0x0028,
    RepaintStack = 0x0029,
    GetLogsEnabled = 0x002A,
    SetLogsEnabled = 0x002B,
    ClearStorage = 0x002C,
};

// UI Chip Responses (UI to MGMT)
enum struct UiToCtl : uint16_t
{
    Pong = 0x0030,
    CircularPing = 0x0031,
    Version = 0x0032,
    SframeKey = 0x0033,
    Ack = 0x0034,
    Error = 0x0035,
    Loopback = 0x0036,
    Log = 0x0037,
    StackInfo = 0x0038,
    LogsEnabled = 0x0039,
};

enum struct UiLoopbackMode : uint8_t
{
    Off = 0,
    Raw = 1,
    Alaw = 2,
    Sframe = 3,
};
