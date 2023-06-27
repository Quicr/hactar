/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file         stm32f0xx_hal_msp.c
  * @brief        This file provides code for the MSP Initialization
  *               and de-Initialization codes.
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
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
/* USER CODE BEGIN Includes */
extern DMA_HandleTypeDef usb_dma6_rx;
extern DMA_HandleTypeDef usb_dma7_tx;
extern DMA_HandleTypeDef ui_dma4_tx;
extern DMA_HandleTypeDef ui_dma5_rx;
extern DMA_HandleTypeDef net_dma2_tx;
extern DMA_HandleTypeDef net_dma3_rx;
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN Define */

/* USER CODE END Define */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN Macro */

/* USER CODE END Macro */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* External functions --------------------------------------------------------*/
/* USER CODE BEGIN ExternalFunctions */

/* USER CODE END ExternalFunctions */

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */
/**
  * Initializes the Global MSP.
  */
void HAL_MspInit(void)
{
  /* USER CODE BEGIN MspInit 0 */

  /* USER CODE END MspInit 0 */

  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_ENABLE();

  /* System interrupt init*/

  /* USER CODE BEGIN MspInit 1 */

  /* USER CODE END MspInit 1 */
}

/**
* @brief TIM_Base MSP Initialization
* This function configures the hardware resources used in this example
* @param htim_base: TIM_Base handle pointer
* @retval None
*/
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* htim_base)
{
  if(htim_base->Instance==TIM3)
  {
  /* USER CODE BEGIN TIM3_MspInit 0 */

  /* USER CODE END TIM3_MspInit 0 */
    /* Peripheral clock enable */
    __HAL_RCC_TIM3_CLK_ENABLE();
  /* USER CODE BEGIN TIM3_MspInit 1 */

  /* USER CODE END TIM3_MspInit 1 */
  }

}

/**
* @brief TIM_Base MSP De-Initialization
* This function freeze the hardware resources used in this example
* @param htim_base: TIM_Base handle pointer
* @retval None
*/
void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef* htim_base)
{
  if(htim_base->Instance==TIM3)
  {
  /* USER CODE BEGIN TIM3_MspDeInit 0 */

  /* USER CODE END TIM3_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_TIM3_CLK_DISABLE();
  /* USER CODE BEGIN TIM3_MspDeInit 1 */

  /* USER CODE END TIM3_MspDeInit 1 */
  }

}

/* USER CODE BEGIN 1 */



/**
* @brief UART MSP Initialization
* This function configures the hardware resources used in this example
* @param huart: UART handle pointer
* @retval None
*/
void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(huart->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspInit 0 */

  /* USER CODE END USART1_MspInit 0 */
    /* Peripheral clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**USART1 GPIO Configuration
    PB6     ------> USART1_TX
    PB7     ------> USART1_RX
    */
    GPIO_InitStruct.Pin = NET_RX0_MGMT_Pin|NET_TX0_MGMT_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF0_USART1;
    HAL_GPIO_Init(NET_RX0_MGMT_GPIO_Port, &GPIO_InitStruct);

    /* USART1 DMA Init */
    /* USART1_RX Init */
    net_dma3_rx.Instance = DMA1_Channel3;
    net_dma3_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    net_dma3_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    net_dma3_rx.Init.MemInc = DMA_MINC_ENABLE;
    net_dma3_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    net_dma3_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    net_dma3_rx.Init.Mode = DMA_NORMAL;
    net_dma3_rx.Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&net_dma3_rx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(huart,hdmarx,net_dma3_rx);

    /* USART1_TX Init */
    net_dma2_tx.Instance = DMA1_Channel2;
    net_dma2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    net_dma2_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    net_dma2_tx.Init.MemInc = DMA_MINC_ENABLE;
    net_dma2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    net_dma2_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    net_dma2_tx.Init.Mode = DMA_NORMAL;
    net_dma2_tx.Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&net_dma2_tx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(huart,hdmatx,net_dma2_tx);
    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);

  /* USER CODE BEGIN USART1_MspInit 1 */

  /* USER CODE END USART1_MspInit 1 */
  }
  else if(huart->Instance==USART2)
  {
  /* USER CODE BEGIN USART2_MspInit 0 */

  /* USER CODE END USART2_MspInit 0 */
    /* Peripheral clock enable */
    __HAL_RCC_USART2_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART2 GPIO Configuration
    PA2     ------> USART2_TX
    PA3     ------> USART2_RX
    */
    GPIO_InitStruct.Pin = UI_RX1_MGMT_Pin|UI_TX1_MGMT_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF1_USART2;
    HAL_GPIO_Init(UI_RX1_MGMT_GPIO_Port, &GPIO_InitStruct);

    /* USART2 DMA Init */
    /* USART2_RX Init */
    ui_dma5_rx.Instance = DMA1_Channel5;
    ui_dma5_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    ui_dma5_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    ui_dma5_rx.Init.MemInc = DMA_MINC_ENABLE;
    ui_dma5_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    ui_dma5_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    ui_dma5_rx.Init.Mode = DMA_NORMAL;
    ui_dma5_rx.Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&ui_dma5_rx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(huart,hdmarx,ui_dma5_rx);

    /* USART2_TX Init */
    ui_dma4_tx.Instance = DMA1_Channel4;
    ui_dma4_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    ui_dma4_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    ui_dma4_tx.Init.MemInc = DMA_MINC_ENABLE;
    ui_dma4_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    ui_dma4_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    ui_dma4_tx.Init.Mode = DMA_NORMAL;
    ui_dma4_tx.Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&ui_dma4_tx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(huart,hdmatx,ui_dma4_tx);

  HAL_NVIC_SetPriority(USART3_4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(USART3_4_IRQn);

  /* USER CODE BEGIN USART2_MspInit 1 */

  /* USER CODE END USART2_MspInit 1 */
  }
  else if(huart->Instance==USART3)
  {
  /* USER CODE BEGIN USART3_MspInit 0 */

  /* USER CODE END USART3_MspInit 0 */
    /* Peripheral clock enable */
    __HAL_RCC_USART3_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**USART3 GPIO Configuration
    PB10     ------> USART3_TX
    PB11     ------> USART3_RX
    */
    GPIO_InitStruct.Pin = USB_RX1_MGMT_Pin|USB_TX1_MGMT_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_USART3;
    HAL_GPIO_Init(USB_RX1_MGMT_GPIO_Port, &GPIO_InitStruct);

    /* USART3 DMA Init */
    /* USART3_RX Init */
    usb_dma6_rx.Instance = DMA1_Channel6;
    usb_dma6_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    usb_dma6_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    usb_dma6_rx.Init.MemInc = DMA_MINC_ENABLE;
    usb_dma6_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    usb_dma6_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    usb_dma6_rx.Init.Mode = DMA_NORMAL;
    usb_dma6_rx.Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&usb_dma6_rx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(huart,hdmarx,usb_dma6_rx);

    /* USART3_TX Init */
    usb_dma7_tx.Instance = DMA1_Channel7;
    usb_dma7_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    usb_dma7_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    usb_dma7_tx.Init.MemInc = DMA_MINC_ENABLE;
    usb_dma7_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    usb_dma7_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    usb_dma7_tx.Init.Mode = DMA_NORMAL;
    usb_dma7_tx.Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&usb_dma7_tx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(huart,hdmatx,usb_dma7_tx);
  HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(USART2_IRQn);

  /* USER CODE BEGIN USART3_MspInit 1 */

  /* USER CODE END USART3_MspInit 1 */
  }

}

/**
* @brief UART MSP De-Initialization
* This function freeze the hardware resources used in this example
* @param huart: UART handle pointer
* @retval None
*/
void HAL_UART_MspDeInit(UART_HandleTypeDef* huart)
{
  if(huart->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspDeInit 0 */

  /* USER CODE END USART1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART1_CLK_DISABLE();

    /**USART1 GPIO Configuration
    PB6     ------> USART1_TX
    PB7     ------> USART1_RX
    */
    HAL_GPIO_DeInit(NET_RX0_MGMT_GPIO_Port, NET_RX0_MGMT_Pin|NET_TX0_MGMT_Pin);

    /* USART1 DMA DeInit */
    HAL_DMA_DeInit(huart->hdmarx);
    HAL_DMA_DeInit(huart->hdmatx);
  /* USER CODE BEGIN USART1_MspDeInit 1 */

  /* USER CODE END USART1_MspDeInit 1 */
  }
  else if(huart->Instance==USART2)
  {
  /* USER CODE BEGIN USART2_MspDeInit 0 */

  /* USER CODE END USART2_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART2_CLK_DISABLE();

    /**USART2 GPIO Configuration
    PA2     ------> USART2_TX
    PA3     ------> USART2_RX
    */
    HAL_GPIO_DeInit(UI_RX1_MGMT_GPIO_Port, UI_RX1_MGMT_Pin|UI_TX1_MGMT_Pin);

    /* USART2 DMA DeInit */
    HAL_DMA_DeInit(huart->hdmarx);
    HAL_DMA_DeInit(huart->hdmatx);
  /* USER CODE BEGIN USART2_MspDeInit 1 */

  /* USER CODE END USART2_MspDeInit 1 */
  }
  else if(huart->Instance==USART3)
  {
  /* USER CODE BEGIN USART3_MspDeInit 0 */

  /* USER CODE END USART3_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART3_CLK_DISABLE();

    /**USART3 GPIO Configuration
    PB10     ------> USART3_TX
    PB11     ------> USART3_RX
    */
    HAL_GPIO_DeInit(USB_RX1_MGMT_GPIO_Port, USB_RX1_MGMT_Pin|USB_TX1_MGMT_Pin);

    /* USART3 DMA DeInit */
    HAL_DMA_DeInit(huart->hdmarx);
    HAL_DMA_DeInit(huart->hdmatx);
  /* USER CODE BEGIN USART3_MspDeInit 1 */

  /* USER CODE END USART3_MspDeInit 1 */
  }

}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */


/* USER CODE END 1 */
