#pragma once

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
};