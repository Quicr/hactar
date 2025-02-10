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

// Buffer declarations
static constexpr uint16_t net_ui_serial_tx_buff_sz = 2048;
uint8_t net_ui_serial_tx_buff[net_ui_serial_tx_buff_sz] = { 0 };
static constexpr uint16_t net_ui_serial_rx_buff_sz = 2048;
uint8_t net_ui_serial_rx_buff[net_ui_serial_rx_buff_sz] = { 0 };
static constexpr uint16_t net_ui_serial_num_rx_packets = 30;

uint8_t link_send_space[20] = { 0 }; // change as needed

static AudioChip audio_chip(hi2s3, hi2c1);

static Serial serial(&huart2, net_ui_serial_num_rx_packets,
    *net_ui_serial_tx_buff, net_ui_serial_tx_buff_sz,
    *net_ui_serial_rx_buff, net_ui_serial_rx_buff_sz);

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
link_packet_t* link_packet = nullptr;

volatile bool sleeping = true;
volatile bool error = false;
bool net_replied = false;
constexpr uint32_t Expected_Flags = (1 << Timer_Flags_Count) - 1 ;
uint32_t flags = Expected_Flags;
uint32_t est_time_ms = 0;
uint32_t num_loops = 0;
uint32_t ticks_ms = 0;
uint32_t num_audio_req_packets = 0;

ui_net_link::AudioObject play_frame;

uint32_t num_req_sent = 0;
uint32_t num_packets_rx = 0;
uint32_t num_packets_tx = 0;

ui_net_link::AudioObject talk_frame = { 0, 0 };
bool started = false;
uint32_t last_rx_time = 0;

bool stop = false;


int app_main()
{

    for (int i = 0; i < 320; ++i)
    {
        talk_frame.data[i] = i % 0xFF;
    }

    audio_chip.Init();
    audio_chip.StartI2S();
    InitScreen();
    LEDS(LOW, LOW, LOW);
    serial.StartReceive();
    LEDS(HIGH, HIGH, HIGH);

    // WaitForNetReady();
    InterHactarSerialRoundTripTest(20, -1);

    uint32_t redraw = uwTick;
    Colour next = Colour::Green;
    Colour curr = Colour::Blue;
    const char* hello = "Hello1 Hello2 Hello3 Hello4 Hello5 Hello6 Hello7";

    bool ptt_down = false;

    uint32_t blinky = 0;
    uint32_t next_print = 0;

    while (1)
    {
        if (HAL_GetTick() > blinky)
        {
            HAL_GPIO_TogglePin(UI_LED_G_GPIO_Port, UI_LED_G_Pin);
            blinky = HAL_GetTick() + 2000;
        }

        num_loops++;
        if (HAL_GetTick() > next_print)
        {
            UI_LOG_ERROR("rx %lu, tx %lu", num_packets_rx, num_packets_tx);
            next_print = HAL_GetTick() + 1000;
        }

        while (sleeping)
        {
            // LowPowerMode();
        }

        if (error)
        {
            // Error("Main loop", "Flags did not match expected");
        }

        // Send talk start and sot packets
        if (HAL_GPIO_ReadPin(PTT_BTN_GPIO_Port, PTT_BTN_Pin) == GPIO_PIN_RESET && !ptt_down)
        {
            // TODO channel id
            ui_net_link::TalkStart talk_start = { 0 };
            ui_net_link::Serialize(talk_start, talk_packet);
            serial.Write(talk_packet);
            // ++num_packets_tx;
            ptt_down = true;
        }
        else if (HAL_GPIO_ReadPin(PTT_BTN_GPIO_Port, PTT_BTN_Pin) == GPIO_PIN_SET && ptt_down)
        {
            // ui_net_link::TalkStop talk_stop = { 0 };
            // ui_net_link::Serialize(talk_stop, talk_packet);
            // serial.Write(talk_packet);
            // // ++num_packets_tx;
            // ptt_down = false;
        }

        if (ptt_down)
        {
            AudioCodec::ALawCompand(audio_chip.RxBuffer(), talk_frame.data, constants::Audio_Buffer_Sz_2);
            ui_net_link::Serialize(talk_frame, talk_packet);
            HAL_GPIO_TogglePin(UI_LED_R_GPIO_Port, UI_LED_R_Pin);
            serial.Write(talk_packet);
            ++num_packets_tx;
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
            // const uint16_t width = 50;
            // const uint16_t height = 50;
            // const uint16_t x_inc = width + 2;
            // const uint16_t y_inc = height + 1;

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
    while (true)
    {
        link_packet = serial.Read();
        if (!link_packet)
        {
            break;
        }

        ++num_packets_rx;
        started = true;
        last_rx_time = HAL_GetTick();

        switch ((ui_net_link::Packet_Type)link_packet->type)
        {
            case ui_net_link::Packet_Type::TalkStart:
            case ui_net_link::Packet_Type::TalkStop:
            case ui_net_link::Packet_Type::GetAudioLinkPacket:
            {
                break;
            }
            case ui_net_link::Packet_Type::AudioMultiObject:
            case ui_net_link::Packet_Type::AudioObject:
            {
                HAL_GPIO_TogglePin(UI_LED_B_GPIO_Port, UI_LED_B_Pin);
                ui_net_link::Deserialize(*link_packet, play_frame);
                AudioCodec::ALawExpand(play_frame.data, audio_chip.TxBuffer(), constants::Audio_Buffer_Sz_2);
                --num_audio_req_packets;
                if (num_audio_req_packets == 0)
                {
                    HAL_GPIO_WritePin(UI_STAT_GPIO_Port, UI_STAT_Pin, LOW);
                }
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
                UI_LOG_ERROR("Packet type %d, %u, %lu, %lu", (int)link_packet->type, link_packet->length, num_audio_req_packets, num_packets_rx);
                // Error("Link packet handler", "Received a packet type that has no handler");
                last_rx_time = 0;
                serial.Reset();
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
    // while (HAL_GPIO_ReadPin(UI_STAT_GPIO_Port, UI_STAT_Pin) == GPIO_PIN_RESET)
    // {
    //     HAL_Delay(10);
    // }
}

inline void AudioCallback()
{
    audio_chip.ISRCallback();
    est_time_ms += 20;
    ticks_ms = HAL_GetTick();
    CheckFlags();
    WakeUp();

    HAL_GPIO_WritePin(UI_STAT_GPIO_Port, UI_STAT_Pin, HIGH);
    ++num_audio_req_packets;
    RaiseFlag(Audio_Interrupt);
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart, uint16_t size)
{
    // UI_LOG_ERROR("rx %u", size);
    if (huart->Instance == Serial::UART(&serial)->Instance)
    {
        Serial::RxISR(&serial, size);
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart)
{
    if (huart->Instance == Serial::UART(&serial)->Instance)
    {
        Serial::TxISR(&serial);
        RaiseFlag(Rx_Audio_Transmitted);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart)
{
    if (huart->Instance == Serial::UART(&serial)->Instance)
    {
        serial.Stop();
        serial.StartReceive();
    }
}

void HAL_UART_AbortReceiveCpltCallback(UART_HandleTypeDef* huart)
{
    if (huart->Instance == Serial::UART(&serial)->Instance)
    {
        serial.StartReceive();
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

inline void Error(const char* who, const char* why)
{
    // Disable interrupts
    UI_LOG_ERROR("Error has occurred; who: %s; why: %s", who, why);
    audio_chip.StopI2S();
    serial.Stop();

    HAL_GPIO_WritePin(UI_LED_B_GPIO_Port, UI_LED_B_Pin, HIGH);
    HAL_GPIO_WritePin(UI_LED_G_GPIO_Port, UI_LED_G_Pin, HIGH);
    while (1)
    {
        HAL_GPIO_TogglePin(UI_LED_R_GPIO_Port, UI_LED_R_Pin);
        HAL_Delay(1000);
    }
}


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

void InterHactarSerialRoundTripTest(int delay, int num)
{
    HAL_Delay(1000);
    UI_LOG_INFO("Start");

    const size_t buff_sz = 321;
    link_packet_t tmp;
    // Fill the tmp audio buffer with random
    tmp.type = (uint8_t)ui_net_link::Packet_Type::AudioObject;
    tmp.length = buff_sz;
    for (size_t i = 0; i < buff_sz; ++i)
    {
        tmp.payload[i] = rand() % 0xFF;
    }

    uint64_t num_good = 0;
    uint64_t total_packets = 0;

    link_packet_t* recv = nullptr;
    int idx = 0;
    uint32_t err_timeout = 0;
    uint32_t timeout = 0;
    uint32_t total_elapsed = 0;
    uint32_t start = 0;
    uint32_t end = 0;
    while (true)
    {
        start = HAL_GetTick();
        UI_LOG_INFO("Tx");
        serial.Write(tmp);
        err_timeout = HAL_GetTick() + 1000;
        HAL_Delay(delay);

        while (true)
        {
            recv = serial.Read();
            if (recv != nullptr)
            {
                if (HAL_GetTick() > err_timeout)
                {
                    Error("InterHactarSerialRoundTripTest", "Never received loopback");
                }
                break;
            }
        }
        UI_LOG_INFO("Rx");


        end = HAL_GetTick();


        // Compare the two
        for (size_t i = 0 ; i < buff_sz;++i)
        {
            if (tmp.payload[i] != recv->payload[i])
            {
                while (true)
                {
                    Error("InterHactarSerialRoundTripTest", "Send didn't match received");
                }
            }
        }
        ++num_good;

        for (size_t i = 0; i < buff_sz; ++i)
        {
            tmp.payload[i] = rand() % 0xFF;
        }

        total_elapsed += end - start;
        if (num_good >= 100)
        {
            total_packets += num_good;
            UI_LOG_INFO("t %ld r %lld ", total_elapsed / 100, total_packets);

            total_elapsed = 0;
            num_good = 0;
        }

        if (idx++ == num)
        {
            break;
        }

        if (HAL_GetTick() > timeout)
        {
            HAL_GPIO_TogglePin(UI_LED_G_GPIO_Port, UI_LED_G_Pin);
            timeout = HAL_GetTick() + 2000;
        }
    }
}

void InterHactarFullRoundTripTest(int delay, int num)
{
    HAL_Delay(20000);
    UI_LOG_INFO("Start");

    const size_t buff_sz = 321;
    link_packet_t tmp;
    // Fill the tmp audio buffer with random
    tmp.type = (uint8_t)ui_net_link::Packet_Type::AudioObject;
    tmp.length = buff_sz;
    for (size_t i = 0; i < buff_sz; ++i)
    {
        tmp.payload[i] = rand() % 0xFF;
    }

    ui_net_link::BuildGetLinkPacket(link_send_space);
    serial.Write(link_send_space, 3);
    HAL_Delay(1);

    for (int i = 0; i < 20; ++i)
    {
        serial.Write(tmp);
        HAL_Delay(delay);
    }
    UI_LOG_WARN("Sent");

    uint64_t num_recv = 0;

    link_packet_t* recv = nullptr;
    int idx = 0;
    uint32_t timeout = 0;
    uint32_t total_elapsed = 0;
    uint32_t total_sent = 0;
    while (true)
    {
        HAL_Delay(delay);
        serial.Write(tmp);
        uint32_t start = HAL_GetTick();
        serial.Write(link_send_space, 3);

        while (true)
        {
            recv = serial.Read();
            if (recv != nullptr)
            {
                break;
            }
        }

        ++num_recv;

        uint32_t end = HAL_GetTick();
        total_elapsed += (end - start);
        if (num_recv >= 10)
        {
            total_sent += num_recv;
            UI_LOG_INFO("t %ld s %ld", total_elapsed / 10, total_sent);

            total_elapsed = 0;
            num_recv = 0;
        }

        if (idx++ == num)
        {
            break;
        }

        if (HAL_GetTick() > timeout)
        {
            HAL_GPIO_TogglePin(UI_LED_G_GPIO_Port, UI_LED_G_Pin);
            timeout = HAL_GetTick() + 2000;
        }
    }
}

void DumpRxBuff()
{
    char dmp[256];
    UI_LOG_INFO("rx w idx %u, rx r idx %u", serial.RxBuffWriteIdx(), serial.RxBuffReadIdx());
    // Dump the rx buff slowly
    for (int i = 0 ; i < net_ui_serial_rx_buff_sz; ++i)
    {
        itoa(i, dmp, 10);
        HAL_UART_Transmit(&huart1, (uint8_t*)dmp, strlen(dmp), HAL_MAX_DELAY);

        HAL_UART_Transmit(&huart1, (uint8_t*)": ", 2, HAL_MAX_DELAY);
        itoa(net_ui_serial_rx_buff[i], dmp, 10);
        HAL_UART_Transmit(&huart1, (uint8_t*)dmp, strlen(dmp), HAL_MAX_DELAY);

        HAL_UART_Transmit(&huart1, (uint8_t*)"\n", 1, HAL_MAX_DELAY);
        HAL_Delay(2);
    }
    Error("Main loop", "rx stopped receiving data, dumping");
}

void LedROn()
{
    HAL_GPIO_WritePin(UI_LED_R_GPIO_Port, UI_LED_R_Pin, LOW);
}

void LedROff()
{
    HAL_GPIO_WritePin(UI_LED_R_GPIO_Port, UI_LED_R_Pin, HIGH);
}

void LedRToggle()
{
    HAL_GPIO_TogglePin(UI_LED_R_GPIO_Port, UI_LED_R_Pin);
}

void LedBOn()
{
    HAL_GPIO_WritePin(UI_LED_B_GPIO_Port, UI_LED_B_Pin, LOW);
}

void LedBOff()
{
    HAL_GPIO_WritePin(UI_LED_B_GPIO_Port, UI_LED_B_Pin, HIGH);
}

void LedBToggle()
{
    HAL_GPIO_TogglePin(UI_LED_B_GPIO_Port, UI_LED_B_Pin);
}

void LedGOn()
{
    HAL_GPIO_WritePin(UI_LED_G_GPIO_Port, UI_LED_G_Pin, LOW);
}

void LedGOff()
{
    HAL_GPIO_WritePin(UI_LED_G_GPIO_Port, UI_LED_G_Pin, HIGH);
}

void LedGToggle()
{
    HAL_GPIO_TogglePin(UI_LED_G_GPIO_Port, UI_LED_G_Pin);
}

void LedsOn()
{
    LedROn();
    LedBOn();
    LedGOn();
}

void LedsOff()
{
    LedROff();
    LedBOff();
    LedGOff();
}

void LedsToggle()
{
    LedRToggle();
    LedBToggle();
    LedGToggle();
}