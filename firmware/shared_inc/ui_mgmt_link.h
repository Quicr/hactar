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
    GetAudioTransmitMode,
    SetAudioTransmitMode,
    GetAudioReceiveMode,
    SetAudioReceiveMode,
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
    AudioTransmitMode,
    AudioReceiveMode,
    AudioStart,
    AudioEnd,
    AudioFrame,
    AudioFrameUnprotected,
};

enum class AudioTransmitMode : uint8_t
{
    Net = 0,
    Mgmt,
    Both
};

enum class AudioReceiveMode : uint8_t
{
    None = 0,
    Headphones,
    Ctl,
    Both,
};

enum class UiLoopbackMode : uint8_t
{
    Off = 0,
    Raw,
    Alaw,
    Sframe,
    Count,
};

enum class AudioAdjustDirection : uint8_t
{
    Down = 0,
    Up
};
