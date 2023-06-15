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
#define OTHER 1

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
static void InitButtons();
static void InitLEDs();
static void InitBitBangPins();
static void InitIRQ();
static void ResetUpload();
static void UploadUIBegin();
static void UploadNetBegin();
static void TransmitReceive();
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
enum State
{
  Reset,
  Running,
  UI,
  Net,
  NetDebug
};

volatile enum State state;
volatile uint8_t uploading = 0;

// Button interrupt debounce
uint32_t btn_debounce_timeout = 0;

uint32_t network_button_last_press = 0;

// Callback from interrupt
void HAL_GPIO_EXTI_Callback(uint16_t gpio_pin)
{
  if (HAL_GetTick() < btn_debounce_timeout)
    return;
  btn_debounce_timeout = HAL_GetTick() + 50;
  if (gpio_pin == GPIO_PIN_1)
  {
    // Interrupt line for when the uploading completes
    if (uploading)
    {
      ResetUpload();
    }
  }
  else if (gpio_pin == GPIO_PIN_13)
  {
    ResetUpload();
  }
  else if (gpio_pin == GPIO_PIN_14)
  {
    UploadUIBegin();
  }
  else if (gpio_pin == GPIO_PIN_15)
  {
    UploadNetBegin();
  }
}

static void InitButtons()
{
  __HAL_RCC_GPIOC_CLK_ENABLE();
  // RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
  // RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

  // Put pin 13, 14, and 15 into input mode [00] (reset)
  // GPIOC->MODER &= ~GPIO_MODER_MODER13;
  // GPIOC->PUPDR
  // GPIOC->MODER &= ~GPIO_MODER_MODER14;
  // GPIOC->MODER &= ~GPIO_MODER_MODER15;
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_15, GPIO_PIN_RESET);

  GPIO_InitTypeDef GPIO_InitStruct = { 0 };
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
  GPIO_InitStruct.Pin = GPIO_PIN_14;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
  GPIO_InitStruct.Pin = GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  SYSCFG->EXTICR[3] |= SYSCFG_EXTICR4_EXTI13_PC;
  EXTI->IMR |= EXTI_IMR_IM13;
  EXTI->RTSR &= ~EXTI_RTSR_TR13;
  EXTI->FTSR |= EXTI_FTSR_FT13;

  SYSCFG->EXTICR[3] |= SYSCFG_EXTICR4_EXTI14_PC;
  EXTI->IMR |= EXTI_IMR_IM14;
  EXTI->RTSR &= ~EXTI_RTSR_TR14;
  EXTI->FTSR |= EXTI_FTSR_FT14;

  SYSCFG->EXTICR[3] |= SYSCFG_EXTICR4_EXTI15_PC;
  EXTI->IMR |= EXTI_IMR_IM15;
  EXTI->RTSR &= ~EXTI_RTSR_TR15;
  EXTI->FTSR |= EXTI_FTSR_FT15;
}

static void InitLEDs()
{
  // TODO move to LL version
  GPIO_InitTypeDef GPIO_InitStruct = { 0 };
  /* USER CODE BEGIN MX_GPIO_Init_1 */
  /* USER CODE END MX_GPIO_Init_1 */

    /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin : PA6 */
  GPIO_InitStruct.Pin = LEDA_R_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LEDA_R_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PA7 */
  GPIO_InitStruct.Pin = LEDA_G_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LEDA_G_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PB0 */
  GPIO_InitStruct.Pin = LEDA_B_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LEDA_B_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PA4 */
  GPIO_InitStruct.Pin = LEDB_R_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LEDB_R_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PB14 */
  GPIO_InitStruct.Pin = LEDB_G_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LEDB_G_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PB15 */
  GPIO_InitStruct.Pin = LEDB_B_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LEDB_B_GPIO_Port, &GPIO_InitStruct);
}

static void InitBitBangPins()
{
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  GPIO_InitTypeDef GPIO_InitStruct = { 0 };

  // USB uart
  // Configure GPIO pins : PB10|PB11 tx/rx
  GPIO_InitStruct.Pin = USB_RX1_MGMT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(USB_RX1_MGMT_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = USB_TX1_MGMT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(USB_TX1_MGMT_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = CTS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(CTS_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = RTS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(RTS_GPIO_Port, &GPIO_InitStruct);

  // UI UART
  // Configure GPIO pins : PA2|PA3 tx/rx
  GPIO_InitStruct.Pin = UI_RX1_MGMT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(UI_RX1_MGMT_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = UI_TX1_MGMT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(UI_TX1_MGMT_GPIO_Port, &GPIO_InitStruct);

  // UI reset and boot
  GPIO_InitStruct.Pin = UI_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(UI_RST_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = UI_BOOT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(UI_BOOT_GPIO_Port, &GPIO_InitStruct);

  // Net UART
  // Configure GPIO pins : PB6|PB7 tx/rx
  GPIO_InitStruct.Pin = NET_RX0_MGMT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(NET_RX0_MGMT_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = NET_TX0_MGMT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(NET_TX0_MGMT_GPIO_Port, &GPIO_InitStruct);

  // Net reset and boot
  GPIO_InitStruct.Pin = NET_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(NET_RST_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = NET_BOOT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(NET_BOOT_GPIO_Port, &GPIO_InitStruct);

  // The RTS line goes low when uploading finishes
  SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI1_PB;
  EXTI->IMR |= EXTI_IMR_IM1;
  EXTI->RTSR &= ~EXTI_RTSR_TR1;   // Turn off rising edge detection
  EXTI->FTSR |= EXTI_FTSR_FT1;    // Turn on falling edge detection

}

static void InitIRQ()
{
  HAL_NVIC_EnableIRQ(BTN_RST_EXTI_IRQn);
  HAL_NVIC_EnableIRQ(BTN_UI_EXTI_IRQn);
  HAL_NVIC_EnableIRQ(BTN_NET_EXTI_IRQn);
  HAL_NVIC_EnableIRQ(RTS_EXTI_IRQn);

  HAL_NVIC_SetPriority(BTN_RST_EXTI_IRQn, 0, 0);
  HAL_NVIC_SetPriority(BTN_UI_EXTI_IRQn, 0, 0);
  HAL_NVIC_SetPriority(BTN_NET_EXTI_IRQn, 0, 0);
  HAL_NVIC_SetPriority(RTS_EXTI_IRQn, 0, 0);
}

static void ResetUpload()
{
  state = Reset;
  uploading = 0;
}

static void UploadUIBegin()
{
  state = UI;
}

static void UploadNetBegin()
{
  if (state == Net)
    state = NetDebug;
  else
    state = Net;
}

static void TransmitReceive()
{
  // TODO use as a template for the UI
  // volatile uint32_t usb_input_pins;
  // const uint32_t USB_odr_on = GPIO_ODR_11;
  // const uint32_t USB_odr_off = ~GPIO_ODR_11;

  // uploading = 1;
  // while (uploading)
  // {

  //   // Copy from slave
  //   if (slave_port->IDR & slave_idr)
  //   {
  //     GPIOB->ODR |= GPIO_ODR_11;
  //   }
  //   else
  //   {
  //     GPIOB->ODR &= ~GPIO_ODR_11;
  //   }
  //   // usb_input_pins = GPIOB->IDR;


  //   // Check the usb uart bit
  //   if (GPIOB->IDR & GPIO_IDR_10)
  //   {
  //     // HAL_GPIO_TogglePin(LEDA_B_GPIO_Port, LEDA_B_Pin);
  //     slave_port->ODR |= slave_odr_on;
  //     // GPIOB->ODR |= USB_odr_on;
  //     // HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_SET);
  //   }
  //   else
  //   {
  //     // HAL_GPIO_TogglePin(LEDA_B_GPIO_Port, LEDA_B_Pin);
  //     slave_port->ODR &= slave_odr_off;
  //     // GPIOB->ODR &= USB_odr_off;
  //     // HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_RESET);
  //   }
  // }
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
  MX_TIM3_Init();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* USER CODE BEGIN 2 */

  InitButtons();
  InitIRQ();
  InitLEDs();
  InitBitBangPins();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  // Turn off leds
  HAL_GPIO_WritePin(LEDA_R_GPIO_Port, LEDA_R_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LEDA_G_GPIO_Port, LEDA_G_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LEDA_B_GPIO_Port, LEDA_B_Pin, GPIO_PIN_SET);

  HAL_GPIO_WritePin(LEDB_R_GPIO_Port, LEDB_R_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LEDB_G_GPIO_Port, LEDB_G_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LEDB_B_GPIO_Port, LEDB_B_Pin, GPIO_PIN_SET);

  // while(1)
  // {
  //   // // Echo usb back
  //   if (USB_PORT->IDR & USB_RX_Bit)
  //   {
  //     USB_PORT->ODR |= USB_TX_Bit_On;
  //   }
  //   else
  //   {
  //     USB_PORT->ODR &= USB_TX_Bit_Off;
  //   }
  // }

  while (1)
  {
    if (state == Reset)
    {
      // Bring the stm boot into normal mode (0) and send the reset
      HAL_GPIO_WritePin(UI_BOOT_GPIO_Port, UI_BOOT_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(UI_RST_GPIO_Port, UI_RST_Pin, GPIO_PIN_RESET);

      // Bring the esp boot into normal mode (1)
      HAL_GPIO_WritePin(NET_BOOT_GPIO_Port, NET_BOOT_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(NET_RST_GPIO_Port, NET_RST_Pin, GPIO_PIN_RESET);

      // Wait a moment
      HAL_Delay(500);

      HAL_GPIO_WritePin(UI_RST_GPIO_Port, UI_RST_Pin, GPIO_PIN_SET);

      // HAL_Delay(100);
      HAL_GPIO_WritePin(NET_RST_GPIO_Port, NET_RST_Pin, GPIO_PIN_SET);

      // Release the LEDs
      // Loop in here forever while running
      state = Running;
      while (state == Running)
      {
        HAL_GPIO_WritePin(LEDA_R_GPIO_Port, LEDA_R_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LEDA_G_GPIO_Port, LEDA_G_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LEDA_B_GPIO_Port, LEDA_B_Pin, GPIO_PIN_SET);

        HAL_GPIO_WritePin(LEDB_R_GPIO_Port, LEDB_R_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LEDB_G_GPIO_Port, LEDB_G_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LEDB_B_GPIO_Port, LEDB_B_Pin, GPIO_PIN_SET);
      }
    }

    if (state == UI)
    {
      // Bring the stm boot into bootloader mode (1) and send the reset
      HAL_GPIO_WritePin(UI_BOOT_GPIO_Port, UI_BOOT_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(UI_RST_GPIO_Port, UI_RST_Pin, GPIO_PIN_RESET);

      // Bring the esp boot into normal mode (1)
      HAL_GPIO_WritePin(NET_BOOT_GPIO_Port, NET_BOOT_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(NET_RST_GPIO_Port, NET_RST_Pin, GPIO_PIN_RESET);

      HAL_Delay(10);

      HAL_GPIO_WritePin(UI_RST_GPIO_Port, UI_RST_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(UI_BOOT_GPIO_Port, UI_BOOT_Pin, GPIO_PIN_SET);

      // HAL_Delay(100);
      // HAL_GPIO_WritePin(NET_RST_GPIO_Port, NET_RST_Pin, GPIO_PIN_SET);

      // Set LEDS for ui
      HAL_GPIO_WritePin(LEDA_R_GPIO_Port, LEDA_R_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(LEDA_G_GPIO_Port, LEDA_G_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(LEDA_B_GPIO_Port, LEDA_B_Pin, GPIO_PIN_SET);

      HAL_GPIO_WritePin(LEDB_R_GPIO_Port, LEDB_R_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(LEDB_G_GPIO_Port, LEDB_G_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(LEDB_B_GPIO_Port, LEDB_B_Pin, GPIO_PIN_SET);

      uploading = 0;
      while (state == UI)
      {
        // Copy from UI chip
        if (UI_PORT->IDR & UI_RX_Bit)
        {
          USB_PORT->ODR |= USB_TX_Bit_On;
        }
        else
        {
          USB_PORT->ODR &= USB_TX_Bit_Off;
        }

        // Check the usb uart bit for input
        if (USB_PORT->IDR & USB_RX_Bit)
        {
          UI_PORT->ODR |= UI_TX_Bit_On;
        }
        else
        {
          UI_PORT->ODR &= UI_TX_Bit_Off;
        }
      }
    }

    if (state == Net)
    {
      // Get some bits to ignore
      uint32_t bits_copied = 0;

      // Set leds
      HAL_GPIO_WritePin(LEDA_R_GPIO_Port, LEDA_R_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(LEDA_G_GPIO_Port, LEDA_G_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(LEDA_B_GPIO_Port, LEDA_B_Pin, GPIO_PIN_SET);

      HAL_GPIO_WritePin(LEDB_R_GPIO_Port, LEDB_R_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(LEDB_G_GPIO_Port, LEDB_G_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(LEDB_B_GPIO_Port, LEDB_B_Pin, GPIO_PIN_SET);

      // Wait for the start bit from esptool
      while ((USB_PORT->IDR & USB_RX_Bit) && (state == Net))
      {
        USB_PORT->ODR |= NET_TX_Bit_On;
      }
      USB_PORT->ODR &= NET_TX_Bit_Off;

      if (state != Net)
        continue;

      // Put esp and stm into reset
      HAL_GPIO_WritePin(NET_BOOT_GPIO_Port, NET_BOOT_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(UI_RST_GPIO_Port, UI_RST_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(NET_RST_GPIO_Port, NET_RST_Pin, GPIO_PIN_RESET);

      // Bring the boot for ui low (normal)
      HAL_GPIO_WritePin(UI_BOOT_GPIO_Port, UI_BOOT_Pin, GPIO_PIN_RESET);

      // Bring the boot low for esp, bootloader mode (0)
      HAL_GPIO_WritePin(NET_BOOT_GPIO_Port, NET_BOOT_Pin, GPIO_PIN_RESET);

      // Pretty much ignore a bunch of bits
      while (bits_copied++ < 0x100000 && state == Net)
      {
        // Copy from network
        if (NET_PORT->IDR & NET_RX_Bit)
        {
          USB_PORT->ODR |= USB_TX_Bit_On;
        }
        else
        {
          USB_PORT->ODR &= USB_TX_Bit_Off;
        }

        // Check the usb uart bit
        if (USB_PORT->IDR & USB_RX_Bit)
        {
          NET_PORT->ODR |= NET_TX_Bit_On;
        }
        else
        {
          NET_PORT->ODR &= NET_TX_Bit_Off;
        }
      }

      if (state != Net)
        continue;

      uploading = 1;

      // Release the esp reset to put into boot mode
      HAL_GPIO_WritePin(NET_RST_GPIO_Port, NET_RST_Pin, GPIO_PIN_SET);

      while (uploading)
      {
        // Copy from net chip
        if (NET_PORT->IDR & NET_RX_Bit)
        {
          USB_PORT->ODR |= USB_TX_Bit_On;
        }
        else
        {
          USB_PORT->ODR &= USB_TX_Bit_Off;
        }

        // Check the usb uart bit for input
        if (USB_PORT->IDR & USB_RX_Bit)
        {
          NET_PORT->ODR |= NET_TX_Bit_On;
        }
        else
        {
          NET_PORT->ODR &= NET_TX_Bit_Off;
        }
      }
      USB_PORT->ODR &= USB_TX_Bit_Off;
      NET_PORT->ODR &= NET_TX_Bit_Off;
    } // end if state == net

    if (state == NetDebug)
    {
      // Set the debug led (blue)
      HAL_GPIO_WritePin(LEDA_R_GPIO_Port, LEDA_R_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(LEDA_G_GPIO_Port, LEDA_G_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(LEDA_B_GPIO_Port, LEDA_B_Pin, GPIO_PIN_SET);

      HAL_GPIO_WritePin(LEDB_R_GPIO_Port, LEDB_R_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(LEDB_G_GPIO_Port, LEDB_G_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(LEDB_B_GPIO_Port, LEDB_B_Pin, GPIO_PIN_RESET);


      // Bring the stm boot into normal mode (0) and send the reset
      HAL_GPIO_WritePin(UI_BOOT_GPIO_Port, UI_BOOT_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(UI_RST_GPIO_Port, UI_RST_Pin, GPIO_PIN_RESET);

      // Bring the esp boot into normal mode (1)
      HAL_GPIO_WritePin(NET_BOOT_GPIO_Port, NET_BOOT_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(NET_RST_GPIO_Port, NET_RST_Pin, GPIO_PIN_RESET);

      // Wait a moment
      HAL_Delay(500);

      HAL_GPIO_WritePin(UI_RST_GPIO_Port, UI_RST_Pin, GPIO_PIN_SET);

      // HAL_Delay(100);
      HAL_GPIO_WritePin(NET_RST_GPIO_Port, NET_RST_Pin, GPIO_PIN_SET);

      while (state == NetDebug)
      {
        // Copy from net chip
        if (UI_PORT->IDR & UI_RX_Bit)
        {
          USB_PORT->ODR |= USB_TX_Bit_On;
        }
        else
        {
          USB_PORT->ODR &= USB_TX_Bit_Off;
        }

        // Check the usb uart bit for input
        if (USB_PORT->IDR & USB_RX_Bit)
        {
          UI_PORT->ODR |= UI_TX_Bit_On;
        }
        else
        {
          UI_PORT->ODR &= UI_TX_Bit_Off;
        }
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
  RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
  RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

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
  HAL_GPIO_WritePin(GPIOA, UI_RX1_MGMT_Pin | LEDB_R_Pin | LEDA_R_Pin | LEDA_G_Pin
    | MGMT_DBG7_Pin | UI_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LEDA_B_Pin | USB_RX1_MGMT_Pin | LEDB_G_Pin | LEDB_B_Pin
    | UI_BOOT_Pin | NET_RST_Pin | NET_BOOT_Pin | NET_RX0_MGMT_Pin, GPIO_PIN_RESET);

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

  /*Configure GPIO pin : UI_TX1_MGMT_Pin */
  GPIO_InitStruct.Pin = UI_TX1_MGMT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(UI_TX1_MGMT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : UI_RX1_MGMT_Pin */
  GPIO_InitStruct.Pin = UI_RX1_MGMT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(UI_RX1_MGMT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LEDB_R_Pin LEDA_R_Pin LEDA_G_Pin MGMT_DBG7_Pin
                           UI_RST_Pin */
  GPIO_InitStruct.Pin = LEDB_R_Pin | LEDA_R_Pin | LEDA_G_Pin | MGMT_DBG7_Pin
    | UI_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LEDA_B_Pin LEDB_G_Pin LEDB_B_Pin UI_BOOT_Pin
                           NET_RST_Pin NET_BOOT_Pin */
  GPIO_InitStruct.Pin = LEDA_B_Pin | LEDB_G_Pin | LEDB_B_Pin | UI_BOOT_Pin
    | NET_RST_Pin | NET_BOOT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : RTS_Pin USB_TX1_MGMT_Pin CTS_Pin NET_TX0_MGMT_Pin */
  GPIO_InitStruct.Pin = RTS_Pin | USB_TX1_MGMT_Pin | CTS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = NET_TX0_MGMT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

  /*Configure GPIO pins : USB_RX1_MGMT_Pin NET_RX0_MGMT_Pin */
  GPIO_InitStruct.Pin = USB_RX1_MGMT_Pin | NET_RX0_MGMT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PA8 */
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF0_MCO;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /**/
  HAL_I2CEx_EnableFastModePlus(SYSCFG_CFGR1_I2C_FMP_PB7);

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
void assert_failed(uint8_t* file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
     /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
