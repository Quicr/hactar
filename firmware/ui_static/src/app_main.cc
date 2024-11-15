#include "main.h"
#include "app_main.hh"

#include "audio_chip.hh"
#include "audio_codec.hh"

#include "screen.hh"


// Handlers
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

extern SPI_HandleTypeDef hspi1;
extern I2C_HandleTypeDef hi2c1;
extern I2S_HandleTypeDef hi2s3;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern RNG_HandleTypeDef hrng;

AudioChip audio_chip(hi2s3, hi2c1);

Screen screen(
    hspi1,
    DISP_CS_GPIO_Port,
    DISP_CS_Pin,
    DISP_DC_GPIO_Port,
    DISP_DC_Pin,
    DISP_RST_GPIO_Port,
    DISP_RST_Pin,
    DISP_BL_GPIO_Port,
    DISP_BL_Pin,
    Screen::portrait
);

// Make tx and rx buffers for audio
uint8_t tx_companded[AudioChip::Audio_Buffer_Sz_2] = { 0 };
uint8_t rx_companded[AudioChip::Audio_Buffer_Sz_2] = { 0 };

enum TIMER_FLAGS
{
    Audio_Interrupt = 0,
    Rx_Audio_Companded,
    Draw_Complete

};
uint32_t flags = 0b0000'0111;
bool error = false;

inline void RaiseFlag(TIMER_FLAGS flag)
{
    flags |= 1 << flag;
}

inline void LowerFlag(TIMER_FLAGS flag)
{
    flags &= ~(1 << flag);
}

inline void LowPowerMode()
{
    HAL_GPIO_WritePin(UI_LED_B_GPIO_Port, UI_LED_B_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(UI_LED_G_GPIO_Port, UI_LED_G_Pin, GPIO_PIN_SET);
    HAL_SuspendTick();
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
}

inline void WakeUp()
{
    HAL_GPIO_WritePin(UI_LED_B_GPIO_Port, UI_LED_B_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(UI_LED_G_GPIO_Port, UI_LED_G_Pin, GPIO_PIN_RESET);

    HAL_ResumeTick();
}

inline void CheckFlags()
{
    if (flags != 0b0000'0111)
    {
        error = true;
    }
    else
    {
        flags = 0;
    }
}

int app_main()
{
    audio_chip.Init();
    screen.Init();
    screen.EnableBacklight();
    // screen.FillRectangle(0, 10, 0, 10, Screen::Colour::CYAN);
    // screen.FillRectangle(0, 10, 0, 10, Screen::Colour::CYAN);

    // screen.FillRectangle(0, WIDTH, 0, HEIGHT, Screen::Colour::YELLOW);
    // screen.FillRectangle(0, 10, 0, 10, Screen::Colour::CYAN);

    HAL_GPIO_WritePin(UI_LED_R_GPIO_Port, UI_LED_R_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(UI_LED_G_GPIO_Port, UI_LED_G_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(UI_LED_B_GPIO_Port, UI_LED_B_Pin, GPIO_PIN_SET);


    audio_chip.StartI2S();
    uint32_t timeout;
    uint32_t current_tick;
    uint32_t redraw = uwTick;
    Screen::Colour next = Screen::Colour::GREEN;
    Screen::Colour curr = Screen::Colour::BLUE;

    const char* hello = "Hello1 Hello2 Hello3 Hello4 Hello5 Hello6 Hello7";
    screen.DrawString(2, 28, &hello, 48, font11x16, Screen::Colour::WHITE, Screen::Colour::BLACK);

    while (1)
    {
        HAL_GPIO_WritePin(UI_LED_R_GPIO_Port, UI_LED_R_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(UI_LED_G_GPIO_Port, UI_LED_G_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(UI_LED_B_GPIO_Port, UI_LED_B_Pin, GPIO_PIN_SET);
        if (error)
        {
            // Error_Handler();
        }

        while (flags == 0)
        {
            // TODO renable
            // LowPowerMode();
        }
        // If we broke out then that means we got an audio callback
        // TODO test clock stability
        current_tick = uwTick;
        timeout = current_tick + 10'000;

        // Send off a tx packet

        HAL_GPIO_WritePin(UI_LED_R_GPIO_Port, UI_LED_R_Pin, GPIO_PIN_RESET);
        AudioCodec::ALawCompand(audio_chip.RxBuffer(), rx_companded, AudioChip::Audio_Buffer_Sz_2);
        RaiseFlag(Rx_Audio_Companded);

        // Use remaining time to draw?
        if (uwTick > redraw)
        {
            // screen.FillRectangle(0, WIDTH, 0, HEIGHT, curr);
            screen.DrawRectangle(0, 10, 0, 10, 2, Screen::Colour::MAGENTA);
            screen.DrawCharacter(11, 0, 'h', font6x8, next, curr);
            // screen.DrawString(2, 28, &hello, 48, font11x16, next, curr);
            const uint16_t width = 50;
            const uint16_t height = 50;
            const uint16_t x_inc = width + 2;
            const uint16_t y_inc = height+1;
            // uint16_t x = 0;
            // uint16_t y = y_inc;
            // for (int i = 0 ; i < 19; ++i)
            // {
            //     if (20 + x >= WIDTH)
            //     {
            //         x = 0;
            //         y += y_inc;
            //     }
            //     // screen.DrawRectangle(x, x + width, y, y + height, 4, next);
            //     // screen.FillRectangle(x, x + width, y, y + height, next);
            //     screen.FillRectangle(0, WIDTH, 0, HEIGHT, next);
            //     x += x_inc;
            // }

            // swap
            Screen::Colour tmp = curr;
            curr = next;
            next = tmp;

            redraw = uwTick + 5000;
        }

        screen.Draw(timeout);
        RaiseFlag(Draw_Complete);
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
        // net_serial_interface->RxEvent(size);
    }
}
void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart)
{
    if (huart->Instance == USART2)
    {
        // net_serial_interface->TxEvent();
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart)
{
    uint16_t err;
    UNUSED(err);
    if (huart->Instance == USART2)
    {
        // net_serial_interface->Reset();
        // HAL_GPIO_TogglePin(UI_LED_R_GPIO_Port, UI_LED_R_Pin);

        // // Read the err codes to clear them
        // err = huart->Instance->SR;

        // net_serial_interface->StartRx();
    }
}

void HAL_I2SEx_TxRxHalfCpltCallback(I2S_HandleTypeDef* hi2s)
{
    UNUSED(hi2s);
    CheckFlags();
    // WakeUp();
    audio_chip.HalfCompleteCallback();
    RaiseFlag(Audio_Interrupt);
}

void HAL_I2SEx_TxRxCpltCallback(I2S_HandleTypeDef* hi2s)
{
    UNUSED(hi2s);
    CheckFlags();
    // HAL_GPIO_WritePin(UI_LED_G_GPIO_Port, UI_LED_G_Pin, GPIO_PIN_RESET);
    // WakeUp();
    audio_chip.CompleteCallback();
    RaiseFlag(Audio_Interrupt);
}


void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef* hspi)
{
    UNUSED(hspi);
    // screen.SpiComplete();
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
        // keyboard.Read();
    }
    else if (htim->Instance == TIM3)
    {
        // screen.Draw();
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
