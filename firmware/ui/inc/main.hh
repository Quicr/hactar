/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : Header for main.c file.
 *                   This file contains the common defines of the application.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2022 STMicroelectronics.
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
#include "stm32.h"

void Error_Handler(void);

// THINK do I need extern?
// Handlers
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart1_tx;
extern SPI_HandleTypeDef hspi1;
extern DMA_HandleTypeDef hdma_spi1_tx;
extern I2C_HandleTypeDef hi2c1;
extern TIM_HandleTypeDef htim2;

#define LCD_SPI             SPI1

#define LCD_MOSI_Pin        GPIO_PIN_7
#define LCD_MOSI_GPIO_Port  GPIOA
#define LCD_SCK_Pin         GPIO_PIN_5
#define LCD_SCK_GPIO_Port   GPIOA
#define LCD_CS_Pin          GPIO_PIN_1
#define LCD_CS_GPIO_Port    GPIOA
#define LCD_DC_Pin          GPIO_PIN_0
#define LCD_DC_GPIO_Port    GPIOB
#define LCD_RST_Pin         GPIO_PIN_6
#define LCD_RST_GPIO_Port   GPIOA
#define LCD_BL_Pin          GPIO_PIN_4
#define LCD_BL_GPIO_Port    GPIOA

#define Q10_COL_1_PIN       GPIO_PIN_9
#define Q10_COL_1_GPIO_PORT GPIOB
#define Q10_COL_2_PIN       GPIO_PIN_8
#define Q10_COL_2_GPIO_PORT GPIOB
#define Q10_COL_3_PIN       GPIO_PIN_14
#define Q10_COL_3_GPIO_PORT GPIOC
#define Q10_COL_4_PIN       GPIO_PIN_15
#define Q10_COL_4_GPIO_PORT GPIOC
#define Q10_COL_5_PIN       GPIO_PIN_5
#define Q10_COL_5_GPIO_PORT GPIOB

#define Q10_ROW_1_PIN       GPIO_PIN_12
#define Q10_ROW_1_GPIO_PORT GPIOB
#define Q10_ROW_2_PIN       GPIO_PIN_13
#define Q10_ROW_2_GPIO_PORT GPIOB
#define Q10_ROW_3_PIN       GPIO_PIN_14
#define Q10_ROW_3_GPIO_PORT GPIOB
#define Q10_ROW_4_PIN       GPIO_PIN_15
#define Q10_ROW_4_GPIO_PORT GPIOB
#define Q10_ROW_5_PIN       GPIO_PIN_8
#define Q10_ROW_5_GPIO_PORT GPIOC
#define Q10_ROW_6_PIN       GPIO_PIN_3
#define Q10_ROW_6_GPIO_PORT GPIOB
#define Q10_ROW_7_PIN       GPIO_PIN_4
#define Q10_ROW_7_GPIO_PORT GPIOB

#define Q10_TIMER_LED_PIN   GPIO_PIN_6
#define Q10_TIMER_LED_PORT  GPIOC

#define UI_TX2_NET_Pin       GPIO_PIN_2
#define UI_TX2_NET_GPIO_PORT GPIOA
#define UI_RX2_NET_Pin       GPIO_PIN_3
#define UI_RX2_NET_GPIO_PORT GPIOA

#define UI_TX1_MGMT_Pin       GPIO_PIN_9
#define UI_TX1_MGMT_GPIO_Port GPIOA
#define UI_RX1_MGMT_Pin       GPIO_PIN_10
#define UI_RX1_MGMT_GPIO_Port GPIOA

#define LED_R_Pin            GPIO_PIN_1
#define LED_R_Port           GPIOC
#define LED_G_Pin            GPIO_PIN_0
#define LED_G_Port           GPIOC
#define LED_B_Pin            GPIO_PIN_2
#define LED_B_Port           GPIOD

#define UI_STAT_Pin          GPIO_PIN_8
#define UI_STAT_Port         GPIOA

#define UI_DBG1_Pin          GPIO_PIN_4
#define UI_DBG1_Port         GPIOC
#define UI_DBG2_Pin          GPIO_PIN_5
#define UI_DBG2_Port         GPIOC
#define UI_DBG3_Pin          GPIO_PIN_6
#define UI_DBG3_Port         GPIOC
#define UI_DBG4_Pin          GPIO_PIN_7
#define UI_DBG4_Port         GPIOC

#define EEPROM_SCL_PORT     GPIOB
#define EEPROM_SCL_PIN      GPIO_PIN_6
#define EEPROM_SDA_PORT     GPIOB
#define EEPROM_SDA_PIN      GPIO_PIN_7
#define EEPROM_RW_PORT      GPIOC
#define EEPROM_RW_PIN       GPIO_PIN_8

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
