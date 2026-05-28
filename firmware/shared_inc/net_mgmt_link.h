#pragma once

#include <cstdint>

// NET Chip Commands (MGMT to NET)
enum struct CtlToNet : uint16_t
{
    Ping = 0x0040,
    CircularPing = 0x0041,
    AddWifiSsid = 0x0042,
    GetWifiSsids = 0x0043,
    ClearWifiSsids = 0x0044,
    GetRelayUrl = 0x0045,
    SetRelayUrl = 0x0046,
    SetLoopback = 0x0047,
    GetLoopback = 0x0048,
    GetLogsEnabled = 0x0049,
    SetLogsEnabled = 0x004A,
    ClearStorage = 0x004B,
    GetLanguage = 0x004C,
    SetLanguage = 0x004D,
    GetChannel = 0x004E,
    SetChannel = 0x004F,
    GetAi = 0x0050,
    SetAi = 0x0051,
    BurnJtagEfuse = 0x0052,
};

// NET Chip Responses (NET to MGMT)
enum struct NetToCtl : uint16_t
{
    Pong = 0x0050,
    CircularPing = 0x0051,
    WifiSsids = 0x0052,
    RelayUrl = 0x0053,
    Ack = 0x0054,
    Error = 0x0055,
    Loopback = 0x0056,
    LogsEnabled = 0x0057,
    Language = 0x0058,
    Channel = 0x0059,
    Ai = 0x005A,
};

enum struct NetLoopbackMode : uint8_t
{
    Off = 0,
    Raw = 1,
    Moq = 2,
};
