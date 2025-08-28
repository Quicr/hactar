#ifndef COMMAND_HANDLER_HH
#define COMMAND_HANDLER_HH

#include "stm32f0xx_hal.h"

typedef enum
{
    Cmd_Version = 0,
    Cmd_Who_Are_You,
    Cmd_Hard_Reset,
    Cmd_Reset,
    Cmd_Reset_Ui,
    Cmd_Reset_Net,
    Cmd_Toggle_Logs,
    Cmd_Toggle_Logs_Ui,
    Cmd_Toggle_Logs_Net,
    Cmd_Quiet_Logs,
    Cmd_Quiet_Logs_Ui,
    Cmd_Quiet_Logs_Net,
    Cmd_To_Ui,
    Cmd_To_Net,
    Cmd_Loopback,
    Cmd_Count
} Command;

typedef struct
{
    const Command command;
    void (*callback)(void* arg);
    void* usr_arg;
} command_map_t;

void command_get_version(void* arg);

#endif