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

// Set the port pins and groups for the keyboard columns
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
static Q10Keyboard keyboard(col_pins, row_pins, 200, 100, &htim2);

SerialStm* net_serial_interface = nullptr;
AudioChip* audio = nullptr;

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

// TODO we have an issue regarding the free-ness of memories
// we get stuck waiting forever for a memory to run
// for some reason.
// Don't know why
int app_main()
{
    audio = new AudioChip(hi2s3, hi2c1);

    // Reserve the first 32 bytes, and the total size is 255 bytes - 1k bits
    EEPROM eeprom(hi2c1, 32, 255);

    screen.Begin();
    HAL_TIM_Base_Start_IT(&htim3);

    // Initialize the keyboard
    keyboard.Begin();

    net_serial_interface = new SerialStm(&huart2, 2048);

    UserInterfaceManager ui_manager(screen, keyboard,
        *net_serial_interface, eeprom, *audio);

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

    // TestScreenInit(C_BLACK);
    while (1)
    {
        ui_manager.Update();

        // TestScreen();

        // if (HAL_GetTick() > blink)
        // {
        //     HAL_GPIO_TogglePin(UI_LED_G_GPIO_Port, UI_LED_G_Pin);
        //     blink = HAL_GetTick() + 1000;
        // }
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
}
void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart)
{
    HAL_GPIO_TogglePin(UI_LED_B_GPIO_Port, UI_LED_B_Pin);
    if (huart->Instance == USART2)
    {
        net_serial_interface->TxEvent();
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
    screen.SpiComplete();
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
        keyboard.Read();
    }
    else if (htim->Instance == TIM3)
    {
        screen.Update(HAL_GetTick());
    }
}

void TestScreenInit(uint16_t colour)
{
    screen.EnableBackLight();
    screen.FillScreen(colour);
}

void TestScreen()
{
    static uint16_t colour = C_GREEN;
    static uint16_t c = 0;

    if (c == 0)
    {
        colour = C_GREEN;
        ++c;
    }
    else if (c == 1)
    {
        colour = C_BLUE;
        ++c;
    }
    else
    {
        colour = C_RED;
        c = 0;
    }

    screen.DrawRectangleAsync(0, 10, 0, 10, 1, colour);
    screen.FillRectangleAsync(10, 20, 0, 10, colour);
    screen.DrawLineAsync(30, 40, 0, 10, 5, colour);

    screen.DrawPixelAsync(40, 1, colour);
    screen.DrawPixelAsync(42, 1, colour);
    screen.DrawPixelAsync(40, 3, colour);
    screen.DrawPixelAsync(42, 3, colour);

    screen.DrawArrowAsync(50, 0, 20, 10, 1, Screen::ArrowDirection::Up, colour);
    screen.DrawArrowAsync(60, 21, 20, 10, 1, Screen::ArrowDirection::Down, colour);
    screen.DrawArrowAsync(75, 10, 20, 10, 1, Screen::ArrowDirection::Left, colour);
    screen.DrawArrowAsync(95, 30, 20, 10, 1, Screen::ArrowDirection::Right, colour);

    screen.FillArrowAsync(105, 0, 20, 10, Screen::ArrowDirection::Up, colour);
    screen.FillArrowAsync(115, 21, 20, 10, Screen::ArrowDirection::Down, colour);
    screen.FillArrowAsync(130, 10, 20, 10, Screen::ArrowDirection::Left, colour);
    screen.FillArrowAsync(150, 30, 20, 10, Screen::ArrowDirection::Right, colour);

    screen.DrawCharacterAsync(0, 30, 'H', font5x8, colour, C_BLACK);
    screen.DrawCharacterAsync(6, 30, 'H', font6x8, colour, C_BLACK);
    screen.DrawCharacterAsync(13, 30, 'H', font7x12, colour, C_BLACK);
    screen.DrawCharacterAsync(21, 30, 'H', font11x16, colour, C_BLACK);

    screen.DrawStringBoxAsync(1, 100, 40, 60, "Hello world, look at my string!", 31, font5x8, colour, C_BLACK, true);
    screen.DrawStringBoxAsync(101, 141, 40, 90, "Hello Hello Hello", 17, font5x8, colour, C_BLACK, true);

    screen.DrawTriangleAsync(0, 65, 10, 65, 5, 70, 1, colour);
    screen.FillTriangleAsync(15, 65, 25, 65, 20, 70, colour);

    const uint16_t points [][2] = { {0, 80}, {40, 85}, {25, 95}, {5, 90} };
    screen.DrawPolygonAsync(sizeof(points) / sizeof(points[0]), points, 1, colour);
    const uint16_t points_2 [][2] = { {45, 80}, {85, 85}, {70, 95}, {50, 90} };
    screen.FillPolygonAsync(sizeof(points_2) / sizeof(points_2[0]), points_2, colour);

    screen.DrawCircleAsync(15, 115, 15, colour);
    screen.FillCircleAsync(60, 115, 15, colour);
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
