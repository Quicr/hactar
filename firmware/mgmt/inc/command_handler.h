#ifndef COMMAND_HANDLER_HH
#define COMMAND_HANDLER_HH

#include "stm32f0xx_hal.h"
#include <stdint.h>

// TODO Get commands
typedef enum
{
    Cmd_Version = 0,
    Cmd_Who_Are_You,
    Cmd_Hard_Reset,
    Cmd_Reset,
    Cmd_Reset_Ui,
    Cmd_Reset_Net,
    Cmd_Stop_Ui,
    Cmd_Stop_Net,
    Cmd_Flash_Ui,
    Cmd_Flash_Net,
    Cmd_Enable_Logs,
    Cmd_Enable_Logs_Ui,
    Cmd_Enable_Logs_Net,
    Cmd_Disable_Logs,
    Cmd_Disable_Logs_Ui,
    Cmd_Disable_Logs_Net,
    Cmd_Default_Logging,
    Cmd_To_Ui,
    Cmd_To_Net,
    Cmd_Loopback,
    Cmd_Count
} Command;

typedef enum
{
    Ping = 0x0000,
    ToUi,
    ToNet,
    HelloRequest,
    SetPin,
    SetUiBaudrate,
    GetStackInfo,
    RepaintStack,
    CtlToMgmtCnt
} CtlToMgmt;

typedef enum
{
    Pong = 0x0010,
    FromUi,
    FromNet,
    AckReply,
    HelloReply,
    StackInfo,
    MgmtToCtlCnt
} MgmtToCtl;

typedef struct
{
    const Command command;
    void (*callback)(void* arg);
    void* usr_arg;
} command_map_t;

typedef struct
{
    const CtlToMgmt command;
    const uint8_t* data;
    const uint32_t len;
    void (*callback)(void* arg);
    void* usr_arg;
} ctl_to_mgmt_map_t;

typedef enum
{
    UiBoot0,
    UiBoot1,
    UiRst,
    NetBoot,
    NetRst,
} PinId;

void command_handle_packet(const CtlToMgmt command, const uint8_t* data, const uint32_t len);
void command_pong();
void command_to_ui();
void command_to_net();
void command_hello_request(const uint8_t* data, const uint32_t len);
void command_set_pin(const uint8_t* data, const uint32_t len);
void command_set_ui_baudrate(const uint8_t* data, const uint32_t len);

void command_get_version(void* arg);

void command_who_are_you(void* arg);

void command_hard_reset(void* arg);

void command_reset(void* arg);
void command_reset_ui(void* arg);
void command_reset_net(void* arg);

void command_stop_ui(void* arg);
void command_stop_net(void* arg);

void command_flash_ui(void* arg);
void command_flash_net(void* arg);

void command_enable_logs(void* arg);
void command_enable_logs_ui(void* arg);
void command_enable_logs_net(void* arg);

void command_disable_logs(void* arg);
void command_disable_logs_ui(void* arg);
void command_disable_logs_net(void* arg);
void command_default_logging(void* arg);

#endif
