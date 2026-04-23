#pragma once

#include <cstdint>

enum class CtlToUi : uint16_t
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
    AdjMicPreamp,
    GetAudioMode,
    SetAudioMode,
    AudioFrame,
    AudioStart,
    AudioEnd
};

enum class UiToCtl : uint16_t
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
    AudioMode,
    AudioStart,
    AudioEnd,
    AudioFrame,
};

enum class AudioDirectionMode : uint8_t
{
    Net = 0,
    Mgmt
};

enum struct UiLoopbackMode : uint8_t
{
    Off = 0,
    Raw,
    Alaw,
    Sframe,
    Count,
};
