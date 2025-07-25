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

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define DISP_RST_Pin GPIO_PIN_13
#define DISP_RST_GPIO_Port GPIOC
#define DISP_BL_Pin GPIO_PIN_14
#define DISP_BL_GPIO_Port GPIOC
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
#define UI_LED_R_Pin GPIO_PIN_4
#define UI_LED_R_GPIO_Port GPIOA
#define SPI1_CLK_Pin GPIO_PIN_5
#define SPI1_CLK_GPIO_Port GPIOA
#define SPI1_MOSI_Pin GPIO_PIN_7
#define SPI1_MOSI_GPIO_Port GPIOA
#define BATTERY_MON_Pin GPIO_PIN_4
#define BATTERY_MON_GPIO_Port GPIOC
#define UI_LED_G_Pin GPIO_PIN_5
#define UI_LED_G_GPIO_Port GPIOC
#define KB_ROW5_Pin GPIO_PIN_0
#define KB_ROW5_GPIO_Port GPIOB
#define KB_ROW6_Pin GPIO_PIN_1
#define KB_ROW6_GPIO_Port GPIOB
#define UI_BOOT1_Pin GPIO_PIN_2
#define UI_BOOT1_GPIO_Port GPIOB
#define KB_ROW7_Pin GPIO_PIN_11
#define KB_ROW7_GPIO_Port GPIOB
#define KB_ROW1_Pin GPIO_PIN_12
#define KB_ROW1_GPIO_Port GPIOB
#define KB_COL1_Pin GPIO_PIN_13
#define KB_COL1_GPIO_Port GPIOB
#define KB_ROW2_Pin GPIO_PIN_14
#define KB_ROW2_GPIO_Port GPIOB
#define KB_COL2_Pin GPIO_PIN_15
#define KB_COL2_GPIO_Port GPIOB
#define KB_COL3_Pin GPIO_PIN_6
#define KB_COL3_GPIO_Port GPIOC
#define KB_COL4_Pin GPIO_PIN_7
#define KB_COL4_GPIO_Port GPIOC
#define KB_ROW3_Pin GPIO_PIN_8
#define KB_ROW3_GPIO_Port GPIOC
#define KB_COL5_Pin GPIO_PIN_9
#define KB_COL5_GPIO_Port GPIOC
#define KB_ROW4_Pin GPIO_PIN_8
#define KB_ROW4_GPIO_Port GPIOA
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
#define MIC_IO_Pin GPIO_PIN_12
#define MIC_IO_GPIO_Port GPIOC
#define UI_DEBUG_2_Pin GPIO_PIN_2
#define UI_DEBUG_2_GPIO_Port GPIOD
#define UI_LED_B_Pin GPIO_PIN_3
#define UI_LED_B_GPIO_Port GPIOB
#define I2S_ADCDAT_Pin GPIO_PIN_4
#define I2S_ADCDAT_GPIO_Port GPIOB
#define I2S_DACDAT_Pin GPIO_PIN_5
#define I2S_DACDAT_GPIO_Port GPIOB
#define UI_SCL1_Pin GPIO_PIN_6
#define UI_SCL1_GPIO_Port GPIOB
#define UI_SDA1_Pin GPIO_PIN_7
#define UI_SDA1_GPIO_Port GPIOB
#define DISP_CS_Pin GPIO_PIN_8
#define DISP_CS_GPIO_Port GPIOB
#define DISP_DC_Pin GPIO_PIN_9
#define DISP_DC_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
