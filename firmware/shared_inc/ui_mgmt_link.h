#pragma once

#include <cstdint>

// UI Chip Commands (MGMT to UI): CtlToUi
// Values aligned with Link protocol
enum UiMgmtCmd : uint16_t
{
    Ui_Cmd_Ping = 0x0020,
    Ui_Cmd_CircularPing = 0x0021,
    Ui_Cmd_GetVersion = 0x0022,
    Ui_Cmd_SetVersion = 0x0023,
    Ui_Cmd_GetSframeKey = 0x0024,
    Ui_Cmd_SetSframeKey = 0x0025,
    Ui_Cmd_SetLoopback = 0x0026,
    Ui_Cmd_GetLoopback = 0x0027,
    Ui_Cmd_GetStackInfo = 0x0028,
    Ui_Cmd_RepaintStack = 0x0029,
    Ui_Cmd_GetLogsEnabled = 0x002A,
    Ui_Cmd_SetLogsEnabled = 0x002B,
    Ui_Cmd_ClearStorage = 0x002C,
};

// UI Chip Responses (UI to MGMT): UiToCtl
// Values aligned with Link protocol
enum UiMgmtResp : uint16_t
{
    Ui_Resp_Pong = 0x0030,
    Ui_Resp_CircularPing = 0x0031,
    Ui_Resp_Version = 0x0032,
    Ui_Resp_SframeKey = 0x0033,
    Ui_Resp_Ack = 0x0034,
    Ui_Resp_Error = 0x0035,
    Ui_Resp_Loopback = 0x0036,
    Ui_Resp_Log = 0x0037,
    Ui_Resp_StackInfo = 0x0038,
    Ui_Resp_LogsEnabled = 0x0039,
};

// Legacy enum for backwards compatibility during transition
// TODO: Remove once all code migrated to UiMgmtCmd/UiMgmtResp
enum Configuration
{
    Ping = Ui_Cmd_Ping,
    Clear = Ui_Cmd_ClearStorage,
    Set_Sframe_Key = Ui_Cmd_SetSframeKey,
    Get_Sframe_Key = Ui_Cmd_GetSframeKey,
    // Legacy log commands - deprecated in favor of SetLogsEnabled with payload
    Toggle_Logs = 0x1000, // Internal-only, not in Link protocol
    Disable_Logs = 0x1001,
    Enable_Logs = 0x1002,
    Get_Stack_Info = Ui_Cmd_GetStackInfo,
    Repaint_Stack = Ui_Cmd_RepaintStack,
    Get_Loopback = Ui_Cmd_GetLoopback,
    Set_Loopback = Ui_Cmd_SetLoopback,
    Get_Logs_Enabled = Ui_Cmd_GetLogsEnabled,
    Set_Logs_Enabled = Ui_Cmd_SetLogsEnabled,

    // Response types
    Response_Pong = Ui_Resp_Pong,
    Response_SframeKey = Ui_Resp_SframeKey,
    Response_Ack = Ui_Resp_Ack,
    Response_Error = Ui_Resp_Error,
    Response_Loopback = Ui_Resp_Loopback,
    Response_StackInfo = Ui_Resp_StackInfo,
    Response_LogsEnabled = Ui_Resp_LogsEnabled,
};

// Loopback modes for UI chip (matches Rust UiLoopbackMode)
enum struct UiLoopbackMode : uint8_t
{
    Off = 0,    // Normal operation - audio sent to NET
    Raw = 1,    // Before A-law encoding (stereo PCM directly to speaker)
    Alaw = 2,   // After encoding, before SFrame (encode then decode)
    Sframe = 3, // Full encryption round-trip (encode → encrypt → decrypt → decode)
};
