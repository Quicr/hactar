/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

// To appease the vscode c++ intellisense
#ifdef __cplusplus
#undef __cplusplus
#endif

#ifndef STM32F072xB
#define STM32F072xB
#endif
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
#define BTN_RST_Pin GPIO_PIN_13
#define BTN_RST_GPIO_Port GPIOC
#define BTN_RST_EXTI_IRQn EXTI4_15_IRQn
#define BTN_UI_Pin GPIO_PIN_14
#define BTN_UI_GPIO_Port GPIOC
#define BTN_UI_EXTI_IRQn EXTI4_15_IRQn
#define BTN_NET_Pin GPIO_PIN_15
#define BTN_NET_GPIO_Port GPIOC
#define BTN_NET_EXTI_IRQn EXTI4_15_IRQn
#define ADC_UI_STAT_Pin GPIO_PIN_0
#define ADC_UI_STAT_GPIO_Port GPIOA
#define ADC_NET_STAT_Pin GPIO_PIN_1
#define ADC_NET_STAT_GPIO_Port GPIOA
#define UI_RX1_MGMT_Pin GPIO_PIN_2
#define UI_RX1_MGMT_GPIO_Port GPIOA
#define UI_TX1_MGMT_Pin GPIO_PIN_3
#define UI_TX1_MGMT_GPIO_Port GPIOA
#define LEDB_R_Pin GPIO_PIN_4
#define LEDB_R_GPIO_Port GPIOA
#define LEDA_R_Pin GPIO_PIN_6
#define LEDA_R_GPIO_Port GPIOA
#define LEDA_G_Pin GPIO_PIN_7
#define LEDA_G_GPIO_Port GPIOA
#define LEDA_B_Pin GPIO_PIN_0
#define LEDA_B_GPIO_Port GPIOB
#define RTS_Pin GPIO_PIN_1
#define RTS_GPIO_Port GPIOB
#define RTS_EXTI_IRQn EXTI0_1_IRQn
#define MGMT_TX3_USB_Pin GPIO_PIN_10
#define MGMT_TX3_USB_GPIO_Port GPIOB
#define MGMT_RX3_USB_Pin GPIO_PIN_11
#define MGMT_RX3_USB_GPIO_Port GPIOB
#define CTS_Pin GPIO_PIN_13
#define CTS_GPIO_Port GPIOB
#define LEDB_G_Pin GPIO_PIN_14
#define LEDB_G_GPIO_Port GPIOB
#define LEDB_B_Pin GPIO_PIN_15
#define LEDB_B_GPIO_Port GPIOB
#define UI_STAT_Pin GPIO_PIN_9
#define UI_STAT_GPIO_Port GPIOA
#define NET_STAT_Pin GPIO_PIN_10
#define NET_STAT_GPIO_Port GPIOA
#define UI_BOOT1_Pin GPIO_PIN_8
#define UI_BOOT1_GPIO_Port GPIOB
#define UI_RST_Pin GPIO_PIN_15
#define UI_RST_GPIO_Port GPIOA
#define UI_BOOT0_Pin GPIO_PIN_3
#define UI_BOOT0_GPIO_Port GPIOB
#define NET_RST_Pin GPIO_PIN_4
#define NET_RST_GPIO_Port GPIOB
#define NET_BOOT_Pin GPIO_PIN_5
#define NET_BOOT_GPIO_Port GPIOB
#define NET_RX0_MGMT_Pin GPIO_PIN_6
#define NET_RX0_MGMT_GPIO_Port GPIOB
#define NET_TX0_MGMT_Pin GPIO_PIN_7
#define NET_TX0_MGMT_GPIO_Port GPIOB
#define MGMT_DBG7_Pin GPIO_PIN_11
#define MGMT_DBG7_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */

#define USB_PORT GPIOB
#define USB_RX_Bit GPIO_IDR_11
#define USB_TX_Bit_On GPIO_ODR_10
#define USB_TX_Bit_Off ~GPIO_ODR_10

#define UI_PORT GPIOA
#define UI_RX_Bit GPIO_IDR_3      // The input data register bit on mgmt
#define UI_TX_Bit_On GPIO_ODR_2   // The output data register bit on mgmt
#define UI_TX_Bit_Off ~GPIO_ODR_2

#define NET_PORT GPIOB
#define NET_RX_Bit GPIO_IDR_7     // The input data register bit on mgmt
#define NET_TX_Bit_On GPIO_ODR_6  // The output data register bit on mgmt
#define NET_TX_Bit_Off ~GPIO_ODR_6


/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */