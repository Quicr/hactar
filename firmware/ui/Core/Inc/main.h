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
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "../inc/stm32.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define UI_DEBUG_1_Pin GPIO_PIN_15
#define UI_DEBUG_1_GPIO_Port GPIOC
#define MCLK_Pin GPIO_PIN_0
#define MCLK_GPIO_Port GPIOH
#define NC_Pin GPIO_PIN_1
#define NC_GPIO_Port GPIOH
#define PTT_BTN_Pin GPIO_PIN_0
#define PTT_BTN_GPIO_Port GPIOC
#define PTT_AI_BTN_Pin GPIO_PIN_1
#define PTT_AI_BTN_GPIO_Port GPIOC
#define UI_READY_Pin GPIO_PIN_2
#define UI_READY_GPIO_Port GPIOC
#define NET_READY_Pin GPIO_PIN_3
#define NET_READY_GPIO_Port GPIOC
#define UI_TX2_NET_Pin GPIO_PIN_2
#define UI_TX2_NET_GPIO_Port GPIOA
#define UI_RX2_NET_Pin GPIO_PIN_3
#define UI_RX2_NET_GPIO_Port GPIOA
#define MIC_IO_Pin GPIO_PIN_4
#define MIC_IO_GPIO_Port GPIOA
#define SPI1_CLK_Pin GPIO_PIN_5
#define SPI1_CLK_GPIO_Port GPIOA
#define SPI1_MOSI_Pin GPIO_PIN_7
#define SPI1_MOSI_GPIO_Port GPIOA
#define UI_BOOT1_Pin GPIO_PIN_2
#define UI_BOOT1_GPIO_Port GPIOB
#define VOLUME_UP_Pin GPIO_PIN_10
#define VOLUME_UP_GPIO_Port GPIOB
#define UI_LED_R_Pin GPIO_PIN_6
#define UI_LED_R_GPIO_Port GPIOC
#define UI_LED_G_Pin GPIO_PIN_7
#define UI_LED_G_GPIO_Port GPIOC
#define UI_LED_B_Pin GPIO_PIN_8
#define UI_LED_B_GPIO_Port GPIOC
#define UI_TX1_MGMT_Pin GPIO_PIN_9
#define UI_TX1_MGMT_GPIO_Port GPIOA
#define UI_RX1_MGMT_Pin GPIO_PIN_10
#define UI_RX1_MGMT_GPIO_Port GPIOA
#define I2S_DACLRC_Pin GPIO_PIN_15
#define I2S_DACLRC_GPIO_Port GPIOA
#define I2S_BCLK_Pin GPIO_PIN_10
#define I2S_BCLK_GPIO_Port GPIOC
#define UI_STAT_Pin GPIO_PIN_11
#define UI_STAT_GPIO_Port GPIOC
#define VOLUME_DOWN_Pin GPIO_PIN_2
#define VOLUME_DOWN_GPIO_Port GPIOD
#define I2S_ADCDAT_Pin GPIO_PIN_4
#define I2S_ADCDAT_GPIO_Port GPIOB
#define I2S_DACDAT_Pin GPIO_PIN_5
#define I2S_DACDAT_GPIO_Port GPIOB
#define UI_SCL1_Pin GPIO_PIN_6
#define UI_SCL1_GPIO_Port GPIOB
#define UI_SDA1_Pin GPIO_PIN_7
#define UI_SDA1_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
