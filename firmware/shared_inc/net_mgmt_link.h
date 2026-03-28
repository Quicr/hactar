#pragma once

#include <cstdint>

// NET Chip Commands (MGMT to NET): CtlToNet
// Values aligned with Link protocol
enum NetMgmtCmd : uint16_t
{
    Net_Cmd_Ping = 0x0040,
    Net_Cmd_CircularPing = 0x0041,
    Net_Cmd_AddWifiSsid = 0x0042,
    Net_Cmd_GetWifiSsids = 0x0043,
    Net_Cmd_ClearWifiSsids = 0x0044,
    Net_Cmd_GetRelayUrl = 0x0045,
    Net_Cmd_SetRelayUrl = 0x0046,
    Net_Cmd_SetLoopback = 0x0047,
    Net_Cmd_GetLoopback = 0x0048,
    Net_Cmd_GetLogsEnabled = 0x0049,
    Net_Cmd_SetLogsEnabled = 0x004A,
    Net_Cmd_ClearStorage = 0x004B,
    Net_Cmd_GetLanguage = 0x004C,
    Net_Cmd_SetLanguage = 0x004D,
    Net_Cmd_GetChannel = 0x004E,
    Net_Cmd_SetChannel = 0x004F,
    Net_Cmd_GetAi = 0x0050,
    Net_Cmd_SetAi = 0x0051,
    Net_Cmd_BurnJtagEfuse = 0x0052,
};

// NET Chip Responses (NET to MGMT): NetToCtl
// Values aligned with Link protocol
enum NetMgmtResp : uint16_t
{
    Net_Resp_Pong = 0x0050,
    Net_Resp_CircularPing = 0x0051,
    Net_Resp_WifiSsids = 0x0052,
    Net_Resp_RelayUrl = 0x0053,
    Net_Resp_Ack = 0x0054,
    Net_Resp_Error = 0x0055,
    Net_Resp_Loopback = 0x0056,
    Net_Resp_LogsEnabled = 0x0057,
    Net_Resp_Language = 0x0058,
    Net_Resp_Channel = 0x0059,
    Net_Resp_Ai = 0x005A,
};

// Legacy enum for backwards compatibility during transition
// TODO: Remove once all code migrated to NetMgmtCmd/NetMgmtResp
enum Configuration
{
    Version = Net_Cmd_Ping, // Version maps to Ping for backwards compat
    Clear_Storage = Net_Cmd_ClearStorage,
    Add_Wifi = Net_Cmd_AddWifiSsid,
    Get_Wifi = Net_Cmd_GetWifiSsids,
    Clear_Wifi = Net_Cmd_ClearWifiSsids,
    Set_Relay_Url = Net_Cmd_SetRelayUrl,
    Get_Relay_Url = Net_Cmd_GetRelayUrl,
    Get_Loopback = Net_Cmd_GetLoopback,
    Set_Loopback = Net_Cmd_SetLoopback,
    Get_Logs_Enabled = Net_Cmd_GetLogsEnabled,
    Set_Logs_Enabled = Net_Cmd_SetLogsEnabled,
    Set_Language = Net_Cmd_SetLanguage,
    Get_Language = Net_Cmd_GetLanguage,
    Set_Channel = Net_Cmd_SetChannel,
    Get_Channel = Net_Cmd_GetChannel,
    Set_AI = Net_Cmd_SetAi,
    Get_AI = Net_Cmd_GetAi,
    Burn_Disable_USB_JTag_Efuse = Net_Cmd_BurnJtagEfuse,

    // Response types
    Response_Ack = Net_Resp_Ack,
    Response_Error = Net_Resp_Error,
    Response_WifiSsids = Net_Resp_WifiSsids,
    Response_RelayUrl = Net_Resp_RelayUrl,
    Response_Loopback = Net_Resp_Loopback,
    Response_LogsEnabled = Net_Resp_LogsEnabled,
    Response_Language = Net_Resp_Language,
    Response_Channel = Net_Resp_Channel,
    Response_AiConfig = Net_Resp_Ai,
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
