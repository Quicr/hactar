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
    Cmd_Flash_Ui,
    Cmd_Flash_Net,
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
void command_who_are_you(void* arg);
void command_hard_reset(void* arg);
void command_reset(void* arg);
void command_reset_ui(void* arg);
void command_reset_net(void* arg);
void command_flash_ui(void* arg);
void command_flash_net(void* arg);
void command_toggle_logs(void* arg);

#endif