#pragma once

#include <cstdint>

// NET Chip Commands (MGMT to NET)
enum struct CtlToNet : uint16_t
{
    Ping = 0x0040,
    CircularPing,
    AddWifiSsid,
    GetWifiSsids,
    ClearWifiSsids,
    GetRelayUrl,
    SetRelayUrl,
    SetLoopback,
    GetLoopback,
    GetLogsEnabled,
    SetLogsEnabled,
    ClearStorage,
    GetLanguage,
    SetLanguage,
    GetChannel,
    SetChannel,
    GetAi,
    SetAi,
    BlasterSend,
    GetBlaster,
    SetBlaster,
    BurnJtagEfuse,
    GetUserId,
    SetUserId,
    GetUserName,
    SetUserName,
};

// NET Chip Responses (NET to MGMT)
enum struct NetToCtl : uint16_t
{
    Pong = 0x0050,
    CircularPing,
    WifiSsids,
    RelayUrl,
    Ack,
    Error,
    Loopback,
    LogsEnabled,
    Language,
    Channel,
    Ai,
    Blaster,
    UserId,
    UserName,
};

enum struct NetLoopbackMode : uint8_t
{
    Off = 0,
    Raw,
    Moq,
};
