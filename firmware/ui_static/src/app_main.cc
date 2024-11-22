#include "main.h"
#include "app_main.hh"

#include "audio_chip.hh"
#include "audio_codec.hh"

#include "screen.hh"

#include "serial.hh"

#include "fib.hh"


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

Serial serial(&huart2);

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
    Screen::flipped_portrait
);

constexpr uint32_t QMessage_Room_Length = 32;
constexpr uint32_t Serial_Header_Sz = 3 + QMessage_Room_Length;
constexpr uint32_t Serial_Audio_Buff_Sz = AudioChip::Audio_Buffer_Sz_2 + Serial_Header_Sz;

// Make tx and rx buffers for audio
uint8_t serial_tx_audio_buff[Serial_Audio_Buff_Sz] = {
    Serial_Audio_Buff_Sz, 0, Serial::Packet_Type::Audio };
// The bytes that take up the room should be set at some point
uint8_t serial_rx_audio_buff[Serial_Audio_Buff_Sz] = {
    Serial_Audio_Buff_Sz, 0, Serial::Packet_Type::Audio };

// Make a pointer that is to JUST the audio data section of the buffer
uint8_t* serial_tx_audio_offset_ptr = serial_tx_audio_buff + Serial_Header_Sz;
uint8_t* serial_rx_audio_offset_ptr = serial_rx_audio_buff + Serial_Header_Sz;

volatile bool sleeping = true;
volatile bool error = false;
constexpr uint32_t Expected_Flags = 0b0000'1111;
uint32_t flags = Expected_Flags;
uint32_t est_time_ms = 0;

inline void RaiseFlag(Timer_Flags flag)
{
    if (error)
    {
        return;
    }
    flags |= 1 << flag;
}

inline void LowerFlag(Timer_Flags flag)
{
    flags &= ~(1 << flag);
}

inline void LowPowerMode()
{
    HAL_GPIO_WritePin(UI_LED_B_GPIO_Port, UI_LED_B_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(UI_LED_G_GPIO_Port, UI_LED_G_Pin, GPIO_PIN_SET);
    HAL_SuspendTick();
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFE);
}

inline void WakeUp()
{
    HAL_GPIO_WritePin(UI_LED_B_GPIO_Port, UI_LED_B_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(UI_LED_G_GPIO_Port, UI_LED_G_Pin, GPIO_PIN_RESET);

    HAL_ResumeTick();
    sleeping = false;
}

inline void CheckFlags()
{
    if (flags != Expected_Flags)
    {
        error = true;
    }
    else
    {
        flags = 0;
    }
}

inline void ProcessText(uint16_t len)
{
    // Get the room
    uint8_t room[QMessage_Room_Length];
    serial.ReadSerial(room, QMessage_Room_Length, QMessage_Room_Length);
    len -= Serial_Header_Sz;

    uint8_t* serial_data = nullptr;
    size_t num_bytes = len;

    while (len > 0)
    {
        // Get the text
        serial.ReadSerial(&serial_data, num_bytes);

        // Now we have the text data
        screen.AppendText((const char*)serial_data, num_bytes);

        len -= num_bytes;
    }

    // Commit the text
    screen.CommitText();
}

int app_main()
{
    audio_chip.Init();
    screen.Init();
    screen.EnableBacklight();

    HAL_GPIO_WritePin(UI_LED_R_GPIO_Port, UI_LED_R_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(UI_LED_G_GPIO_Port, UI_LED_G_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(UI_LED_B_GPIO_Port, UI_LED_B_Pin, GPIO_PIN_SET);


    uint32_t timeout;
    uint32_t current_tick;
    uint32_t redraw = uwTick;
    Screen::Colour next = Screen::Colour::GREEN;
    Screen::Colour curr = Screen::Colour::BLUE;

    const char* hello = "Hello1 Hello2 Hello3 Hello4 Hello5 Hello6 Hello7";

    uint16_t rec_size = 20;
    Screen::Colour clr = Screen::Colour::RED;
    // for (int i = 0; i < 320; i += rec_size)
    // {
    //     screen.FillRectangle(0, WIDTH, i, (rec_size) + i, clr);

    //     clr = (Screen::Colour)((int)clr + 1);

    //     if (clr > Screen::Colour::YELLOW)
    //     {
    //         clr = Screen::Colour::RED;
    //     }
    // }

    screen.FillRectangle(0, WIDTH, 0, 20, Screen::Colour::GREEN);
    screen.FillRectangle(0, WIDTH, 21, 22, Screen::Colour::WHITE);
    screen.FillRectangle(0, WIDTH, 22, 62, clr);
    screen.FillRectangle(0, WIDTH, 62, 63, Screen::Colour::WHITE);
    screen.FillRectangle(0, WIDTH, 302, 320, Screen::Colour::BLUE);

    for (int i = 0; i < 320; i += NUM_ROWS)
    {
        screen.Draw(0);
    }

    HAL_Delay(1000);

    while (true)
    {
        for (int i = 0 ; i < 320; i++)
        {
            HAL_Delay(100);
            screen.TestScroll(i);
        }

    }
    bool scroll_timeout = est_time_ms + 5000;
    bool scroll = true;

    serial.StartReceive();
    audio_chip.StartI2S();
    while (1)
    {
        while (sleeping)
        {
            // LowPowerMode();
        }

        HAL_GPIO_WritePin(UI_LED_R_GPIO_Port, UI_LED_R_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(UI_LED_G_GPIO_Port, UI_LED_G_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(UI_LED_B_GPIO_Port, UI_LED_B_Pin, GPIO_PIN_SET);

        if (error)
        {
            // Error_Handler();
        }



        // If we broke out then that means we got an audio callback
        // TODO test clock stability

        AudioCodec::ALawCompand(audio_chip.RxBuffer(), serial_tx_audio_offset_ptr, AudioChip::Audio_Buffer_Sz_2);
        RaiseFlag(Rx_Audio_Companded);
        HAL_GPIO_WritePin(UI_LED_R_GPIO_Port, UI_LED_R_Pin, GPIO_PIN_RESET);

        // Try to send packets
        // serial.WriteSerial(serial_tx_audio_buff, Serial_Audio_Buff_Sz);

        // If there are bytes available read them
        while (serial.Unread())
        {
            // TODO decide how to do this.
            // Need to determine what kind of packet
            // audio, text, mls packets etc.

            // Get the length and the type
            uint16_t len = 0;
            uint8_t type = 3;

            // Deep magickas of the ancients
            serial.ReadSerial((uint8_t*)len, 2, 2);
            serial.ReadSerial(&type, 1, 1);

            switch (type)
            {
                case Serial::Audio:
                {
                    break;
                }
                case Serial::Text:
                {
                    ProcessText(len);
                    break;
                }
                case Serial::MLS:
                {
                    break;
                }
            }



            // If it is an audio packet we should uncompand it
            // First 2 bytes will be the length

            // If it is a text packet we need to display it


            // If it is mls we need to process it etc.
        }


        // Use remaining time to draw
        if (est_time_ms > redraw)
        {
            // screen.FillRectangle(0, WIDTH, 0, HEIGHT, curr);
            // screen.DrawRectangle(0, 10, 0, 10, 2, next);
            // screen.DrawCharacter(11, 0, 'h', font6x8, next, curr);
            // screen.DrawString(0, 28, &hello, 48, font5x8, next, curr);
            // const uint16_t width = 50;
            // const uint16_t height = 50;
            // const uint16_t x_inc = width + 2;
            // const uint16_t y_inc = height + 1;

            // swap
            Screen::Colour tmp = curr;
            curr = next;
            next = tmp;
            redraw = est_time_ms + 1600;
        }

        // Draw what we can
        screen.Draw(0);
        RaiseFlag(Draw_Complete);
        HAL_GPIO_WritePin(UI_LED_G_GPIO_Port, UI_LED_G_Pin, GPIO_PIN_RESET);

        sleeping = true;
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
        serial.RxEvent(size);
    }
}
void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart)
{
    if (huart->Instance == USART2)
    {
        serial.Free();
        RaiseFlag(Rx_Audio_Transmitted);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart)
{
    if (huart->Instance == USART2)
    {
        serial.Reset();
    }
}

void HAL_I2SEx_TxRxHalfCpltCallback(I2S_HandleTypeDef* hi2s)
{
    UNUSED(hi2s);
    est_time_ms += 10;
    CheckFlags();
    WakeUp();
    audio_chip.HalfCompleteCallback();
    RaiseFlag(Audio_Interrupt);
}

void HAL_I2SEx_TxRxCpltCallback(I2S_HandleTypeDef* hi2s)
{
    UNUSED(hi2s);
    est_time_ms += 10;
    CheckFlags();
    WakeUp();
    audio_chip.CompleteCallback();
    RaiseFlag(Audio_Interrupt);
}


void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef* hspi)
{
    UNUSED(hspi);
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
