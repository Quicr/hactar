/* USER CODE BEGIN Header */
/**
******************************************************************************
* @file           : main.c
* @brief          : Main program body
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

/* Includes ------------------------------------------------------------------*/
#include "main.hh"

#include "String.hh"
#include "Font.hh"
#include "PortPin.hh"
#include "PushReleaseButton.hh"

#include "Screen.hh"
#include "Q10Keyboard.hh"
#include "EEPROM.hh"
#include "UserInterfaceManager.hh"

#include "SerialStm.hh"
#include "Led.hh"

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init();
static void MX_SPI1_Init();
static void MX_DMA_Init();
static void MX_USART1_Init();
static void MX_USART2_Init();
static void MX_I2C1_Init();
static void KeyboardTimerInit();

// Overriden interrupt callbacks
// void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

// Handlers
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart1_tx;
DMA_HandleTypeDef hdma_usart1_rx;

SPI_HandleTypeDef hspi1;
DMA_HandleTypeDef hdma_spi1_tx;
I2C_HandleTypeDef hi2c1;
TIM_HandleTypeDef htim2;


port_pin cs = { LCD_CS_GPIO_Port, LCD_CS_Pin };
port_pin dc = { LCD_DC_GPIO_Port, LCD_DC_Pin };
port_pin rst = { LCD_RST_GPIO_Port, LCD_RST_Pin };
port_pin bl = { LCD_BL_GPIO_Port, LCD_BL_Pin };

Screen screen(hspi1, cs, dc, rst, bl, Screen::Orientation::left_landscape);
Q10Keyboard* keyboard;
SerialStm* net_layer = nullptr;
UserInterfaceManager* ui_manager = nullptr;
EEPROM* eeprom = nullptr;

Led rx_led(LED1_Port, LED1_Pin, 10);
Led tx_led(LED2_Port, LED2_Pin, 10);

int main(void)
{
    // Reset of all peripherals, Initializes the Flash interface and the Systick.
    HAL_Init();

    // Initialize the GPIO prior to clock so that the io pins do not
    // start high and then go low when the clock config starts.
    // This prevents the screen from flashing on before its ready.
    MX_GPIO_Init();

    /* Configure the system clock */
    SystemClock_Config();

    // Not in use
    // IRQInit();

    // Init DMA for SPI1, NOTE- This MUST come before SPI1
    MX_DMA_Init();

    MX_USART1_Init();
    MX_USART2_Init();

    // Init the SPI for the screen
    MX_SPI1_Init();

    MX_I2C1_Init();

    // TODO remove
    // const uint8_t sz = 2;
    // uint8_t audio_data[sz] = { 0x19, 0b00000010 };
    // const uint8_t write_condition = 0x34 + 0;
    // // const uint8_t read_condition = 0x34 + 1;
    // HAL_StatusTypeDef audio_select = HAL_I2C_Master_Transmit(&hi2c1,
    //     write_condition, audio_data, sz, HAL_MAX_DELAY);
    // TODO remove to here

    // Reserve the first 32 bytes, and the total size is 255 bytes - 1k bits
    eeprom = new EEPROM(hi2c1, 32, 255);

    screen.Begin();

    // // Set the port pins and groups for the keyboard columns
    port_pin col_pins[Q10_COLS] =
    {
        { Q10_COL_1_GPIO_PORT, Q10_COL_1_PIN },
        { Q10_COL_2_GPIO_PORT, Q10_COL_2_PIN },
        { Q10_COL_3_GPIO_PORT, Q10_COL_3_PIN },
        { Q10_COL_4_GPIO_PORT, Q10_COL_4_PIN },
        { Q10_COL_5_GPIO_PORT, Q10_COL_5_PIN },
    };

    // Set the port pins and groups for the keyboard rows
    port_pin row_pins[Q10_ROWS] =
    {
        { Q10_ROW_1_GPIO_PORT, Q10_ROW_1_PIN },
        { Q10_ROW_2_GPIO_PORT, Q10_ROW_2_PIN },
        { Q10_ROW_3_GPIO_PORT, Q10_ROW_3_PIN },
        { Q10_ROW_4_GPIO_PORT, Q10_ROW_4_PIN },
        { Q10_ROW_5_GPIO_PORT, Q10_ROW_5_PIN },
        { Q10_ROW_6_GPIO_PORT, Q10_ROW_6_PIN },
        { Q10_ROW_7_GPIO_PORT, Q10_ROW_7_PIN },
    };

    // Initialize the keyboard timer
    KeyboardTimerInit();

    // Create the keyboard object
    keyboard = new Q10Keyboard(col_pins, row_pins, 500, 100, &htim2);

    // Initialize the keyboard
    keyboard->Begin();

    net_layer = new SerialStm(&huart2);

    ui_manager = new UserInterfaceManager(screen, *keyboard, *net_layer, *eeprom);


    uint32_t blink = 0;
    uint8_t test_message [] = "UI: Test\n";
    while (1)
    {
        ui_manager->Run();

        rx_led.Timeout();
        tx_led.Timeout();

        if (HAL_GetTick() > blink)
        {
            blink = HAL_GetTick() + 1000;
            HAL_GPIO_TogglePin(LED1_Port, LED1_Pin);
            HAL_UART_Transmit(&huart1, test_message, 10, 1000);
        }
    }

    return 0;
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {};

    /** Configure the main internal regulator output voltage
     */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /** Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
    {
        Error_Handler();
    }
    //   RCC_OscInitTypeDef RCC_OscInitStruct = {};
    //   RCC_ClkInitTypeDef RCC_ClkInitStruct = {};

    //   /** Configure the main internal regulator output voltage
    //   */
    //   __HAL_RCC_PWR_CLK_ENABLE();
    //   __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    //   /** Initializes the RCC Oscillators according to the specified parameters
    //   * in the RCC_OscInitTypeDef structure.
    //   */
    //   RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    //   RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    //   RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    //   RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    //   RCC_OscInitStruct.PLL.PLLM = 25;
    //   RCC_OscInitStruct.PLL.PLLN = 192;
    //   RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    //   RCC_OscInitStruct.PLL.PLLQ = 4;
    //   if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    //   {
    //     Error_Handler();
    //   }

    //   /** Initializes the CPU, AHB and APB buses clocks
    //   */
    //   RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
    //                               |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    //   RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    //   RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
    //   RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    //   RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    //   if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
    //   {
    //     Error_Handler();
    //   }
    //   __HAL_RCC_GPIOH_CLK_ENABLE();

        /**Configure the Systick interrupt time
         */
         // HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq() / 1000);

         // /**Configure the Systick
         //  */
         // HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

         // /* SysTick_IRQn interrupt configuration */
         // HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {};

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    HAL_GPIO_WritePin(Q10_TIMER_LED_PORT, Q10_TIMER_LED_PIN, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = Q10_TIMER_LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(Q10_TIMER_LED_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LED1_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED1_Port, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED1_Port, LED1_Pin, GPIO_PIN_SET);

    GPIO_InitStruct.Pin = LED2_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED2_Port, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED2_Port, LED2_Pin, GPIO_PIN_SET);

    GPIO_InitStruct.Pin = LED3_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED3_Port, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED3_Port, LED3_Pin, GPIO_PIN_SET);

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
}

static void MX_SPI1_Init(void)
{
    /* SPI1 parameter configuration*/
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_1LINE;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 10;
    if (HAL_SPI_Init(&hspi1) != HAL_OK)
    {
        Error_Handler();
    }
}

static void MX_DMA_Init()
{
    __HAL_RCC_DMA2_CLK_ENABLE();

    HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);

    /* DMA interrupt init */
    /* DMA2_Stream5_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(DMA2_Stream5_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream5_IRQn);
    /* DMA2_Stream7_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(DMA2_Stream7_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream7_IRQn);
}

static void MX_USART1_Init(void)
{
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_9B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_EVEN;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
        Error_Handler();
    }
}

static void MX_USART2_Init()
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
        Error_Handler();
    }
}

static void MX_I2C1_Init()
{
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK)
    {
        Error_Handler();
    }
}

// This will only be called when the rx_buffer is filled up
// void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart)
// {
//     if (huart->Instance == huart2.Instance)
//     {
//         net_layer->SetRxFlag();
//         // HAL_GPIO_TogglePin(USART2_RX_LED_PORT, USART2_RX_LED_PIN);
//     }
//     // HAL_UARTEx_ReceiveToIdle_IT(&huart2, rx_buff, 16);
// }

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart, uint16_t size)
{
    UNUSED(huart);
    UNUSED(size);
    net_layer->RxEvent();
    rx_led.On();
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart)
{
    UNUSED(huart);
    net_layer->TxEvent();
    tx_led.On();
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef* hspi)
{
    UNUSED(hspi);
    screen.ReleaseSPI();
}

static void KeyboardTimerInit()
{
    // Timer
    __HAL_RCC_TIM2_CLK_ENABLE();

    // Configure the timer
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 1600;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 1000;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
    {
        Error_Handler();
    }

    TIM_ClockConfigTypeDef sClockSourceConfig = {};
    TIM_MasterConfigTypeDef sMasterConfig = {};

    // Set the timer callback function
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
    {
        Error_Handler();
    }

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim)
{
    // Keyboard timer callback!
    if (htim->Instance == TIM2)
    {
        // Poll the keyboard and have the keys saved in the internal rx_buffer
        // keyboard->Read();
    }
}

// We aren't using this for interrupts yet
#if 0
static void IRQInit()
{
    GPIO_InitTypeDef gpio_init_struct;
    gpio_init_struct.Pin = Q10_INTERRUPT_PIN;
    gpio_init_struct.Mode = GPIO_MODE_IT_RISING;
    gpio_init_struct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(Q10_INTERRUPT_PORT, &gpio_init_struct);

    // EXTI_HandleTypeDef exti_typedef;
    // exti_typedef.Line = EXTI_LINE_0;
    // exti_typedef.PendingCallback = &test;

    // EXTI_ConfigTypeDef exti_config;
    // exti_config.Line = EXTI_LINE_0;
    // exti_config.Trigger = EXTI_TRIGGER_RISING;
    // exti_config.GPIOSel = EXTI_GPIOA;
    // HAL_EXTI_SetConfigLine(&exti_typedef, &exti_config);

    HAL_NVIC_SetPriority(EXTI0_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == Q10_INTERRUPT_PIN && !keyboard_lock)
    {
        // keyboard_lock = true;
        // HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        // if (ui_manager != nullptr)
        //     ui_manager->ForceRedraw();

        // keyboard_lock = false;
    }
}
#endif

// TODO move to helper function
// TODO add error messaging
/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
    // if (HAL_SPI_Init(&hspi1) == HAL_OK)
    // {
    //     screen.FillScreen(C_BLACK);
    //     screen.DrawText(2, 2, "There was an error", font11x16, C_WHITE, C_BLACK);
    //     screen.DrawRectangle(2, 18, 2 + 11*18, 18, 1, C_WHITE);
    // }
    while (1)
    {
        __enable_irq();
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_6);
        HAL_Delay(1000);
        __disable_irq();
    }
    /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
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
