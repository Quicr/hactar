#pragma once

#include <cstdint>

// UI Chip Commands (MGMT to UI): CtlToUi
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

enum struct UiLoopbackMode : uint8_t
{
    Off = 0,
    Raw = 1,
    Alaw = 2,
    Sframe = 3,
};
