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
#include "stm32f0xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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
#define USB_CC2_DETECT_Pin GPIO_PIN_14
#define USB_CC2_DETECT_GPIO_Port GPIOC
#define USB_CC1_DETECT_Pin GPIO_PIN_15
#define USB_CC1_DETECT_GPIO_Port GPIOC
#define CTS_NOT_NEEDED_Pin GPIO_PIN_0
#define CTS_NOT_NEEDED_GPIO_Port GPIOA
#define RTS_NOT_NEEDED_Pin GPIO_PIN_1
#define RTS_NOT_NEEDED_GPIO_Port GPIOA
#define UI_RX1_MGMT_Pin GPIO_PIN_2
#define UI_RX1_MGMT_GPIO_Port GPIOA
#define UI_TX1_MGMT_Pin GPIO_PIN_3
#define UI_TX1_MGMT_GPIO_Port GPIOA
#define LEDA_R_Pin GPIO_PIN_4
#define LEDA_R_GPIO_Port GPIOA
#define MGMT_DEBUG_1_Pin GPIO_PIN_5
#define MGMT_DEBUG_1_GPIO_Port GPIOA
#define LEDA_G_Pin GPIO_PIN_6
#define LEDA_G_GPIO_Port GPIOA
#define LEDA_B_Pin GPIO_PIN_7
#define LEDA_B_GPIO_Port GPIOA
#define LEDB_R_Pin GPIO_PIN_0
#define LEDB_R_GPIO_Port GPIOB
#define USB_DTR_MGMT_Pin GPIO_PIN_2
#define USB_DTR_MGMT_GPIO_Port GPIOB
#define NET_RX1_MGMT_Pin GPIO_PIN_10
#define NET_RX1_MGMT_GPIO_Port GPIOB
#define NET_TX1_MGMT_Pin GPIO_PIN_11
#define NET_TX1_MGMT_GPIO_Port GPIOB
#define MGMT_DEBUG_2_Pin GPIO_PIN_12
#define MGMT_DEBUG_2_GPIO_Port GPIOB
#define LEDB_B_Pin GPIO_PIN_15
#define LEDB_B_GPIO_Port GPIOB
#define MCLK_Pin GPIO_PIN_8
#define MCLK_GPIO_Port GPIOA
#define USB_RX1_MGMT_Pin GPIO_PIN_9
#define USB_RX1_MGMT_GPIO_Port GPIOA
#define USB_TX1_MGMT_Pin GPIO_PIN_10
#define USB_TX1_MGMT_GPIO_Port GPIOA
#define MISTAKE_Pin GPIO_PIN_11
#define MISTAKE_GPIO_Port GPIOA
#define MISTAKEA12_Pin GPIO_PIN_12
#define MISTAKEA12_GPIO_Port GPIOA
#define UI_BOOT0_Pin GPIO_PIN_15
#define UI_BOOT0_GPIO_Port GPIOA
#define UI_NRST_Pin GPIO_PIN_3
#define UI_NRST_GPIO_Port GPIOB
#define NET_NRST_Pin GPIO_PIN_4
#define NET_NRST_GPIO_Port GPIOB
#define NET_BOOT_Pin GPIO_PIN_5
#define NET_BOOT_GPIO_Port GPIOB
#define LEDB_G_Pin GPIO_PIN_6
#define LEDB_G_GPIO_Port GPIOB
#define NET_STAT_Pin GPIO_PIN_7
#define NET_STAT_GPIO_Port GPIOB
#define UI_BOOT1_Pin GPIO_PIN_8
#define UI_BOOT1_GPIO_Port GPIOB
#define UI_STAT_Pin GPIO_PIN_9
#define UI_STAT_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
