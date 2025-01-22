#include "main.h"
#include "app_main.hh"

#include "audio_chip.hh"
#include "audio_codec.hh"

#include "screen.hh"

#include "serial.hh"

#include "link_packet_t.hh"
#include "ui_net_link.hh"
#include "logger.hh"

#include <string.h>

#include <random>


#define HIGH GPIO_PIN_SET
#define LOW GPIO_PIN_RESET

void reverse(char* str, int length)
{
    int start = 0;
    int end = length - 1;
    while (start < end)
    {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

char* itoa(int value, char* str, int base)
{
    if (base < 2 || base > 36)
    { // Base check: only supports bases 2-36
        *str = '\0';
        return str;
    }

    bool isNegative = false;
    if (value < 0 && base == 10)
    { // Handle negative numbers in base 10
        isNegative = true;
        value = -value;
    }

    int i = 0;
    do
    {
        int digit = value % base;
        str[i++] = (digit > 9) ? (digit - 10 + 'a') : (digit + '0');
        value /= base;
    } while (value != 0);

    if (isNegative)
    {
        str[i++] = '-';
    }

    str[i] = '\0'; // Null-terminate the string

    reverse(str, i); // Reverse the string to correct order

    return str;
}

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
    Screen::Orientation::flipped_portrait
);

link_packet_t talk_packet;
link_packet_t play_buffer;
link_packet_t* play_packet = nullptr;
uint8_t link_send_space[20] = { 0 }; // change as needed

size_t num_packets_recv = 0;

volatile bool sleeping = true;
volatile bool error = false;
bool net_replied = false;
constexpr uint32_t Expected_Flags = (1 << Timer_Flags_Count) - 1 ;
uint32_t flags = Expected_Flags;
uint32_t est_time_ms = 0;
uint32_t num_loops = 0;
uint32_t ticks_ms = 0;
uint32_t num_awaiting_packets = 0;

ui_net_link::AudioObject play_frame;

int app_main()
{
    HAL_Delay(1000);
    audio_chip.Init();
    InitScreen();
    LEDS(LOW, LOW, LOW);
    serial.StartReceive();
    LEDS(HIGH, HIGH, HIGH);


    uint32_t timeout;
    uint32_t current_tick;
    uint32_t redraw = uwTick;
    Colour next = Colour::Green;
    Colour curr = Colour::Blue;
    const char* hello = "Hello1 Hello2 Hello3 Hello4 Hello5 Hello6 Hello7";

    bool ptt_down = false;

    // TODO Fix
    WaitForNetReady();

    // TODO Probe for a reply from the net chip using serial
    // SlowSendTest(100, 10);

    // TODO test clock stability by switching a debug pin on and off and measure it with the scope
    HAL_Delay(5000);
    audio_chip.StartI2S();
    bool stop = false;

    uint32_t blinky = 0;

    uint32_t time_start = HAL_GetTick();


    while (1)
    {
        if (HAL_GetTick() > blinky)
        {
            HAL_GPIO_TogglePin(UI_LED_G_GPIO_Port, UI_LED_G_Pin);
            // Logger::Log(Logger::Level::Error, "UI ALIVE");
            blinky = HAL_GetTick() + 2000;
        }

        num_loops++;
        while (sleeping)
        {
            // LowPowerMode();
        }

        if (error)
        {
            // Error_Handler();
        }

        if (num_awaiting_packets < 2)
        {
            // ask for packets?
            ui_net_link::BuildGetLinkPacket(link_send_space);
            serial.Write(link_send_space, 3);
            ++num_awaiting_packets;
        }


        // Send talk start and sot packets
        if (HAL_GPIO_ReadPin(PTT_BTN_GPIO_Port, PTT_BTN_Pin) == GPIO_PIN_RESET && !ptt_down)
        {
            // TODO channel id
            ui_net_link::TalkStart talk_start = { 0 };
            ui_net_link::Serialize(talk_start, talk_packet);
            serial.Write(talk_packet);
            Logger::Log(Logger::Level::Error, "PTT pressed");
            ptt_down = true;
        }
        else if (HAL_GPIO_ReadPin(PTT_BTN_GPIO_Port, PTT_BTN_Pin) == GPIO_PIN_SET && ptt_down)
        {
            ui_net_link::TalkStop talk_stop = { 0 };
            ui_net_link::Serialize(talk_stop, talk_packet);
            serial.Write(talk_packet);
            Logger::Log(Logger::Level::Error, "PTT released");
            ptt_down = false;
        }

        if (ptt_down)
        {
            ui_net_link::AudioObject talk_frame = { 0, 0 };

            AudioCodec::ALawCompand(audio_chip.RxBuffer(), talk_frame.data, constants::Audio_Buffer_Sz_2);
            ui_net_link::Serialize(talk_frame, talk_packet);
            serial.Write(talk_packet);
        }
        RaiseFlag(Rx_Audio_Companded);
        RaiseFlag(Rx_Audio_Transmitted);

        HandleRecvLinkPackets();

        // Use remaining time to draw
        if (est_time_ms > redraw)
        {
            screen.FillRectangle(0, WIDTH, 0, HEIGHT, curr);
            screen.DrawRectangle(0, 10, 0, 10, 2, next);
            screen.DrawCharacter(11, 0, 'h', font6x8, next, curr);
            screen.DrawString(0, 28, hello, 48, font5x8, next, curr);
            const uint16_t width = 50;
            const uint16_t height = 50;
            const uint16_t x_inc = width + 2;
            const uint16_t y_inc = height + 1;

            // swap
            Colour tmp = curr;
            curr = next;
            next = tmp;
            redraw = est_time_ms + 1600;
        }

        // Draw what we can
        screen.Draw(0);
        RaiseFlag(Draw_Complete);

        sleeping = true;
        stop = true;
    }

    return 0;
}

inline void LEDR(GPIO_PinState state)
{
    HAL_GPIO_WritePin(UI_LED_R_GPIO_Port, UI_LED_R_Pin, state);
}

inline void LEDG(GPIO_PinState state)
{
    HAL_GPIO_WritePin(UI_LED_G_GPIO_Port, UI_LED_G_Pin, state);
}

inline void LEDB(GPIO_PinState state)
{
    HAL_GPIO_WritePin(UI_LED_B_GPIO_Port, UI_LED_B_Pin, state);
}

inline void LEDS(GPIO_PinState r, GPIO_PinState g, GPIO_PinState b)
{
    HAL_GPIO_WritePin(UI_LED_R_GPIO_Port, UI_LED_R_Pin, r);
    HAL_GPIO_WritePin(UI_LED_G_GPIO_Port, UI_LED_G_Pin, g);
    HAL_GPIO_WritePin(UI_LED_B_GPIO_Port, UI_LED_B_Pin, b);
}

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
    HAL_SuspendTick();
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFE);
}

inline void WakeUp()
{

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

void HandleRecvLinkPackets()
{
    // If there are bytes available read them
    play_packet = serial.Read();
    if (play_packet)
    {
        if (++num_packets_recv % 100 == 0)
        {
            Logger::Log(Logger::Level::Info, ".");
            num_packets_recv = 0;
        }

        switch ((ui_net_link::Packet_Type)play_packet->type)
        {
            case ui_net_link::Packet_Type::AudioMultiObject:
            case ui_net_link::Packet_Type::AudioObject:
            {
                --num_awaiting_packets;
                ui_net_link::Deserialize(*play_packet, play_frame);
                AudioCodec::ALawExpand(play_frame.data, audio_chip.TxBuffer(), constants::Audio_Buffer_Sz_2);
                break;
            };
            case ui_net_link::Packet_Type::MoQStatus:
            {
                break;
            }
            case ui_net_link::Packet_Type::WifiConnect:
            {
                break;
            }
            case ui_net_link::Packet_Type::WifiStatus:
            {
                break;
            }
            default:
            {
                play_packet->is_ready = false;
                Error_Handler();
                break;
            }
        }
    }
}


// inline void ProcessText(uint16_t len)
// {
//     // Get the room
//     uint8_t room[QMessage_Room_Length];
//     serial.Read(room, QMessage_Room_Length, QMessage_Room_Length);
//     len -= Serial_Header_Sz;

//     uint8_t* serial_data = nullptr;
//     size_t num_bytes = len;

//     while (len > 0)
//     {
//         // Get the text
//         serial.Read(&serial_data, num_bytes);

//         // Now we have the text data
//         screen.AppendText((const char*)serial_data, num_bytes);

//         len -= num_bytes;
//     }

//     // Commit the text
//     screen.CommitText();
// }

void InitScreen()
{
    screen.Init();
    // Do the first draw
    screen.FillRectangle(0, WIDTH, 0, HEIGHT, Colour::Black);
    for (int i = 0; i < 320; i += Screen::Num_Rows)
    {
        screen.Draw(0xFFFF);
    }
    screen.EnableBacklight();
}

void WaitForNetReady()
{
    while (HAL_GPIO_ReadPin(UI_STAT_GPIO_Port, UI_STAT_Pin) == GPIO_PIN_RESET)
    {
        HAL_Delay(10);
    }
}

inline void AudioCallback()
{
    audio_chip.ISRCallback();
    est_time_ms += 20;
    ticks_ms = HAL_GetTick();
    CheckFlags();
    WakeUp();
    RaiseFlag(Audio_Interrupt);
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart, uint16_t size)
{
    if (huart->Instance == USART2)
    {
        // Logger::Log(Logger::Level::Info, "rx", size);
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
    AudioCallback();
}

void HAL_I2SEx_TxRxCpltCallback(I2S_HandleTypeDef* hi2s)
{
    UNUSED(hi2s);
    AudioCallback();
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



void SlowSendTest(int delay, int num)
{
    link_packet_t packet;
    ui_net_link::AudioObject talk_frame = { 0, 0 };
    // Fill the tmp audio buffer with random
    for (int i = 0; i < constants::Audio_Buffer_Sz_2; ++i)
    {
        talk_frame.data[i] = i;
    }

    ui_net_link::Serialize(talk_frame, packet);

    int i = 0;
    while (i++ != num)
    {
        HAL_Delay(delay);
        serial.Write(packet);
    }
}

void InterHactarRoundTripTest()
{
    // TODO fix
    // uint8_t tmp[Serial_Audio_Buff_Sz] = {
    //     Serial_Audio_Buff_Sz & 0xFF, Serial_Audio_Buff_Sz >> 8,
    //     Serial::Packet_Type::Audio
    // };

    // // Fill the tmp audio buffer with random
    // for (int i = 35; i < Serial_Audio_Buff_Sz; ++i)
    // {
    //     tmp[i] = rand() % 0xFF;
    // }

    // while (true)
    // {
    //     HAL_Delay(100);
    //     serial.Write(tmp, Serial_Audio_Buff_Sz);
    //     HAL_Delay(20);
    //     serial.Read(serial_rx_buffer, 400, Serial_Audio_Buff_Sz);

    //     // Compare the two
    //     bool is_eq = true;
    //     for (int i = 0 ; i < Serial_Audio_Buff_Sz;++i)
    //     {
    //         if (tmp[i] != serial_rx_buffer[i])
    //         {
    //             while (true)
    //             {
    //                 Error_Handler();
    //             }
    //         }
    //     }
    // }
}