/* USER CODE BEGIN Header */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "mgmt.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdlib.h>
#include <string.h>
#include "stm32f0xx_hal_def.h"
#include "uart_stream.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

// TODO add messages that to the monitor

#define CPU_FREQ 48000000

#define BAUD 115200
#define UI_RECEIVE_BUFF_SZ 20
#define UI_TRANSMIT_BUFF_SZ UI_RECEIVE_BUFF_SZ * 2
#define NET_RECEIVE_BUFF_SZ 1024
#define NET_TRANSMIT_BUFF_SZ NET_RECEIVE_BUFF_SZ * 2
#define COMMAND_BUFF_SZ 16

#define BTN_PRESS_TIMEOUT 5000
#define BTN_DEBOUNCE_TIMEOUT 50
#define BTN_WAIT_TIMEOUT 1000
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

    enum State state;
    enum State next_state;

    uart_stream_t usb_stream;
    uart_stream_t net_stream;
    uart_stream_t ui_stream;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
static void Usart1_Net_Upload_Runnning_Debug_Reset(void);
static void Usart1_UI_Upload_Init(void);
static void CancelAllUart();
static void NetBootloaderMode();
static void NetNormalMode();
static void NetHoldInReset();
static void UIBootloaderMode();
static void UINormalMode();
static void UIHoldInReset();
static void UIHoldInReset();
static void NetUpload();
static void UIUpload();
static void RunningMode();
static void DebugMode();


/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint32_t wait_timeout = 0;

void HAL_GPIO_EXTI_Callback(uint16_t gpio_pin)
{
    if (gpio_pin == USB_RTS_Pin)
    {
        if ((state == Net_Upload || state == UI_Upload) && usb_stream.idle_receive)
        {
            state = Reset;
        }
        return;
    }

    // Gets here then then it was a button that was pressed
    state = Waiting;
    wait_timeout = HAL_GetTick() + BTN_WAIT_TIMEOUT;
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart, uint16_t size)
{
    // There are 3 conditions that call this function.

    // 1. Half complete -- The rx buffer is half full
    // 2. rx complete -- the rx buffer is full
    // 3. idle  -- Nothing has been received in awhile

    // Need to have as separate if statements so we can loop back properly
    // Which doesn't make any sense to me, but it makes it work.
    if (huart->Instance == net_stream.from_uart->Instance)
    {
        HandleRx(&net_stream, size);
    }

    if (huart->Instance == ui_stream.from_uart->Instance)
    {
        HandleRx(&ui_stream, size);
    }

    if (huart->Instance == usb_stream.from_uart->Instance)
    {
        HandleRx(&usb_stream, size);
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart)
{
    // Need to have as separate if statements so we can loop back properly
    // Which doesn't make any sense to me, but it makes it work.
    if (huart->Instance == net_stream.to_uart->Instance)
    {
        net_stream.tx_free = 1;
        net_stream.last_transmission_time = HAL_GetTick();
    }

    if (huart->Instance == ui_stream.to_uart->Instance)
    {
        ui_stream.tx_free = 1;
        ui_stream.last_transmission_time = HAL_GetTick();
    }

    if (huart->Instance == usb_stream.to_uart->Instance)
    {
        usb_stream.tx_free = 1;
        usb_stream.last_transmission_time = HAL_GetTick();
    }
}

extern inline void CancelAllUart()
{
    CancelUart(&ui_stream);
    CancelUart(&net_stream);
    CancelUart(&usb_stream);
}

void NetBootloaderMode()
{
    // Bring the boot low for esp, bootloader mode (0)
    HAL_GPIO_WritePin(NET_BOOT_GPIO_Port, NET_BOOT_Pin, GPIO_PIN_RESET);

    // Power cycle
    HAL_GPIO_WritePin(NET_RST_GPIO_Port, NET_RST_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(NET_RST_GPIO_Port, NET_RST_Pin, GPIO_PIN_SET);
}

void NetNormalMode()
{
    HAL_GPIO_WritePin(NET_BOOT_GPIO_Port, NET_BOOT_Pin, GPIO_PIN_SET);

    // Power cycle
    HAL_GPIO_WritePin(NET_RST_GPIO_Port, NET_RST_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(NET_RST_GPIO_Port, NET_RST_Pin, GPIO_PIN_SET);
}

void NetHoldInReset()
{
    // Reset
    HAL_GPIO_WritePin(NET_RST_GPIO_Port, NET_RST_Pin, GPIO_PIN_RESET);
}

void UIBootloaderMode()
{
    // Normal boot mode (boot0 = 1 and boot1 = 0)
    HAL_GPIO_WritePin(UI_BOOT0_GPIO_Port, UI_BOOT0_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(UI_BOOT1_GPIO_Port, UI_BOOT1_Pin, GPIO_PIN_RESET);

    // Power cycle
    HAL_GPIO_WritePin(UI_RST_GPIO_Port, UI_RST_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(UI_RST_GPIO_Port, UI_RST_Pin, GPIO_PIN_SET);
}

void UINormalMode()
{
    // Normal boot mode (boot0 = 0 and boot1 = 1)
    HAL_GPIO_WritePin(UI_BOOT0_GPIO_Port, UI_BOOT0_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(UI_BOOT1_GPIO_Port, UI_BOOT1_Pin, GPIO_PIN_SET);

    // Power cycle
    HAL_GPIO_WritePin(UI_RST_GPIO_Port, UI_RST_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(UI_RST_GPIO_Port, UI_RST_Pin, GPIO_PIN_SET);
}

void UIHoldInReset()
{
    HAL_GPIO_WritePin(UI_RST_GPIO_Port, UI_RST_Pin, GPIO_PIN_RESET);
}

void NetUpload()
{
    NetHoldInReset();
    UIHoldInReset();

    HAL_GPIO_WritePin(LEDA_R_GPIO_Port, LEDA_R_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDA_G_GPIO_Port, LEDA_G_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDA_B_GPIO_Port, LEDA_B_Pin, GPIO_PIN_SET);

    HAL_GPIO_WritePin(LEDB_R_GPIO_Port, LEDB_R_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDB_G_GPIO_Port, LEDB_G_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDB_B_GPIO_Port, LEDB_B_Pin, GPIO_PIN_RESET);

    CancelAllUart();

    // Deinit usb
    HAL_UART_DeInit(usb_stream.from_uart);

    // Init huart3
    Usart1_Net_Upload_Runnning_Debug_Reset();

    usb_stream.to_uart = &huart3;
    usb_stream.rx_buffer_size = NET_RECEIVE_BUFF_SZ;
    usb_stream.tx_buffer_size = NET_TRANSMIT_BUFF_SZ;

    InitUartStreamParameters(&usb_stream);
    InitUartStreamParameters(&net_stream);

    StartUartReceive(&usb_stream);
    StartUartReceive(&net_stream);

    // Put net into bootloader mode
    UIHoldInReset();
    NetBootloaderMode();

    HAL_GPIO_WritePin(LEDA_R_GPIO_Port, LEDA_R_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDA_G_GPIO_Port, LEDA_G_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDA_B_GPIO_Port, LEDA_B_Pin, GPIO_PIN_SET);

    HAL_GPIO_WritePin(LEDB_R_GPIO_Port, LEDB_R_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDB_G_GPIO_Port, LEDB_G_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LEDB_B_GPIO_Port, LEDB_B_Pin, GPIO_PIN_SET);

    state = Net_Upload;

    HAL_Delay(500);

    // Send a ready message
    HAL_UART_Transmit(usb_stream.from_uart, READY, 1, HAL_MAX_DELAY);

    usb_stream.last_transmission_time = HAL_GetTick();
    net_stream.last_transmission_time = HAL_GetTick();


    while (state == Net_Upload)
    {
        HandleTx(&usb_stream, &state);
        HandleTx(&net_stream, &state);
    }
}

void UIUpload()
{
    HAL_GPIO_WritePin(LEDB_R_GPIO_Port, LEDB_R_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDB_G_GPIO_Port, LEDB_G_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDB_B_GPIO_Port, LEDB_B_Pin, GPIO_PIN_SET);

    HAL_GPIO_WritePin(LEDA_R_GPIO_Port, LEDA_R_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LEDA_G_GPIO_Port, LEDA_G_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDA_B_GPIO_Port, LEDA_B_Pin, GPIO_PIN_SET);

    CancelAllUart();

    // Init uart3 for UI upload
    HAL_UART_DeInit(usb_stream.from_uart);

    // Init huart3
    Usart1_UI_Upload_Init();

    usb_stream.to_uart = &huart2;
    usb_stream.rx_buffer_size = UI_RECEIVE_BUFF_SZ;
    usb_stream.tx_buffer_size = UI_TRANSMIT_BUFF_SZ;

    InitUartStreamParameters(&usb_stream);
    InitUartStreamParameters(&ui_stream);

    StartUartReceive(&usb_stream);
    StartUartReceive(&ui_stream);

    NetHoldInReset();
    UIBootloaderMode();

    HAL_GPIO_WritePin(LEDB_R_GPIO_Port, LEDB_R_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDB_G_GPIO_Port, LEDB_G_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDB_B_GPIO_Port, LEDB_B_Pin, GPIO_PIN_SET);

    HAL_GPIO_WritePin(LEDA_R_GPIO_Port, LEDA_R_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LEDA_G_GPIO_Port, LEDA_G_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDA_B_GPIO_Port, LEDA_B_Pin, GPIO_PIN_SET);

    state = UI_Upload;

    HAL_Delay(500);

    // Send a ready message
    HAL_UART_Transmit(usb_stream.from_uart, READY, 1, HAL_MAX_DELAY);

    usb_stream.last_transmission_time = HAL_GetTick();
    ui_stream.last_transmission_time = HAL_GetTick();

    while (state == UI_Upload)
    {
        HandleTx(&usb_stream, &state);
        HandleTx(&ui_stream, &state);
    }
}

void RunningMode()
{
    // Set LEDS for ui
    HAL_GPIO_WritePin(LEDB_R_GPIO_Port, LEDB_R_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LEDB_G_GPIO_Port, LEDB_G_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDB_B_GPIO_Port, LEDB_B_Pin, GPIO_PIN_RESET);

    // Set LEDS for net
    HAL_GPIO_WritePin(LEDA_R_GPIO_Port, LEDA_R_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LEDA_G_GPIO_Port, LEDA_G_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDA_B_GPIO_Port, LEDA_B_Pin, GPIO_PIN_RESET);

    CancelAllUart();

    // De-init usb from uart for running mode
    HAL_UART_DeInit(usb_stream.from_uart);

    // Init huart3
    Usart1_Net_Upload_Runnning_Debug_Reset();

    usb_stream.to_uart = &huart3;
    usb_stream.rx_buffer_size = COMMAND_BUFF_SZ;
    usb_stream.tx_buffer_size = COMMAND_BUFF_SZ;

    InitUartStreamParameters(&usb_stream);
    StartUartReceive(&usb_stream);

    UINormalMode();
    uint32_t timeout = HAL_GetTick() + 3000;
    while (HAL_GetTick() < timeout &&
        HAL_GPIO_ReadPin(ADC_UI_STAT_GPIO_Port, ADC_UI_STAT_Pin) != GPIO_PIN_SET)
    {
        // Stay here until the UI is finished booting
        HAL_Delay(10);
    }

    NetNormalMode();
    // Refresh the timeout
    timeout = HAL_GetTick() + 3000;
    while (HAL_GetTick() < timeout &&
        HAL_GPIO_ReadPin(ADC_NET_STAT_GPIO_Port, ADC_NET_STAT_Pin) != GPIO_PIN_SET)
    {
        // Stay here until the Net is done booting
        HAL_Delay(10);
    }

    state = Running;
    // char hello[] = {'h', 'e', 'l', 'l', 'o'};
    // unsigned long long next_debug_msg = 0;
    while (state == Running)
    {
        HandleCommands(&usb_stream, &huart1, &state);

        // if (HAL_GetTick() > next_debug_msg)
        // {
        //     next_debug_msg = HAL_GetTick() + 1000;
        //     HAL_UART_Transmit(usb_stream.from_uart, hello, 5, HAL_MAX_DELAY);
        //     HAL_GPIO_TogglePin(LEDA_G_GPIO_Port, LEDA_G_Pin);
        // }
    }
}

void DebugMode()
{
    // Set LEDS for ui
    HAL_GPIO_WritePin(LEDB_R_GPIO_Port, LEDB_R_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LEDB_G_GPIO_Port, LEDB_G_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDB_B_GPIO_Port, LEDB_B_Pin, GPIO_PIN_SET);

    // Set LEDS for net
    HAL_GPIO_WritePin(LEDA_R_GPIO_Port, LEDA_R_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LEDA_G_GPIO_Port, LEDA_G_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDA_B_GPIO_Port, LEDA_B_Pin, GPIO_PIN_SET);

    CancelAllUart();

    // Init uart3 for UI upload
    HAL_UART_DeInit(usb_stream.from_uart);

    // Init huart3
    Usart1_Net_Upload_Runnning_Debug_Reset();

    // usb_stream.to_uart = &huart3;
    usb_stream.rx_buffer_size = NET_RECEIVE_BUFF_SZ;
    usb_stream.tx_buffer_size = NET_TRANSMIT_BUFF_SZ;
    ui_stream.rx_buffer_size = UI_RECEIVE_BUFF_SZ;
    ui_stream.tx_buffer_size = UI_TRANSMIT_BUFF_SZ;
    net_stream.rx_buffer_size = NET_RECEIVE_BUFF_SZ;
    net_stream.tx_buffer_size = NET_TRANSMIT_BUFF_SZ;

    InitUartStreamParameters(&usb_stream);
    InitUartStreamParameters(&ui_stream);
    InitUartStreamParameters(&net_stream);
    StartUartReceive(&usb_stream);
    StartUartReceive(&ui_stream);
    StartUartReceive(&net_stream);

    HAL_Delay(100);

    UINormalMode();

    uint32_t timeout = HAL_GetTick() + 3000;
    while (HAL_GetTick() < timeout &&
        HAL_GPIO_ReadPin(ADC_UI_STAT_GPIO_Port, ADC_UI_STAT_Pin) != GPIO_PIN_SET)
    {
        // Stay here until the UI is finished booting
        HAL_Delay(10);
    }

    NetNormalMode();
    // Refresh the timeout
    timeout = HAL_GetTick() + 3000;
    while (HAL_GetTick() < timeout &&
        HAL_GPIO_ReadPin(ADC_NET_STAT_GPIO_Port, ADC_NET_STAT_Pin) != GPIO_PIN_SET)
    {
        // Stay here until the Net is done booting
        HAL_Delay(10);
    }

    state = Debug_Running;
    while (state == Debug_Running)
    {
        HandleCommands(&usb_stream, &huart1, &state);
        HandleTx(&ui_stream, &state);
        HandleTx(&net_stream, &state);
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
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_TIM3_Init();
    MX_USART2_UART_Init();
    MX_USART3_UART_Init();
    Usart1_Net_Upload_Runnning_Debug_Reset();
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

    // TODO move these into functions for starting ui/net upload
    // Init uart structures

    usb_stream.rx_buffer_size = NET_RECEIVE_BUFF_SZ;
    usb_stream.tx_buffer_size = NET_TRANSMIT_BUFF_SZ;

    usb_stream.rx_buffer = (uint8_t*)malloc(usb_stream.rx_buffer_size * sizeof(uint8_t));
    usb_stream.tx_buffer = (uint8_t*)malloc(usb_stream.tx_buffer_size * sizeof(uint8_t));

    net_stream.rx_buffer_size = NET_RECEIVE_BUFF_SZ;
    net_stream.tx_buffer_size = NET_TRANSMIT_BUFF_SZ;

    net_stream.rx_buffer = (uint8_t*)malloc(net_stream.rx_buffer_size * sizeof(uint8_t));
    net_stream.tx_buffer = (uint8_t*)malloc(net_stream.tx_buffer_size * sizeof(uint8_t));

    ui_stream.rx_buffer_size = UI_RECEIVE_BUFF_SZ;
    ui_stream.tx_buffer_size = UI_TRANSMIT_BUFF_SZ;

    ui_stream.rx_buffer = (uint8_t*)malloc(ui_stream.rx_buffer_size * sizeof(uint8_t));
    ui_stream.tx_buffer = (uint8_t*)malloc(ui_stream.tx_buffer_size * sizeof(uint8_t));

    usb_stream.from_uart = &huart1;
    usb_stream.to_uart = &huart1;
    InitUartStreamParameters(&usb_stream);

    net_stream.from_uart = &huart3;
    net_stream.to_uart = &huart1;
    InitUartStreamParameters(&net_stream);

    ui_stream.from_uart = &huart2;
    ui_stream.to_uart = &huart1;
    InitUartStreamParameters(&ui_stream);

    // NOTE these need to be set to 1 so when we choose a mode
    // It can properly de-init and re-init the huart modes
    // Its weird but hey, its what works with HAL
    usb_stream.is_listening = 1;
    net_stream.is_listening = 1;
    ui_stream.is_listening = 1;

    state = Waiting;
    next_state = Reset;
    while (1)
    {
        if (state == Waiting)
        {
            state = next_state;
            next_state = Waiting;
            while (HAL_GetTick() < wait_timeout)
            {
                __NOP();
            }
        }
        else if (state == Reset)
        {
            RunningMode();
        }
        else if (state == Debug_Reset)
        {
            DebugMode();
        }
        else if (state == UI_Upload_Reset)
        {
            UIUpload();
        }
        else if (state == Net_Upload_Reset)
        {
            NetUpload();
        }
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
    RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
    RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
    RCC_PeriphCLKInitTypeDef PeriphClkInit = { 0 };

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
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
        | RCC_CLOCKTYPE_PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
    {
        Error_Handler();
    }
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_USART2;
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

    TIM_SlaveConfigTypeDef sSlaveConfig = { 0 };
    TIM_MasterConfigTypeDef sMasterConfig = { 0 };

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
  * @brief USART3 Initialization Function
  *     Setting for net upload, running mode, debug mode, and default reset
  * @param None
  * @retval None
  */
static void Usart1_Net_Upload_Runnning_Debug_Reset(void)
{

    /* USER CODE BEGIN USART1_Init 0 */
    /* USER CODE END USART1_Init 0 */

    /* USER CODE BEGIN USART1_Init 1 */
    /* USER CODE END USART1_Init 1 */
    huart1.Instance = USART1;
    huart1.Init.BaudRate = BAUD;
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
  * @brief USART1 Initialization Function
  *     Specific initialization for UI upload transmission and receives
  * @param None
  * @retval None
  */
static void Usart1_UI_Upload_Init(void)
{

    /* USER CODE BEGIN USART1_Init 0 */
    /* USER CODE END USART1_Init 0 */

    /* USER CODE BEGIN USART1_Init 1 */
    /* USER CODE END USART1_Init 1 */
    huart1.Instance = USART1;
    huart1.Init.BaudRate = BAUD;
    huart1.Init.WordLength = UART_WORDLENGTH_9B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_EVEN;
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
    huart2.Init.BaudRate = BAUD;
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
    huart3.Init.BaudRate = BAUD;
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
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    /* USER CODE BEGIN MX_GPIO_Init_1 */
    /* USER CODE END MX_GPIO_Init_1 */

      /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOA, LEDB_R_Pin | LEDA_R_Pin | LEDA_G_Pin | UI_RST_Pin, GPIO_PIN_RESET);

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOB, LEDA_B_Pin | LEDB_G_Pin | LEDB_B_Pin | UI_BOOT0_Pin
        | NET_RST_Pin | NET_BOOT_Pin | UI_BOOT1_Pin, GPIO_PIN_RESET);

    /*Configure GPIO pins : BTN_RST_Pin BTN_UI_Pin BTN_NET_Pin */
    GPIO_InitStruct.Pin = BTN_RST_Pin | BTN_UI_Pin | BTN_NET_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /*Configure GPIO pins : ADC_UI_STAT_Pin ADC_NET_STAT_Pin */
    GPIO_InitStruct.Pin = ADC_UI_STAT_Pin | ADC_NET_STAT_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /*Configure GPIO pins : LEDB_R_Pin LEDA_R_Pin LEDA_G_Pin UI_RST_Pin */
    GPIO_InitStruct.Pin = LEDB_R_Pin | LEDA_R_Pin | LEDA_G_Pin | UI_RST_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /*Configure GPIO pins : LEDA_B_Pin LEDB_G_Pin LEDB_B_Pin UI_BOOT0_Pin
                             NET_RST_Pin NET_BOOT_Pin UI_BOOT1_Pin */
    GPIO_InitStruct.Pin = LEDA_B_Pin | LEDB_G_Pin | LEDB_B_Pin | UI_BOOT0_Pin
        | NET_RST_Pin | NET_BOOT_Pin | UI_BOOT1_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /*Configure GPIO pin : USB_RTS_Pin */
    GPIO_InitStruct.Pin = USB_RTS_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(USB_RTS_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pin : USB_DTR_Pin */
    GPIO_InitStruct.Pin = USB_DTR_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(USB_DTR_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pin : MCLK_Pin */
    GPIO_InitStruct.Pin = MCLK_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF0_MCO;
    HAL_GPIO_Init(MCLK_GPIO_Port, &GPIO_InitStruct);

    /* EXTI interrupt init*/
    HAL_NVIC_SetPriority(EXTI0_1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);

    HAL_NVIC_SetPriority(EXTI4_15_IRQn, 1, 0);
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
    // Set LEDS for ui
    HAL_GPIO_WritePin(LEDB_R_GPIO_Port, LEDB_R_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDB_G_GPIO_Port, LEDB_G_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDB_B_GPIO_Port, LEDB_B_Pin, GPIO_PIN_SET);

    // Set LEDS for net
    HAL_GPIO_WritePin(LEDA_R_GPIO_Port, LEDA_R_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDA_G_GPIO_Port, LEDA_G_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDA_B_GPIO_Port, LEDA_B_Pin, GPIO_PIN_SET);
    uint32_t blink_r = 0;
    state = Error;
    while (state == Error)
    {
        if (HAL_GetTick() > blink_r)
        {
            HAL_GPIO_TogglePin(LEDA_R_GPIO_Port, LEDA_R_Pin);
            HAL_GPIO_TogglePin(LEDB_R_GPIO_Port, LEDB_R_Pin);
            blink_r = HAL_GetTick() + 200;
        }
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
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */