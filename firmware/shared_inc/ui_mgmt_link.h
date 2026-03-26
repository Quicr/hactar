#pragma once

enum Configuration
{
    Ping, // Replaces Version stub - echoes payload back
    Clear,
    Set_Sframe,
    Get_Sframe,
    Toggle_Logs,
    Disable_Logs,
    Enable_Logs,
    Get_Stack_Info,
    Repaint_Stack,
};