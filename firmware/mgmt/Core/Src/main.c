/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim3;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */

  // TURN off right Net LED 
  HAL_GPIO_WritePin( LEDA_R_GPIO_Port, LEDA_R_Pin, GPIO_PIN_SET); 
  HAL_GPIO_WritePin( LEDA_G_GPIO_Port, LEDA_G_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin( LEDA_B_GPIO_Port, LEDA_B_Pin, GPIO_PIN_SET); 

  // TURN off left UI LED 
  HAL_GPIO_WritePin( LEDB_R_GPIO_Port, LEDB_R_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin( LEDB_G_GPIO_Port, LEDB_G_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin( LEDB_B_GPIO_Port, LEDB_B_Pin, GPIO_PIN_SET); 

  
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    // If UI button pressed 
    if ( HAL_GPIO_ReadPin( GPIOC, BTN_UI_Pin ) == 0 ) {
      HAL_GPIO_WritePin(UI_RST_GPIO_Port, UI_RST_Pin,  GPIO_PIN_RESET); // put UI in reset mode 
      HAL_GPIO_WritePin(NET_RST_GPIO_Port, NET_RST_Pin, GPIO_PIN_RESET); // put net in reset mode
      
      HAL_GPIO_WritePin(UI_BOOT_GPIO_Port, UI_BOOT_Pin,  GPIO_PIN_SET); // put UI in bootloader mode 
      HAL_GPIO_WritePin(NET_BOOT_GPIO_Port, NET_BOOT_Pin, GPIO_PIN_SET); // normal boot mode is 1 
 			 
      //  NET LED Off
      HAL_GPIO_WritePin(LEDB_R_GPIO_Port, LEDB_R_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(LEDB_G_GPIO_Port, LEDB_G_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(LEDB_B_GPIO_Port, LEDB_B_Pin, GPIO_PIN_SET);
      //  UI LED Purple 
      HAL_GPIO_WritePin(LEDA_R_GPIO_Port, LEDA_R_Pin, GPIO_PIN_RESET); 
      HAL_GPIO_WritePin(LEDA_G_GPIO_Port, LEDA_G_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(LEDA_B_GPIO_Port, LEDA_B_Pin, GPIO_PIN_RESET); 

      HAL_GPIO_WritePin(UI_RST_GPIO_Port, UI_RST_Pin,  GPIO_PIN_SET); // release UI reset and start program

      // Port notes:
      //    USB_TX1_MGMT_GPIO_Port and  USB_RX1_MGMT_GPIO_Port are GPIOB
      //    UI_TX2_MGMT_GPIO_Port and UI_RX2_MGMT_GPIO_Port are GPIOA
      //    BTN_RST_GPIO_Port is on GPIOC
      
      // Program UI mode
      while( HAL_GPIO_ReadPin( BTN_RST_GPIO_Port, BTN_RST_Pin ) != 0 ) {
	// Copy USB serial to UI serial 
	//HAL_GPIO_WritePin( UI_RX2_MGMT_GPIO_Port, UI_RX2_MGMT_Pin,
	//		   ( HAL_GPIO_ReadPin( USB_TX1_MGMT_GPIO_Port, USB_TX1_MGMT_Pin ) == 0 )
	//		   ? GPIO_PIN_RESET : GPIO_PIN_SET ); 
      
	// Copy UI serial to USB serial 
	//HAL_GPIO_WritePin( USB_RX1_MGMT_GPIO_Port, USB_RX1_MGMT_Pin,
	//		   ( HAL_GPIO_ReadPin( UI_TX2_MGMT_GPIO_Port, UI_TX2_MGMT_Pin ) == 0 )
	//		   ? GPIO_PIN_RESET:GPIO_PIN_SET); 
      }
    
      // go back to normal mode 
      HAL_GPIO_WritePin(UI_RST_GPIO_Port, UI_RST_Pin,  GPIO_PIN_SET); 
      HAL_GPIO_WritePin(NET_RST_GPIO_Port, NET_RST_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(UI_BOOT_GPIO_Port, UI_BOOT_Pin,  GPIO_PIN_RESET); // normal boot mode is 0
      HAL_GPIO_WritePin(NET_BOOT_GPIO_Port, NET_BOOT_Pin, GPIO_PIN_SET); // normal boot mode is 1 
    }

     // If NET button pressed 
    if ( HAL_GPIO_ReadPin(BTN_NET_GPIO_Port, BTN_NET_Pin ) == 0 ) {
      HAL_GPIO_WritePin(UI_RST_GPIO_Port, UI_RST_Pin,  GPIO_PIN_RESET); // put UI in reset mode 
      HAL_GPIO_WritePin(NET_RST_GPIO_Port, NET_RST_Pin, GPIO_PIN_RESET); // put net in reset mode
      
      HAL_GPIO_WritePin(GPIOB, UI_BOOT_Pin,  GPIO_PIN_RESET); // normal boot mode is 0
      HAL_GPIO_WritePin(GPIOB, NET_BOOT_Pin, GPIO_PIN_RESET); // put NET in bootload mode 
 			 
      //  NET LED Purple
      HAL_GPIO_WritePin(LEDB_R_GPIO_Port, LEDB_R_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(LEDB_G_GPIO_Port, LEDB_G_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(LEDB_B_GPIO_Port, LEDB_B_Pin, GPIO_PIN_RESET);
      //  UI LED Off 
      HAL_GPIO_WritePin(LEDA_R_GPIO_Port, LEDA_R_Pin, GPIO_PIN_SET); 
      HAL_GPIO_WritePin(LEDA_G_GPIO_Port, LEDA_G_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(LEDA_B_GPIO_Port, LEDA_B_Pin, GPIO_PIN_SET); 

      HAL_GPIO_WritePin(NET_RST_GPIO_Port, NET_RST_Pin, GPIO_PIN_SET); // release NET reset and start program

      // Port notes:
      //    USB_TX1_MGMT_GPIO_Port and USB_RX1_MGMT_GPIO_Port are GPIOB
      //    NET_TX0_MGMT_GPIO_Port and NET_RX0_MGMT_GPIO_Port are GPIOB
      //    BTN_RST_GPIO_Port is on GPIOC

      // Program NET mode
      while( HAL_GPIO_ReadPin(BTN_RST_GPIO_Port, BTN_RST_Pin ) != 0 ) {
	// Copy USB serial to NET serial
	
	//HAL_GPIO_WritePin( NET_RX0_MGMT_GPIO_Port, NET_RX0_MGMT_Pin,
	//		   ( HAL_GPIO_ReadPin( USB_TX1_MGMT_GPIO_Port, USB_TX1_MGMT_Pin ) == 0 )
	//		   ? GPIO_PIN_RESET : GPIO_PIN_SET ); 
	
	// Copy NET serial to USB serial
	
	//HAL_GPIO_WritePin( USB_RX1_MGMT_GPIO_Port, USB_RX1_MGMT_Pin,
	//		   ( HAL_GPIO_ReadPin( NET_TX0_MGMT_GPIO_Port, NET_TX0_MGMT_Pin ) == 0 )
	//		   ? GPIO_PIN_RESET:GPIO_PIN_SET); 
      }

      // go back to normal mode
      HAL_GPIO_WritePin(UI_BOOT_GPIO_Port, UI_BOOT_Pin,  GPIO_PIN_RESET); // normal boot mode is 0
      HAL_GPIO_WritePin(NET_BOOT_GPIO_Port, NET_BOOT_Pin, GPIO_PIN_SET); // normal boot mode is 1 
      HAL_GPIO_WritePin(UI_RST_GPIO_Port, UI_RST_Pin,  GPIO_PIN_SET); 
      HAL_GPIO_WritePin(NET_RST_GPIO_Port, NET_RST_Pin, GPIO_PIN_SET);
    }
    
    // If reset button pressed 
    if ( HAL_GPIO_ReadPin( BTN_RST_GPIO_Port, BTN_RST_Pin ) == 0 ) {
      HAL_GPIO_WritePin(UI_RST_GPIO_Port, UI_RST_Pin,  GPIO_PIN_RESET); 
      HAL_GPIO_WritePin(NET_RST_GPIO_Port, NET_RST_Pin, GPIO_PIN_RESET);

      HAL_GPIO_WritePin(UI_BOOT_GPIO_Port, UI_BOOT_Pin,  GPIO_PIN_RESET); // normal boot mode is 0
      HAL_GPIO_WritePin(NET_BOOT_GPIO_Port, NET_BOOT_Pin, GPIO_PIN_SET); // normal boot mode is 1 
      
      //  UI LED Off 
      HAL_GPIO_WritePin(LEDA_R_GPIO_Port, LEDA_R_Pin, GPIO_PIN_SET); 
      HAL_GPIO_WritePin(LEDA_G_GPIO_Port, LEDA_G_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(LEDA_B_GPIO_Port, LEDA_B_Pin, GPIO_PIN_SET); 

      //  NET LED  Off
      HAL_GPIO_WritePin(LEDB_R_GPIO_Port, LEDB_R_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(LEDB_G_GPIO_Port, LEDB_G_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(LEDB_B_GPIO_Port, LEDB_B_Pin, GPIO_PIN_SET); 
    }
    else {
      // Normal mode
      HAL_GPIO_WritePin(UI_RST_GPIO_Port, UI_RST_Pin,  GPIO_PIN_SET); 
      HAL_GPIO_WritePin(NET_RST_GPIO_Port, NET_RST_Pin, GPIO_PIN_SET);

      HAL_GPIO_WritePin(UI_BOOT_GPIO_Port, UI_BOOT_Pin,  GPIO_PIN_RESET); // normal boot mode is 0
      HAL_GPIO_WritePin(NET_BOOT_GPIO_Port, NET_BOOT_Pin, GPIO_PIN_SET); // normal boot mode is 1 
      
      // Check UI CPU status 
      if ( HAL_GPIO_ReadPin( UI_STAT_GPIO_Port, UI_STAT_Pin ) == 0 ) {
	//  UI LED Red
	HAL_GPIO_WritePin(LEDA_R_GPIO_Port, LEDA_R_Pin, GPIO_PIN_RESET); 
	HAL_GPIO_WritePin(LEDA_G_GPIO_Port, LEDA_G_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LEDA_B_GPIO_Port, LEDA_B_Pin, GPIO_PIN_SET); 
      }
      else {
	//  UI LED Green 
	HAL_GPIO_WritePin(LEDA_R_GPIO_Port, LEDA_R_Pin, GPIO_PIN_SET); 
	HAL_GPIO_WritePin(LEDA_G_GPIO_Port, LEDA_G_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LEDA_B_GPIO_Port, LEDA_B_Pin, GPIO_PIN_SET); 
      }

      // Check NET CPU status 
      if ( HAL_GPIO_ReadPin( NET_STAT_GPIO_Port, NET_STAT_Pin ) == 0 ) {
	//  NET LED Red
	HAL_GPIO_WritePin(LEDB_R_GPIO_Port, LEDB_R_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LEDB_G_GPIO_Port, LEDB_G_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LEDB_B_GPIO_Port, LEDB_B_Pin, GPIO_PIN_SET);
      }
      else {
	//  NET LED Green
	HAL_GPIO_WritePin(LEDB_R_GPIO_Port, LEDB_R_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LEDB_G_GPIO_Port, LEDB_G_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LEDB_B_GPIO_Port, LEDB_B_Pin, GPIO_PIN_SET);
      }
      
    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL3;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_RCC_MCOConfig(RCC_MCO, RCC_MCO1SOURCE_PLLCLK_DIV2, RCC_MCODIV_2);
}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_SlaveConfigTypeDef sSlaveConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 48000;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sSlaveConfig.SlaveMode = TIM_SLAVEMODE_DISABLE;
  sSlaveConfig.InputTrigger = TIM_TS_ITR0;
  if (HAL_TIM_SlaveConfigSynchro(&htim3, &sSlaveConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LEDB_R_Pin|LEDA_R_Pin|LEDA_G_Pin|MGMT_DBG7_Pin
                          |UI_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LEDA_B_Pin|LEDB_G_Pin|LEDB_B_Pin|UI_BOOT_Pin
                          |NET_RST_Pin|NET_BOOT_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : BTN_RST_Pin BTN_UI_Pin BTN_NET_Pin */
  GPIO_InitStruct.Pin = BTN_RST_Pin|BTN_UI_Pin|BTN_NET_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : ADC_UI_STAT_Pin ADC_NET_STAT_Pin UI_STAT_Pin NET_STAT_Pin */
  GPIO_InitStruct.Pin = ADC_UI_STAT_Pin|ADC_NET_STAT_Pin|UI_STAT_Pin|NET_STAT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : UI_TX2_MGMT_Pin UI_RX2_MGMT_Pin */
  GPIO_InitStruct.Pin = UI_TX2_MGMT_Pin|UI_RX2_MGMT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LEDB_R_Pin LEDA_R_Pin LEDA_G_Pin MGMT_DBG7_Pin
                           UI_RST_Pin */
  GPIO_InitStruct.Pin = LEDB_R_Pin|LEDA_R_Pin|LEDA_G_Pin|MGMT_DBG7_Pin
                          |UI_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LEDA_B_Pin LEDB_G_Pin LEDB_B_Pin UI_BOOT_Pin
                           NET_RST_Pin NET_BOOT_Pin */
  GPIO_InitStruct.Pin = LEDA_B_Pin|LEDB_G_Pin|LEDB_B_Pin|UI_BOOT_Pin
                          |NET_RST_Pin|NET_BOOT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : RTS_Pin USB_TX1_MGMT_Pin USB_RX1_MGMT_Pin CTS_Pin
                           NET_TX0_MGMT_Pin NET_RX0_MGMT_Pin */
  GPIO_InitStruct.Pin = RTS_Pin|USB_TX1_MGMT_Pin|USB_RX1_MGMT_Pin|CTS_Pin
                          |NET_TX0_MGMT_Pin|NET_RX0_MGMT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PA8 */
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF0_MCO;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
    	//  UI LED Blue 
	HAL_GPIO_WritePin(LEDA_R_GPIO_Port, LEDA_R_Pin, GPIO_PIN_SET); 
	HAL_GPIO_WritePin(LEDA_G_GPIO_Port, LEDA_G_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LEDA_B_GPIO_Port, LEDA_B_Pin, GPIO_PIN_RESET);

       //  NET LED Blue
	HAL_GPIO_WritePin(LEDB_R_GPIO_Port, LEDB_R_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LEDB_G_GPIO_Port, LEDB_G_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LEDB_B_GPIO_Port, LEDB_B_Pin, GPIO_PIN_RESET);
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
