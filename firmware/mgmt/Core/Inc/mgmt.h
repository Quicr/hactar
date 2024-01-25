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
#ifndef MGMT_H
#define MGMT_H

#ifdef __cplusplus
extern "C" {
    #endif

    /* Includes ------------------------------------------------------------------*/
    #include "main.h"
    #include "stm32f0xx_hal.h"

    int app_main(void);

    void CancelAllUart();
    void Usart1_Net_Upload_Runnning_Debug_Reset(void);
    void Usart1_UI_Upload_Init(void);
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

    #define HIGH GPIO_PIN_SET
    #define LOW GPIO_PIN_RESET

    #ifdef __cplusplus
}
#endif

#endif /* MGMT_H */
