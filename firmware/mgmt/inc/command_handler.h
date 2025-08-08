#ifndef COMMAND_HANDLER_HH
#define COMMAND_HANDLER_HH

#include "stm32f0xx_hal.h"

#define WORD_COUNT_MAX 4

typedef struct
{
    const char* pattern;
    uint8_t word_count;
    void (*callback)(
        UART_HandleTypeDef* huart, const char* w1, const char* w2, const char* w3, const char* w4);
} command_t;

typedef struct
{
    const char* w1;
    const char* w2;
    void (*callback)(const char* w1, const char* w2);
} subcommand_t;

int8_t ParseWords(const uint8_t* buff, const size_t len, char words[][32], uint8_t* word_count);
void ProcessCommand(UART_HandleTypeDef* huart, const uint8_t* buff, const size_t len);

void Cmd_Help(
    UART_HandleTypeDef* huart, const char* w1, const char* w2, const char* w3, const char* w4);
void Cmd_Upload(
    UART_HandleTypeDef* huart, const char* w1, const char* w2, const char* w3, const char* w4);
void Cmd_Debug(
    UART_HandleTypeDef* huart, const char* w1, const char* w2, const char* w3, const char* w4);
void Cmd_Reset(
    UART_HandleTypeDef* huart, const char* w1, const char* w2, const char* w3, const char* w4);
void Cmd_Hide_Logs(
    UART_HandleTypeDef* huart, const char* w1, const char* w2, const char* w3, const char* w4);
void Cmd_Show_Logs(
    UART_HandleTypeDef* huart, const char* w1, const char* w2, const char* w3, const char* w4);
void Cmd_Status(
    UART_HandleTypeDef* huart, const char* w1, const char* w2, const char* w3, const char* w4);
void Cmd_Version(
    UART_HandleTypeDef* huart, const char* w1, const char* w2, const char* w3, const char* w4);
void Cmd_Clear(
    UART_HandleTypeDef* huart, const char* w1, const char* w2, const char* w3, const char* w4);
void Cmd_Get(
    UART_HandleTypeDef* huart, const char* w1, const char* w2, const char* w3, const char* w4);
void Cmd_Set(
    UART_HandleTypeDef* huart, const char* w1, const char* w2, const char* w3, const char* w4);
void Cmd_Stop(
    UART_HandleTypeDef* huart, const char* w1, const char* w2, const char* w3, const char* w4);

const command_t commands[] = {{"HELP", 1, Cmd_Help},
                              {"STATUS", 1, Cmd_Status},
                              {"CLEAR", 2, Cmd_Clear},
                              {"DEBUG", 2, Cmd_Debug},
                              {"HIDE", 2, Cmd_Hide_Logs},
                              {"RESET", 2, Cmd_Reset},
                              {"SHOW", 2, Cmd_Show_Logs},
                              {"STOP", 2, Cmd_Stop},
                              {"UPLOAD", 2, Cmd_Upload},
                              {"VERSION", 2, Cmd_Version},
                              {"GET", 3, Cmd_Get},
                              {"SET", 4, Cmd_Set},
                              {NULL, 0, NULL}};

// const

#endif