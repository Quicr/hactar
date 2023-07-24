/* USER CODE BEGIN Header */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdint-gcc.h>
#include <stdlib.h>
#include <string.h>
#include "stm32f0xx_hal_def.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define BAUD 115200
#define UI_RECEIVE_BUFF_SZ 20
#define UI_TRANSMIT_BUFF_SZ UI_RECEIVE_BUFF_SZ * 2
#define NET_RECEIVE_BUFF_SZ 1024
#define NET_TRANSMIT_BUFF_SZ NET_RECEIVE_BUFF_SZ * 2
#define TRANSMISSION_TIMEOUT 5000
#define COMMAND_BUFF_SZ 16

#define BTN_PRESS_TIMEOUT 5000
#define BTN_DEBOUNCE_TIMEOUT 50
#define BTN_WAIT_TIMEOUT 1000

// Structure for uart copy
typedef struct
{
  UART_HandleTypeDef* from_uart;
  UART_HandleTypeDef* to_uart;
  uint8_t* rx_buffer;
  uint16_t rx_buffer_size;
  uint8_t* tx_buffer;
  uint16_t tx_buffer_size;
  uint16_t rx_read;
  uint16_t tx_write;
  uint16_t tx_read;
  uint8_t tx_read_overflow;
  uint8_t tx_free;
  uint16_t pending_bytes;
  uint8_t idle_receive;
  uint32_t last_transmission_time;
  uint8_t has_received;
  uint8_t is_listening;
  uint8_t command_complete;
} uart_stream_t;

typedef struct
{
  uint16_t pin;
  uint32_t pressed_timeout;
  uint32_t debounce_timeout;
} button_it_t;

enum State
{
  Error,
  Waiting,
  Reset,
  Running,
  UI_Upload_Reset,
  UI_Upload,
  Net_Upload_Reset,
  Net_Upload,
  Debug_Reset,
  Debug_Running
};
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;
UART_HandleTypeDef huart2_normal;
UART_HandleTypeDef huart3_normal;
UART_HandleTypeDef huart2_upload;
UART_HandleTypeDef huart3_upload;
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
/* USER CODE BEGIN PFP */
static void Usart3_Net_Upload_Runnning_Debug_Reset(void);
static void Usart3_UI_Upload_Init(void);
extern inline void HandleRx(uart_stream_t* uart_stream, uint16_t num_received);
extern inline void HandleTx(uart_stream_t* uart_stream);
extern inline void HandleCommands(uart_stream_t* uart_stream);
extern inline void InitUartStreamParameters(uart_stream_t* uart_stream);
void InitBtnStruct(button_it_t* btn, uint16_t pin);
extern inline void CancelUart(uart_stream_t* uart_stream);
extern inline void CancelAllUart();
extern inline void StartUartReceive(uart_stream_t* uart_stream);
void NetBootloaderMode();
void NetNormalMode();
void NetHoldInReset();
void UIBootloaderMode();
void UINormalMode();
void UIHoldInReset();
void UIHoldInReset();
void NetUpload();
void UIUpload();
void RunningMode();
void DebugMode();


/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uart_stream_t usb_stream;
uart_stream_t net_stream;
uart_stream_t ui_stream;

uint16_t send_bytes = 0;

button_it_t rst_btn;
button_it_t ui_btn;
button_it_t net_btn;

enum State state;
enum State next_state;
uint32_t wait_timeout = 0;

// Commands
const char ui_upload_cmd [] = "ui_upload";
const char net_upload_cmd [] = "net_upload";
const char debug_cmd [] = "debug";
const char reset_cmd [] = "reset";

uint8_t CheckForDebugMode()
{
  uint32_t current_tick = HAL_GetTick();
  if (current_tick < rst_btn.pressed_timeout &&
    current_tick < ui_btn.pressed_timeout &&
    current_tick < net_btn.pressed_timeout)
  {
    state = Debug_Reset;
    return 1;
  }

  return 0;
}

void HAL_GPIO_EXTI_Callback(uint16_t gpio_pin)
{
  if (gpio_pin == RTS_Pin)
  {
    if ((state == Net_Upload || state == UI_Upload) && usb_stream.idle_receive)
    {
      state = Reset;
    }
    return;
  }
  else if ((gpio_pin == rst_btn.pin) && HAL_GetTick() > rst_btn.debounce_timeout)
  {
    rst_btn.debounce_timeout = HAL_GetTick() + BTN_DEBOUNCE_TIMEOUT;
    rst_btn.pressed_timeout = HAL_GetTick() + BTN_PRESS_TIMEOUT;
    if (CheckForDebugMode()) return;
    next_state = Reset;
  }
  else if (gpio_pin == ui_btn.pin && HAL_GetTick() > ui_btn.debounce_timeout)
  {
    ui_btn.debounce_timeout = HAL_GetTick() + BTN_DEBOUNCE_TIMEOUT;
    ui_btn.pressed_timeout = HAL_GetTick() + BTN_PRESS_TIMEOUT;
    if (CheckForDebugMode()) return;
    next_state = UI_Upload_Reset;
  }
  else if (gpio_pin == net_btn.pin && HAL_GetTick() > net_btn.debounce_timeout)
  {
    net_btn.debounce_timeout = HAL_GetTick() + BTN_DEBOUNCE_TIMEOUT;
    net_btn.pressed_timeout = HAL_GetTick() + BTN_PRESS_TIMEOUT;
    if (CheckForDebugMode()) return;
    next_state = Net_Upload_Reset;
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

extern inline void HandleRx(uart_stream_t* rx_stream, uint16_t num_received)
{
  // Calculate the number of bytes have occurred since the last event
  uint16_t num_bytes = num_received - rx_stream->rx_read;

  // Faster than putting a check inside of the copy loop since this is only
  // checked once per rx event.
  if (rx_stream->tx_write + num_bytes > rx_stream->tx_buffer_size)
  {
    // Fill in the remaining space and circle around
    while (rx_stream->rx_read < num_received && rx_stream->tx_write < rx_stream->tx_buffer_size)
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
  if (rx_stream->rx_read == rx_stream->rx_buffer_size)
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
}

extern inline void HandleTx(uart_stream_t* uart_stream)
{
  if (uart_stream->pending_bytes > 0 && uart_stream->tx_free)
  {
    if (uart_stream->pending_bytes >= uart_stream->rx_buffer_size || uart_stream->idle_receive || uart_stream->tx_read_overflow)
    {
      send_bytes = uart_stream->rx_buffer_size;

      // Should only occur on an idle
      if (uart_stream->idle_receive || uart_stream->tx_read_overflow)
      {
        send_bytes = uart_stream->pending_bytes;
        uart_stream->tx_read_overflow = 0;
      }

      if (send_bytes > uart_stream->tx_buffer_size - uart_stream->tx_read)
      {
        send_bytes = uart_stream->tx_buffer_size - uart_stream->tx_read;
        uart_stream->tx_read_overflow = 1;
      }
      else if (send_bytes > uart_stream->pending_bytes)
      {
        send_bytes = uart_stream->pending_bytes;
      }

      // Technically this should be lower, but it makes it work for the ui stream...
      // because... good question
      if (uart_stream->idle_receive && uart_stream->pending_bytes == 0)
      {
        uart_stream->idle_receive = 0;
      }

      uart_stream->tx_free = 0;
      HAL_UART_Transmit_DMA(uart_stream->to_uart, (uart_stream->tx_buffer + uart_stream->tx_read), send_bytes);
      uart_stream->pending_bytes -= send_bytes;
      uart_stream->tx_read += send_bytes;
    }

    if (uart_stream->tx_read >= uart_stream->tx_buffer_size)
    {
      uart_stream->tx_read = 0;
    }
  }

  if (HAL_GetTick() > uart_stream->last_transmission_time + TRANSMISSION_TIMEOUT &&
    state != Debug_Running &&
    state != Reset &&
    state != Running)
  {
    // Clean up and return to reset mode
    state = Reset;
  }
}

extern inline void HandleCommands(uart_stream_t* uart_stream)
{
  if (uart_stream->pending_bytes > 0)
  {
    if (uart_stream->pending_bytes >= uart_stream->rx_buffer_size || uart_stream->idle_receive || uart_stream->tx_read_overflow)
    {
      uart_stream->has_received = 1;
      send_bytes = uart_stream->rx_buffer_size;

      // Should only occur on an idle
      if (uart_stream->idle_receive || uart_stream->tx_read_overflow)
      {
        send_bytes = uart_stream->pending_bytes;
        uart_stream->tx_read_overflow = 0;
      }

      if (send_bytes > uart_stream->tx_buffer_size - uart_stream->tx_read)
      {
        send_bytes = uart_stream->tx_buffer_size - uart_stream->tx_read;
        uart_stream->tx_read_overflow = 1;
      }
      else if (send_bytes > uart_stream->pending_bytes)
      {
        send_bytes = uart_stream->pending_bytes;
      }

      uart_stream->pending_bytes -= send_bytes;
      uart_stream->tx_read += send_bytes;

      if (uart_stream->idle_receive || uart_stream->pending_bytes == 0)
      {
        // End of transmission
        uart_stream->idle_receive = 0;
        uart_stream->command_complete = 1;
      }
    }

    if (uart_stream->tx_read >= uart_stream->tx_buffer_size)
    {
      uart_stream->tx_read = 0;
    }
  }

  // BUG, sometimes this gets stuck because some bytes come in that make
  // an improper message and it doesn't reset or something.
  // Need to try with debugger
  if (uart_stream->command_complete ||
    (HAL_GetTick() > uart_stream->last_transmission_time + TRANSMISSION_TIMEOUT
      && uart_stream->has_received))
  {
    // Safety measure to ensure we don't read illegal memory
    uart_stream->tx_buffer[uart_stream->tx_buffer_size - 1] = '\0';

    if (strcmp((const char*)uart_stream->tx_buffer, ui_upload_cmd) == 0)
    {
      state = UI_Upload_Reset;
      ui_stream.is_listening = 1;
    }
    else if (strcmp((const char*)uart_stream->tx_buffer, net_upload_cmd) == 0)
    {
      state = Net_Upload_Reset;

      // NOTE do not remove, for reasons that are beyond me, we need to
      // set this to is_listening so that the we can call
      // HAL_UART_AbortReceive_IT in the CancelUart.
      // I honestly don't understand why this is required only on net
      // but I am positive there is some wonky stuff happening in the HAL lib
      net_stream.is_listening = 1;
    }
    else if (strcmp((const char*)uart_stream->tx_buffer, debug_cmd) == 0)
    {
      state = Debug_Reset;
      net_stream.is_listening = 1;
    }
    else if (strcmp((const char*)uart_stream->tx_buffer, reset_cmd) == 0)
    {
      state = Reset;
      net_stream.is_listening = 1;
    }

    uart_stream->tx_write = 0;
    uart_stream->command_complete = 0;
    uart_stream->has_received = 0;
  }
}

extern inline void InitUartStreamParameters(uart_stream_t* uart_stream)
{
  uart_stream->rx_read = 0;
  uart_stream->tx_write = 0;
  uart_stream->tx_read = 0;
  uart_stream->tx_read_overflow = 0;
  uart_stream->tx_free = 1;
  uart_stream->pending_bytes = 0;
  uart_stream->idle_receive = 0;
  uart_stream->last_transmission_time = HAL_GetTick();
  uart_stream->has_received = 0;
  uart_stream->command_complete = 0;
}

void InitBtnStruct(button_it_t* btn, uint16_t pin)
{
  btn->pin = pin;
  btn->debounce_timeout = 0;
  btn->pressed_timeout = 0;
}

extern inline void CancelUart(uart_stream_t* uart_stream)
{
  // if (!uart_stream->is_listening) return;

  HAL_UART_Abort_IT(uart_stream->from_uart);
  uart_stream->is_listening = 0;
}

extern inline void CancelAllUart()
{
  CancelUart(&ui_stream);
  CancelUart(&net_stream);
  CancelUart(&usb_stream);
}

extern inline void StartUartReceive(uart_stream_t* uart_stream)
{
  if (uart_stream->is_listening)
  {
    // If this occurs we have a bug
    Error_Handler();
  }

  uint8_t attempt = 0;
  while (attempt++ != 10 &&
    HAL_OK != HAL_UARTEx_ReceiveToIdle_DMA(uart_stream->from_uart, uart_stream->rx_buffer, uart_stream->rx_buffer_size))
  {
    // Make sure the uart is cancelled, sometimes it appears to not cancel
    CancelUart(uart_stream);
  }

  if (attempt >= 10)
  {
    Error_Handler();
  }

  uart_stream->is_listening = 1;
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
  Usart3_Net_Upload_Runnning_Debug_Reset();

  usb_stream.to_uart = &huart1;
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

  while (state == Net_Upload)
  {
    HandleTx(&usb_stream);
    HandleTx(&net_stream);
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
  Usart3_UI_Upload_Init();

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

  while (state == UI_Upload)
  {
    HandleTx(&usb_stream);
    HandleTx(&ui_stream);
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

  // Init uart3 for UI upload
  HAL_UART_DeInit(usb_stream.from_uart);

  // Init huart3
  Usart3_Net_Upload_Runnning_Debug_Reset();

  // usb_stream.to_uart = &huart3;
  usb_stream.rx_buffer_size = COMMAND_BUFF_SZ;
  usb_stream.tx_buffer_size = COMMAND_BUFF_SZ;

  InitUartStreamParameters(&usb_stream);
  StartUartReceive(&usb_stream);

  UINormalMode();

  uint32_t timeout = HAL_GetTick() + 10000;
  while (HAL_GetTick() < timeout &&
    HAL_GPIO_ReadPin(UI_STAT_GPIO_Port, UI_STAT_Pin) != GPIO_PIN_SET)
  {
    // Stay here until the UI is finished booting
    HAL_Delay(10);
  }

  NetNormalMode();

  // Refresh the timeout
  timeout = HAL_GetTick() + 10000;
  while (HAL_GetTick() < timeout &&
    HAL_GPIO_ReadPin(NET_STAT_GPIO_Port, NET_STAT_Pin) != GPIO_PIN_SET)
  {
    // Stay here until the Net is done booting
    HAL_Delay(10);
  }

  state = Running;
  while (state == Running)
  {
    HandleCommands(&usb_stream);
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
  Usart3_Net_Upload_Runnning_Debug_Reset();

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

  UINormalMode();

  uint32_t timeout = HAL_GetTick() + 10000;
  while (HAL_GetTick() < timeout &&
    HAL_GPIO_ReadPin(UI_STAT_GPIO_Port, UI_STAT_Pin) != GPIO_PIN_SET)
  {
    // Stay here until the UI is finished booting
    HAL_Delay(10);
  }

  NetNormalMode();

  // Refresh the timeout
  timeout = HAL_GetTick() + 10000;
  while (HAL_GetTick() < timeout &&
    HAL_GPIO_ReadPin(NET_STAT_GPIO_Port, NET_STAT_Pin) != GPIO_PIN_SET)
  {
    // Stay here until the Net is done booting
    HAL_Delay(10);
  }

  state = Debug_Running;
  while (state == Debug_Running)
  {
    HandleTx(&ui_stream);
    HandleTx(&net_stream);
    HandleCommands(&usb_stream);
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
  MX_USART1_UART_Init();
  Usart3_Net_Upload_Runnning_Debug_Reset();
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

  usb_stream.from_uart = &huart3;
  usb_stream.to_uart = &huart1;
  InitUartStreamParameters(&usb_stream);

  net_stream.from_uart = &huart1;
  net_stream.to_uart = &huart3;
  InitUartStreamParameters(&net_stream);

  ui_stream.from_uart = &huart2;
  ui_stream.to_uart = &huart3;
  InitUartStreamParameters(&ui_stream);

  // NOTE these need to be set to 1 so when we choose a mode
  // It can properly de-init and re-init the huart modes
  // Its weird but hey, its what works with HAL
  usb_stream.is_listening = 1;
  net_stream.is_listening = 1;
  ui_stream.is_listening = 1;

  InitBtnStruct(&rst_btn, BTN_RST_Pin);
  InitBtnStruct(&ui_btn, BTN_UI_Pin);
  InitBtnStruct(&net_btn, BTN_NET_Pin);

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
static void Usart3_Net_Upload_Runnning_Debug_Reset(void)
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
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void Usart3_UI_Upload_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */
  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */
  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = BAUD;
  huart3.Init.WordLength = UART_WORDLENGTH_9B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_EVEN;
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
  HAL_GPIO_WritePin(GPIOA, LEDB_R_Pin | LEDA_R_Pin | LEDA_G_Pin | UI_BOOT1_Pin
    | UI_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LEDA_B_Pin | LEDB_G_Pin | LEDB_B_Pin | UI_BOOT0_Pin
    | NET_RST_Pin | NET_BOOT_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : BTN_RST_Pin BTN_UI_Pin BTN_NET_Pin */
  GPIO_InitStruct.Pin = BTN_RST_Pin | BTN_UI_Pin | BTN_NET_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : ADC_UI_STAT_Pin ADC_NET_STAT_Pin UI_STAT_Pin NET_STAT_Pin */
  GPIO_InitStruct.Pin = ADC_UI_STAT_Pin | ADC_NET_STAT_Pin | UI_STAT_Pin | NET_STAT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LEDB_R_Pin LEDA_R_Pin LEDA_G_Pin UI_BOOT1_Pin
                           UI_RST_Pin */
  GPIO_InitStruct.Pin = LEDB_R_Pin | LEDA_R_Pin | LEDA_G_Pin | UI_BOOT1_Pin
    | UI_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LEDA_B_Pin LEDB_G_Pin LEDB_B_Pin UI_BOOT0_Pin
                           NET_RST_Pin NET_BOOT_Pin */
  GPIO_InitStruct.Pin = LEDA_B_Pin | LEDB_G_Pin | LEDB_B_Pin | UI_BOOT0_Pin
    | NET_RST_Pin | NET_BOOT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : RTS_Pin */
  GPIO_InitStruct.Pin = RTS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(RTS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : CTS_Pin */
  GPIO_InitStruct.Pin = CTS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(CTS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : CTS_Pin */
  GPIO_InitStruct.Pin = MGMT_DBG7_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(MGMT_DBG7_GPIO_Port, &GPIO_InitStruct);


  // Clock
  /*Configure GPIO pin : PA8 */
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF0_MCO;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);
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
