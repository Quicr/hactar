/* USER CODE BEGIN Header */
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

// TODO dynamically adjust the receive buff sz
// what I could do is have a large buffer, but depending on if the upload
// is ui or net I can choose to use a small or large send and restart the
// transmission on the DMA.

// TODO Similarily I need to adjust the parity and the data bits to account for
// different expected input and output for the ui and net chips

// TODO I also need to dynamically change the "to_uart on the uart streams"

#define BAUD 115200
#define RECEIVE_BUFF_SZ 1024
#define TRANSMIT_BUFF_SZ RECEIVE_BUFF_SZ * 2
#define TRANSMISSION_TIMEOUT 5000

// Structure for uart copy
typedef struct {
  UART_HandleTypeDef* from_uart;
  UART_HandleTypeDef* to_uart;
  uint8_t rx_buffer[RECEIVE_BUFF_SZ];
  uint8_t tx_buffer[TRANSMIT_BUFF_SZ];
  uint16_t rx_read;
  uint16_t tx_write;
  uint16_t tx_read;
  uint8_t tx_read_overflow;
  uint8_t tx_free;
  uint16_t pending_bytes;
  uint8_t idle_receive;
  uint32_t last_transmission_time;
  uint8_t has_received;
} uart_stream_t;
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart1_tx;
DMA_HandleTypeDef hdma_usart2_rx;
DMA_HandleTypeDef hdma_usart2_tx;
DMA_HandleTypeDef hdma_usart3_rx;
DMA_HandleTypeDef hdma_usart3_tx;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART3_UART_Init(void);
/* USER CODE BEGIN PFP */
extern inline void HandleRx(uart_stream_t* uart_stream, uint16_t num_received);
extern inline void HandleTx(uart_stream_t* uart_stream);
extern inline void InitUartStreamParameters(uart_stream_t* uart_stream);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uart_stream_t usb_stream;
uart_stream_t periph_stream;

uint16_t send_bytes = 0;

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size)
{
  // There are 3 conditions that call this function.

  // 1. Half complete -- The rx buffer is half full
  // 2. rx complete -- the rx buffer is full
  // 3. idle  -- Nothing has been received in awhile

  // Need to have as separate if statements so we can loop back properly
  if (huart->Instance == periph_stream.from_uart->Instance)
  {
    HandleRx(&periph_stream, size);
  }

  if (huart->Instance == usb_stream.from_uart->Instance)
  {
    HandleRx(&usb_stream, size);
  }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  // Need to have as separate if statements so we can loop back properly
  if (huart->Instance == periph_stream.to_uart->Instance)
  {
    periph_stream.tx_free = 1;
    periph_stream.last_transmission_time = HAL_GetTick();
  }

  if (huart->Instance == usb_stream.to_uart->Instance)
  {
    usb_stream.tx_free = 1;
    usb_stream.last_transmission_time = HAL_GetTick();
  }
}

extern inline void HandleRx(uart_stream_t* rx_stream, uint16_t num_received)
{
    // Calculate the number of bytes have occurred since the last event
    uint16_t num_bytes = num_received - rx_stream->rx_read;

    // Faster than putting a check inside of the copy loop since this is only
    // checked once per rx event.
    if (rx_stream->tx_write + num_bytes > TRANSMIT_BUFF_SZ)
    {
      // Fill in the remaining space and circle around
      while (rx_stream->rx_read < num_received && rx_stream->tx_write < TRANSMIT_BUFF_SZ)
      {
        rx_stream->tx_buffer[rx_stream->tx_write++] = rx_stream->rx_buffer[rx_stream->rx_read++];
      }

      rx_stream->tx_write = 0;
    }

    // Copy bytes to tx buffer
    while (rx_stream->rx_read < num_received)
    {
      rx_stream->tx_buffer[rx_stream->tx_write++] = rx_stream->rx_buffer[rx_stream->rx_read++];
    }

    // rx read head is at the end
    if(rx_stream->rx_read == RECEIVE_BUFF_SZ)
    {
      rx_stream->rx_read = 0;
    }

    if (rx_stream->from_uart->RxEventType == HAL_UART_RXEVENT_IDLE)
    {
      // Set the idle receive flag
      rx_stream->idle_receive = 1;
    }

    // Update the number of pending bytes
    rx_stream->pending_bytes += num_bytes;
    rx_stream->has_received = 1;
    rx_stream->last_transmission_time = HAL_GetTick();
    HAL_GPIO_TogglePin(LEDA_G_GPIO_Port, LEDA_G_Pin);
}

extern inline void HandleTx(uart_stream_t* uart_stream)
{
  if (uart_stream->pending_bytes > 0 && uart_stream->tx_free)
  {
    if (uart_stream->pending_bytes >= RECEIVE_BUFF_SZ || uart_stream->idle_receive || uart_stream->tx_read_overflow)
    {
      send_bytes = RECEIVE_BUFF_SZ;

      // Should only occur on an idle
      if (uart_stream->idle_receive || uart_stream->tx_read_overflow)
      {
        send_bytes = uart_stream->pending_bytes;
        uart_stream->tx_read_overflow = 0;
      }

      if (send_bytes > TRANSMIT_BUFF_SZ - uart_stream->tx_read)
      {
        send_bytes = TRANSMIT_BUFF_SZ - uart_stream->tx_read;
        uart_stream->tx_read_overflow = 1;
      }
      else if (send_bytes > uart_stream->pending_bytes)
      {
        send_bytes = uart_stream->pending_bytes;
      }

      if (uart_stream->idle_receive && uart_stream->pending_bytes == 0)
      {
        uart_stream->idle_receive = 0;
      }

      uart_stream->tx_free = 0;
      HAL_UART_Transmit_DMA(uart_stream->to_uart, (uart_stream->tx_buffer + uart_stream->tx_read), send_bytes);
      uart_stream->pending_bytes -= send_bytes;
      uart_stream->tx_read += send_bytes;
      HAL_GPIO_TogglePin(LEDA_B_GPIO_Port, LEDA_B_Pin);
    }

    if (uart_stream->tx_read >= TRANSMIT_BUFF_SZ)
    {
      uart_stream->tx_read = 0;
    }
  }

  if (uart_stream->has_received && HAL_GetTick() > uart_stream->last_transmission_time + TRANSMISSION_TIMEOUT)
  {
    // Clean up and reset all params
    InitUartStreamParameters(uart_stream);
    HAL_GPIO_TogglePin(LEDA_R_GPIO_Port, LEDA_R_Pin);
  }
}

extern inline void InitUartStreamParameters(uart_stream_t* uart_stream)
{
  HAL_UART_AbortReceive_IT(uart_stream->from_uart);
  uart_stream->rx_read = 0;
  uart_stream->tx_write = 0;
  uart_stream->tx_read = 0;
  uart_stream->tx_read_overflow = 0;
  uart_stream->tx_free = 1;
  uart_stream->pending_bytes = 0;
  uart_stream->idle_receive = 0;
  uart_stream->last_transmission_time = 0;
  uart_stream->has_received = 0;

  // Restart the uart stream
  HAL_UARTEx_ReceiveToIdle_DMA(uart_stream->from_uart, uart_stream->rx_buffer, RECEIVE_BUFF_SZ);
}

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
  MX_DMA_Init();
  MX_TIM3_Init();
  MX_USART2_UART_Init();
  MX_USART1_UART_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */

  // Set LEDS for ui
  HAL_GPIO_WritePin(LEDB_R_GPIO_Port, LEDB_R_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LEDB_G_GPIO_Port, LEDB_G_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LEDB_B_GPIO_Port, LEDB_B_Pin, GPIO_PIN_SET);

  // Set LEDS for net
  HAL_GPIO_WritePin(LEDA_R_GPIO_Port, LEDA_R_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LEDA_G_GPIO_Port, LEDA_G_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LEDA_B_GPIO_Port, LEDA_B_Pin, GPIO_PIN_SET);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  // Put the stm into reset
  HAL_GPIO_WritePin(UI_BOOT0_GPIO_Port, UI_BOOT0_Pin, GPIO_PIN_SET);

  // Bring BOOT1 to low and leave it?
  HAL_GPIO_WritePin(UI_BOOT1_GPIO_Port, UI_BOOT1_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(UI_RST_GPIO_Port, UI_RST_Pin, GPIO_PIN_RESET);

  HAL_Delay(10);
  HAL_GPIO_WritePin(UI_RST_GPIO_Port, UI_RST_Pin, GPIO_PIN_SET);
  HAL_Delay(10);
  HAL_GPIO_WritePin(UI_BOOT0_GPIO_Port, UI_BOOT0_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(UI_BOOT1_GPIO_Port, UI_BOOT1_Pin, GPIO_PIN_SET);

  HAL_GPIO_WritePin(NET_BOOT_GPIO_Port, NET_BOOT_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(NET_RST_GPIO_Port, NET_RST_Pin, GPIO_PIN_RESET);

  // Bring the boot low for esp, bootloader mode (0)
  HAL_GPIO_WritePin(NET_BOOT_GPIO_Port, NET_BOOT_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(NET_RST_GPIO_Port, NET_RST_Pin, GPIO_PIN_SET);

  // TODO move these into functions for starting ui/net upload
  // Init uart structures
  usb_stream.from_uart = &huart3;
  usb_stream.to_uart = &huart1;
  InitUartStreamParameters(&usb_stream);

  periph_stream.from_uart = &huart1;
  periph_stream.to_uart = &huart3;
  InitUartStreamParameters(&periph_stream);

  uint32_t blink = 0;
  while (1)
  {
    if (HAL_GetTick() > blink)
    {
      // Blink for alive!
      HAL_GPIO_TogglePin(LEDB_B_GPIO_Port, LEDB_B_Pin);
      blink = HAL_GetTick() + 1000;
    }

    // Constantly check for transmissions
    HandleTx(&usb_stream);
    HandleTx(&periph_stream);
  }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1|RCC_PERIPHCLK_USART2;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
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
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */
  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */
  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */
  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */
  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */
  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_9B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_EVEN;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */
  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */
  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */
  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */
  /* USER CODE END USART3_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel2_3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel2_3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);
  /* DMA1_Channel4_5_6_7_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel4_5_6_7_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel4_5_6_7_IRQn);

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
  HAL_GPIO_WritePin(GPIOA, LEDB_R_Pin|LEDA_R_Pin|LEDA_G_Pin|UI_BOOT1_Pin
                          |UI_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LEDA_B_Pin|LEDB_G_Pin|LEDB_B_Pin|UI_BOOT0_Pin
                          |NET_RST_Pin|NET_BOOT_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : BTN_RST_Pin BTN_UI_Pin BTN_NET_Pin */
  GPIO_InitStruct.Pin = BTN_RST_Pin|BTN_UI_Pin|BTN_NET_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : ADC_UI_STAT_Pin ADC_NET_STAT_Pin UI_STAT_Pin NET_STAT_Pin */
  GPIO_InitStruct.Pin = ADC_UI_STAT_Pin|ADC_NET_STAT_Pin|UI_STAT_Pin|NET_STAT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LEDB_R_Pin LEDA_R_Pin LEDA_G_Pin UI_BOOT1_Pin
                           UI_RST_Pin */
  GPIO_InitStruct.Pin = LEDB_R_Pin|LEDA_R_Pin|LEDA_G_Pin|UI_BOOT1_Pin
                          |UI_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LEDA_B_Pin LEDB_G_Pin LEDB_B_Pin UI_BOOT0_Pin
                           NET_RST_Pin NET_BOOT_Pin */
  GPIO_InitStruct.Pin = LEDA_B_Pin|LEDB_G_Pin|LEDB_B_Pin|UI_BOOT0_Pin
                          |NET_RST_Pin|NET_BOOT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : RTS_Pin CTS_Pin */
  GPIO_InitStruct.Pin = RTS_Pin|CTS_Pin;
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

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI4_15_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);

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
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
