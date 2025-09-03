/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : Header for main.c file.
 *                   This file contains the common defines of the application.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef APP_MGMT_H
#define APP_MGMT_H

#ifdef __cplusplus
extern "C" {
#endif

#define HIGH GPIO_PIN_SET
#define LOW GPIO_PIN_RESET

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "state.h"
#include "stm32f0xx_hal.h"
#include "uart_stream.h"

static const enum State default_state = Debug;

typedef enum
{
    Hard_Reset,
    Ui_Reset,
    Net_Reset
} Reset_Type;

int app_main(void);

void app_mgmt_reset(const Reset_Type reset_type);

void NetBootloaderMode();
void NetNormalMode();
void NetHoldInReset();
void UIBootloaderMode();
void UINormalMode();
void UIHoldInReset();
void UIHoldInReset();
void NetUpload();
void UIUpload();
void RunningMode();
void DebugMode();
void NormalInit();

void CheckTimeout();
void WaitForNetReady(const enum State* state);
void CancelAllUart();
void SendUploadOk();
void LEDA(GPIO_PinState r, GPIO_PinState g, GPIO_PinState b);
void LEDB(GPIO_PinState r, GPIO_PinState g, GPIO_PinState b);
void TurnOffLEDs();

#ifdef __cplusplus
}
#endif

#endif /* APP_MGMT_H */
