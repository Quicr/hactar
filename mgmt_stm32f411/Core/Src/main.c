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
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart6;



enum State
{
    Reset,
    UI,
    Net,
};

volatile uint8_t uploading = 0;
volatile enum State state = Reset;
volatile uint32_t slave_odr_on = 0x0; // OR operations will only allow current set
volatile uint32_t slave_odr_off = 0x0; // AND operation check will give 0
volatile uint32_t slave_idr = ~0x0; // AND operation will not change anything

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART6_UART_Init(void);

void InitButtons();
void InitLEDS();
void InitIRQ();
void ResetUpload();
void UploadUIButton();
void UploadNetButton();
void InitBitBangPins();
/* USER CODE BEGIN PFP */

void HAL_GPIO_EXTI_Callback(uint16_t gpio_pin)
{
    if (gpio_pin == GPIO_PIN_12)
    {
        // When the RTS line goes low disable the programming
        ResetUpload();
    }
    else if (gpio_pin == GPIO_PIN_13)
    {
        ResetUpload();
    }
    else if (gpio_pin == GPIO_PIN_14)
    {
        UploadUIButton();
    }
    else if (gpio_pin == GPIO_PIN_15)
    {
        UploadNetButton();
    }
}

void InitButtons()
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    // Put pin 13, 14, and 15 into input mode [00] (reset)
    GPIOC->MODER &= ~GPIO_MODER_MODE13;
    GPIOC->MODER &= ~GPIO_MODER_MODE14;
    GPIOC->MODER &= ~GPIO_MODER_MODE15;

    SYSCFG->EXTICR[3] |= SYSCFG_EXTICR4_EXTI13_PC;
    EXTI->IMR |= EXTI_IMR_IM13;
    EXTI->RTSR |= EXTI_RTSR_TR13;

    SYSCFG->EXTICR[3] |= SYSCFG_EXTICR4_EXTI14_PC;
    EXTI->IMR |= EXTI_IMR_IM14;
    EXTI->RTSR |= EXTI_RTSR_TR14;

    SYSCFG->EXTICR[3] |= SYSCFG_EXTICR4_EXTI15_PC;
    EXTI->IMR |= EXTI_IMR_IM15;
    EXTI->RTSR |= EXTI_RTSR_TR15;
}

void InitLEDS()
{
    // TODO move to LL version
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    /* USER CODE BEGIN MX_GPIO_Init_1 */
    /* USER CODE END MX_GPIO_Init_1 */

      /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /*Configure GPIO pin : PB14 */
    GPIO_InitStruct.Pin = GPIO_PIN_14;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /*Configure GPIO pin : PB14 */
    GPIO_InitStruct.Pin = GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void InitBitBangPins()
{
    // TODO move to LL version
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };

    /*Configure GPIO pins : PA9|PA10 tx/rx */
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /*Configure GPIO pins : PA2|PA3 tx/rx */
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /*Configure GPIO pins : PA11|PA12 tx/rx */
    GPIO_InitStruct.Pin = GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);


    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    GPIOB->MODER &= ~GPIO_MODER_MODE12;

    SYSCFG->EXTICR[3] |= SYSCFG_EXTICR4_EXTI12_PB;
    EXTI->IMR |= EXTI_IMR_IM12;
    // Disable rising edge trigger and Enable falling edge trigger
    EXTI->RTSR &= ~EXTI_RTSR_TR12;
    EXTI->FTSR |= EXTI_FTSR_TR12;
}

void InitIRQ()
{
    NVIC_EnableIRQ(EXTI15_10_IRQn);
}

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void ResetUpload()
{
    state = RESET;
    uploading = 0;

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
}

void UploadUIButton()
{
    state = UI;
    slave_odr_on = GPIO_ODR_OD11;
    slave_odr_off = ~slave_odr_on;
    slave_idr = GPIO_IDR_ID12;

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
}

void UploadNetButton()
{
    state = Net;
    slave_odr_on = GPIO_ODR_OD2;
    slave_odr_off = ~slave_odr_on;
    slave_idr = GPIO_IDR_ID3;

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);
}

void TransmitToSlave()
{
    volatile uint32_t input_pins;
    const uint32_t master_odr_on = GPIO_ODR_OD9;
    const uint32_t master_odr_off = ~GPIO_ODR_OD9;

    // Start uploading
    uploading = 1;
    while (uploading)
    {
        // Get the current input pin states
        input_pins = GPIOA->IDR;

        // GPIOA->ODR = (GPIOA->ODR & slave_odr_off) | ((input_pins & GPIO_IDR_ID10) ? slave_odr_on : 0);
        // GPIOA->ODR = (GPIOA->ODR & master_odr_off) | ((input_pins & slave_idr) ? master_odr_on : 0);

        // Copy the bit value for PA10(rx) to PA2(tx) or PA11(tx)
        if (input_pins & GPIO_IDR_ID10)
        {
            GPIOA->ODR |= slave_odr_on;
        }
        else
        {
            GPIOA->ODR &= slave_odr_off;
        }

        // Copy the bit value of PA3(rx) or PA12(rx) to PA9(tx)
        if (input_pins & slave_idr)
        {
            GPIOA->ODR |= GPIO_ODR_OD9;
        }
        else
        {
            GPIOA->ODR &= master_odr_off;
        }
    }
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
    // MX_GPIO_Init();
    /* USER CODE BEGIN 2 */
    InitButtons();
    InitLEDS();
    InitBitBangPins();
    InitIRQ();


    /* USER CODE END 2 */



        // Copy the bit value for GPIO_IDR_ID




        // Reset OD2 and set to ID10
        // GPIOA->ODR = (GPIOA->ODR & ~GPIO_ODR_OD2) | ((input_pins & GPIO_IDR_ID10) ? GPIO_ODR_OD2 : 0);
        // GPIOA->ODR = (GPIOA->ODR & ~GPIO_ODR_OD9) | ((input_pins & GPIO_IDR_ID3) ? GPIO_ODR_OD9 : 0)


    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    uint32_t next_blink = 0;

    // listen on master
    while (1)
    {
        // https://www.youtube.com/watch?v=92A98iEFmaA
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */

        // Read the button

        // Check the state
        if (state != RESET)
        {
            TransmitToSlave();
        }

        // if (!uploading)
        // {
        //     int reset_button = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13);
        //     int ui_button = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_14);
        //     int net_button = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_15);

        //     if (reset_button)
        //     {

        //     }
        //     else if (ui_button)
        //     {

        //     }
        //     else if (net_button)
        //     {

        //     }

        //     if (HAL_GetTick() > next_blink)
        //     {
        //         HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_13);
        //         next_blink = HAL_GetTick() + 200;
        //     }
        // }
    }
    /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
    RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

    /** Configure the main internal regulator output voltage
    */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /** Initializes the RCC Oscillators according to the specified parameters
    * in the RCC_OscInitTypeDef structure.
    */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
    */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
        | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSE;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

    /* USER CODE BEGIN USART1_Init 0 */

    huart1.Instance = USART1;

    // FOR STM32 = 115200
    // FOR ESP32 = 78800
    huart1.Init.BaudRate = 115200;
    // FOR STM32 = UART_WORDLENGTH_9B
    // FOR ESP32 = UART_WORDLENGTH_8B
    huart1.Init.WordLength = UART_WORDLENGTH_9B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    // STM32 = UART_PARITY_EVEN
    // ESP32 = UART_PARITY_NONE
    huart1.Init.Parity = UART_PARITY_EVEN;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE END USART1_Init 0 */

    /* USER CODE BEGIN USART1_Init 1 */

    /* USER CODE END USART1_Init 1 */
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
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_9B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_EVEN;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
        Error_Handler();
    }

    /* USER CODE END USART2_Init 0 */

    /* USER CODE BEGIN USART2_Init 1 */

    /* USER CODE END USART2_Init 1 */
    /* USER CODE BEGIN USART2_Init 2 */

    /* USER CODE END USART2_Init 2 */

}

/**
  * @brief USART6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART6_UART_Init(void)
{

    /* USER CODE BEGIN USART6_Init 0 */
    huart6.Instance = USART6;
    huart6.Init.BaudRate = 115200;
    huart6.Init.WordLength = UART_WORDLENGTH_8B;
    huart6.Init.StopBits = UART_STOPBITS_1;
    huart6.Init.Parity = UART_PARITY_NONE;
    huart6.Init.Mode = UART_MODE_TX_RX;
    huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart6.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart6) != HAL_OK)
    {
        Error_Handler();
    }

    /* USER CODE END USART6_Init 0 */

    /* USER CODE BEGIN USART6_Init 1 */

    /* USER CODE END USART6_Init 1 */
    /* USER CODE BEGIN USART6_Init 2 */

    /* USER CODE END USART6_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    /* USER CODE BEGIN MX_GPIO_Init_1 */
    /* USER CODE END MX_GPIO_Init_1 */

      /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15, GPIO_PIN_RESET);

    // /*Configure GPIO pins : PC13 PC14 PC15 */
    // GPIO_InitStruct.Pin = GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    // GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    // GPIO_InitStruct.Pull = GPIO_NOPULL;
    // HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /*Configure GPIO pins : PB13 PB14 PB15 */
    GPIO_InitStruct.Pin = GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* USER CODE BEGIN MX_GPIO_Init_2 */
    /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart, uint16_t size)
{
    // if (huart->Instance == USART1)
    // {
    //     // Received from programmer
    //     uint8_t* prev_pointer = from_master2.current_buffer_ptr;
    //     HAL_UART_Transmit_IT(prog_uart, prev_pointer, size);

    //     from_master2.current_buffer_ptr = from_master2.next_buffer_ptr;
    //     from_master2.next_buffer_ptr = prev_pointer;

    //     HAL_UARTEx_ReceiveToIdle_IT(&huart1, from_master2.current_buffer_ptr, UART_BUFF_SZ);
    // }
    // else if (huart->Instance == USART2 || huart->Instance == USART6)
    // {

    //     // slave_ready = 0;
    //     uint8_t* prev_pointer = from_slave2.current_buffer_ptr;
    //     HAL_UART_Transmit_IT(&huart1, prev_pointer, size);

    //     from_slave2.current_buffer_ptr = from_slave2.next_buffer_ptr;
    //     from_slave2.next_buffer_ptr = prev_pointer;

    //     // Listen on slave
    //     HAL_UARTEx_ReceiveToIdle_IT(prog_uart, from_slave2.current_buffer_ptr, UART_BUFF_SZ);
    // }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart)
{
}

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
void assert_failed(uint8_t* file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
       /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
