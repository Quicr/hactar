#include "main.h"
#include "app_main.hh"

#include <string>
#include "font.hh"
#include "port_pin.hh"
#include "push_release_button.hh"

#include "screen.hh"
#include "q10_keyboard.hh"
#include "eeprom.hh"
#include "user_interface_manager.hh"

#include "serial_stm.hh"
#include "led.hh"
#include "audio_chip.hh"

#include <hpke/random.h>
#include <hpke/digest.h>
#include <hpke/signature.h>
#include <hpke/hpke.h>

#include <crypto/cmox_crypto.h>
#include <cstring>

#include <mls/state.h>

#include <memory>
#include <cmath>
#include <sstream>

#include "logger.hh"
#include <stdio.h>
#include <stdlib.h>

// Handlers
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

extern SPI_HandleTypeDef hspi1;
extern I2C_HandleTypeDef hi2c1;
extern I2S_HandleTypeDef hi2s3;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern RNG_HandleTypeDef hrng;


port_pin cs = { DISP_CS_GPIO_Port, DISP_CS_Pin };
port_pin dc = { DISP_DC_GPIO_Port, DISP_DC_Pin };
port_pin rst = { DISP_RST_GPIO_Port, DISP_RST_Pin };
port_pin bl = { DISP_BL_GPIO_Port, DISP_BL_Pin };

Screen screen(hspi1, cs, dc, rst, bl, Screen::Orientation::portrait);
Q10Keyboard* keyboard = nullptr;
SerialStm* mgmt_serial_interface = nullptr;
SerialStm* net_serial_interface = nullptr;
UserInterfaceManager* ui_manager = nullptr;
EEPROM* eeprom = nullptr;
AudioChip* audio = nullptr;
bool rx_busy = false;

uint8_t random_byte()
{
    // XXX(RLB) This is 4x slower than it could be, because we only take the
    // low-order byte of the four bytes in a uint32_t.
    /// BER, I don't know if that is necessarily true, since we would be
    // pulling a 32 bit number from hardware anyways.
    auto value = uint32_t(0);
    HAL_RNG_GenerateRandomNumber(&hrng, &value);
    return value;
}

extern char _end;  // End of BSS section
extern char _estack;  // Start of stack

static char* heap_end = &_end;

// Function to get the remaining heap size
size_t getFreeHeapSize(void)
{
    char* current_heap_end = heap_end;
    return &_estack - current_heap_end;
}

// TODO Get the osc working correctly from an external signal
int app_main()
{
    audio = new AudioChip(hi2s3, hi2c1);

    // Reserve the first 32 bytes, and the total size is 255 bytes - 1k bits
    eeprom = new EEPROM(hi2c1, 32, 255);

    screen.Begin();
    HAL_TIM_Base_Start_IT(&htim3);

    // // Set the port pins and groups for the keyboard columns
    port_pin col_pins[Q10_COLS] =
    {
        { KB_COL1_GPIO_Port, KB_COL1_Pin },
        { KB_COL2_GPIO_Port, KB_COL2_Pin },
        { KB_COL3_GPIO_Port, KB_COL3_Pin },
        { KB_COL4_GPIO_Port, KB_COL4_Pin },
        { KB_COL5_GPIO_Port, KB_COL5_Pin },
    };

    // Set the port pins and groups for the keyboard rows
    port_pin row_pins[Q10_ROWS] =
    {
        { KB_ROW1_GPIO_Port, KB_ROW1_Pin },
        { KB_ROW2_GPIO_Port, KB_ROW2_Pin },
        { KB_ROW3_GPIO_Port, KB_ROW3_Pin },
        { KB_ROW4_GPIO_Port, KB_ROW4_Pin },
        { KB_ROW5_GPIO_Port, KB_ROW5_Pin },
        { KB_ROW6_GPIO_Port, KB_ROW6_Pin },
        { KB_ROW7_GPIO_Port, KB_ROW7_Pin },
    };

    // Create the keyboard object
    keyboard = new Q10Keyboard(col_pins, row_pins, 200, 100, &htim2);

    // Initialize the keyboard
    keyboard->Begin();

    mgmt_serial_interface = new SerialStm(&huart1);
    net_serial_interface = new SerialStm(&huart2, 2048);

    ui_manager = new UserInterfaceManager(screen, *keyboard,
        *net_serial_interface, *eeprom, *audio);

    HAL_GPIO_WritePin(UI_LED_R_GPIO_Port, UI_LED_R_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(UI_LED_G_GPIO_Port, UI_LED_G_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(UI_LED_B_GPIO_Port, UI_LED_B_Pin, GPIO_PIN_SET);

    // Initialize the cryptographic library
    const auto rv = cmox_initialize(nullptr);
    Logger::Log(Logger::Level::Info, "app init:", rv == CMOX_INIT_SUCCESS);

    // Delayed condition
    uint32_t blink = HAL_GetTick();

    WaitForNetReady();
    Logger::Log(Logger::Level::Info, "Hactar is ready");

    screen.EnableBackLight();

    screen.FillRectangleAsync(0, screen.ViewWidth(), 0, screen.ViewHeight(), C_WHITE);
    screen.FillRectangleAsync(0, screen.ViewWidth(), 0, screen.ViewHeight(), C_BLACK);
    screen.FillRectangleAsync(10, 20, 10, 20, C_BLUE);
    screen.FillRectangleAsync(0, 10, 0, 10, C_RED);
    screen.DrawLineAsync(50, 60, 80, 45, 5, C_BLUE);
    screen.DrawLineAsync(100, 90, 50, 30, 3, C_GREEN);
    screen.DrawLineAsync(50, 63, 100, 115, 3, C_RED);
    screen.DrawLineAsync(100, 88, 120, 140, 3, C_CYAN);

    screen.DrawPixelAsync(200, 200, C_MAGENTA);

    const uint16_t points[][2] = {{100, 100}, {120, 120}, {80, 120}};
    screen.DrawPolygonAsync(3, points, 1, C_YELLOW);

    screen.DrawArrowAsync(20, 200, 20, 10, 1, Screen::ArrowDirection::Up, C_BLUE);
    screen.DrawArrowAsync(20, 200, 20, 10, 1, Screen::ArrowDirection::Up, C_BLUE);
    screen.DrawArrowAsync(20, 200, 20, 10, 1, Screen::ArrowDirection::Up, C_BLUE);
    screen.DrawArrowAsync(20, 200, 20, 10, 1, Screen::ArrowDirection::Up, C_BLUE);
    screen.DrawArrowAsync(20, 200, 20, 10, 1, Screen::ArrowDirection::Up, C_BLUE);
    screen.DrawArrowAsync(20, 200, 20, 10, 1, Screen::ArrowDirection::Up, C_BLUE);
    screen.DrawArrowAsync(20, 200, 20, 10, 1, Screen::ArrowDirection::Up, C_BLUE);
    screen.DrawArrowAsync(40, 200, 20, 10, 1, Screen::ArrowDirection::Up, C_BLUE);
    screen.DrawArrowAsync(60, 200, 20, 10, 1, Screen::ArrowDirection::Up, C_YELLOW);

    screen.DrawCharacterAsync(70, 200, 'H', font5x8, C_BLUE, C_BLACK);
    screen.DrawCharacterAsync(80, 200, 'H', font6x8, C_BLUE, C_BLACK);
    screen.DrawCharacterAsync(90, 200, 'H', font7x12, C_BLUE, C_BLACK);
    screen.DrawCharacterAsync(100, 200, 'H', font11x16, C_BLUE, C_BLACK);
    screen.DrawCharacterAsync(100, 200, 'H', font11x16, C_BLUE, C_BLACK);
    screen.DrawCharacterAsync(100, 200, 'H', font11x16, C_BLUE, C_BLACK);
    screen.DrawCharacterAsync(100, 200, 'H', font11x16, C_BLUE, C_BLACK);
    screen.DrawCharacterAsync(100, 200, 'H', font11x16, C_BLUE, C_BLACK);

    screen.DrawRectangleAsync(110, 120, 200, 210, 2, C_BLUE);
    screen.DrawRectangleAsync(125, 135, 200, 210, 2, C_RED);

    screen.DrawStringBoxAsync(1, 95, 140, 200, "Hello world, look at my string!", 31, font5x8, C_YELLOW, C_BLACK, true);
    screen.DrawStringBoxAsync(1, 36, 200, 220, "Hello Hello Hello", 17, font5x8, C_GREEN, C_BLACK, true);

    screen.DrawTriangleAsync(100, 0, 110, 10, 95, 10, 1, C_YELLOW);
    while (1)
    {
        // ui_manager->Update();

        if (HAL_GetTick() > blink)
        {
            HAL_GPIO_TogglePin(UI_LED_G_GPIO_Port, UI_LED_G_Pin);
            blink = HAL_GetTick() + 1000;
        }
    }

    return 0;
}

void WaitForNetReady()
{
    while (HAL_GPIO_ReadPin(UI_STAT_GPIO_Port, UI_STAT_Pin) == GPIO_PIN_RESET)
    {
        HAL_Delay(10);
    }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart, uint16_t size)
{
    if (huart->Instance == USART2)
    {
        net_serial_interface->RxEvent(size);
    }
    else if (huart->Instance == USART1)
    {
        mgmt_serial_interface->RxEvent(size);
    }
}
void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart)
{
    HAL_GPIO_TogglePin(UI_LED_B_GPIO_Port, UI_LED_B_Pin);
    if (huart->Instance == USART2)
    {
        net_serial_interface->TxEvent();
    }
    else if (huart->Instance == USART1)
    {
        // HAL_GPIO_TogglePin(UI_LED_B_GPIO_Port, UI_LED_B_Pin);
        mgmt_serial_interface->TxEvent();
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart)
{
    uint16_t err;
    UNUSED(err);
    if (huart->Instance == USART2)
    {
        net_serial_interface->Reset();
        HAL_GPIO_TogglePin(UI_LED_R_GPIO_Port, UI_LED_R_Pin);

        // Read the err codes to clear them
        err = huart->Instance->SR;

        net_serial_interface->StartRx();
    }
    else if (huart->Instance == USART1)
    {
        mgmt_serial_interface->Reset();
        HAL_GPIO_TogglePin(UI_LED_G_GPIO_Port, UI_LED_G_Pin);

        // Read the err codes to clear them
        err = huart->Instance->SR;

        mgmt_serial_interface->StartRx();
    }
}

void HAL_I2SEx_TxRxHalfCpltCallback(I2S_HandleTypeDef* hi2s)
{
    UNUSED(hi2s);
    // HAL_GPIO_TogglePin(UI_LED_G_GPIO_Port, UI_LED_G_Pin);
    audio->HalfCompleteCallback();
}

void HAL_I2SEx_TxRxCpltCallback(I2S_HandleTypeDef* hi2s)
{
    UNUSED(hi2s);
    // HAL_GPIO_TogglePin(UI_LED_R_GPIO_Port, UI_LED_R_Pin);
    audio->CompleteCallback();
}


void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef* hspi)
{
    UNUSED(hspi);
    screen.ReleaseSPI();
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef* hspi)
{
    volatile int x = 10;
    x += 10;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim)
{
    // Keyboard timer callback!
    if (htim->Instance == TIM2)
    {
        keyboard->Read();
    }
    else if (htim->Instance == TIM3)
    {
        screen.Update(HAL_GetTick());
    }
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
        e.g.: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
        /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
