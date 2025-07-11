#include "app_main.hh"
#include "audio_chip.hh"
#include "audio_codec.hh"
#include "keyboard.hh"
#include "link_packet_t.hh"
#include "logger.hh"
#include "main.h"
#include "renderer.hh"
#include "screen.hh"
#include "serial.hh"
#include "tools.hh"
#include "ui_net_link.hh"
#include <cmox_crypto.h>
#include <cmox_init.h>
#include <cmox_low_level.h>
#include <sframe/sframe.h>
#include <stm32f4xx_hal.h>
#include <random>

// Forward declare
inline void CheckPTT();
inline void CheckPTTAI();
inline void
SendAudio(const uint8_t channel_id, const ui_net_link::Packet_Type packet_type, bool last);
inline void HandleAiResponse(link_packet_t* packet);
inline void HandleKeypress();
inline bool TryProtect(link_packet_t* link_packet);
inline bool TryUnprotect(link_packet_t* link_packet);

constexpr const char* mls_key = "sixteen byte key";
sframe::MLSContext mls_ctx(sframe::CipherSuite::AES_GCM_128_SHA256, 1);
uint8_t dummy_ciphertext[link_packet_t::Payload_Size];

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

char* itoa(int value, char* str, int& len, int base)
{
    if (base < 2 || base > 36)
    { // Base check: only supports bases 2-36
        *str = '\0';
        len = 0;
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

    len = i;
    str[i] = '\0'; // Null-terminate the string

    reverse(str, i); // Reverse the string to correct order

    return str;
}

cmox_init_retval_t cmox_ll_init(void* pArg)
{
    (void)pArg;
    /* Ensure CRC is enabled for cryptographic processing */
    __HAL_RCC_CRC_RELEASE_RESET();
    __HAL_RCC_CRC_CLK_ENABLE();
    return CMOX_INIT_SUCCESS;
}

cmox_init_retval_t cmox_ll_deInit(void* pArg)
{
    (void)pArg;
    /* Do not turn off CRC to avoid side effect on other SW parts using it */
    return CMOX_INIT_SUCCESS;
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
uint8_t net_ui_serial_tx_buff[net_ui_serial_tx_buff_sz] = {0};
static constexpr uint16_t net_ui_serial_rx_buff_sz = 2048;
uint8_t net_ui_serial_rx_buff[net_ui_serial_rx_buff_sz] = {0};
static constexpr uint16_t net_ui_serial_num_rx_packets = 7;

static AudioChip audio_chip(hi2s3, hi2c1);

static Serial serial(&huart2,
                     net_ui_serial_num_rx_packets,
                     *net_ui_serial_tx_buff,
                     net_ui_serial_tx_buff_sz,
                     *net_ui_serial_rx_buff,
                     net_ui_serial_rx_buff_sz);

Screen screen(hspi1,
              DISP_CS_GPIO_Port,
              DISP_CS_Pin,
              DISP_DC_GPIO_Port,
              DISP_DC_Pin,
              DISP_RST_GPIO_Port,
              DISP_RST_Pin,
              DISP_BL_GPIO_Port,
              DISP_BL_Pin,
              Screen::Orientation::flipped_portrait);

link_packet_t message_packet;
link_packet_t* link_packet = nullptr;

volatile bool sleeping = true;
volatile bool error = false;

constexpr uint32_t Expected_Flags = (1 << Timer_Flags_Count) - 1;
uint32_t flags = Expected_Flags;

uint32_t est_time_ms = 0;
uint32_t ticks_ms = 0;
uint32_t num_audio_req_packets = 0;

ui_net_link::AudioObject play_frame;

uint32_t num_req_sent = 0;
uint32_t num_packets_rx = 0;
uint32_t num_packets_tx = 0;

ui_net_link::AudioObject talk_frame = {0, 0};

GPIO_TypeDef* col_ports[Keyboard::Q10_Cols] = {
    KB_COL1_GPIO_Port, KB_COL2_GPIO_Port, KB_COL3_GPIO_Port, KB_COL4_GPIO_Port, KB_COL5_GPIO_Port,
};

uint16_t col_pins[Keyboard::Q10_Cols] = {
    KB_COL1_Pin, KB_COL2_Pin, KB_COL3_Pin, KB_COL4_Pin, KB_COL5_Pin,
};

GPIO_TypeDef* row_ports[Keyboard::Q10_Rows] = {
    KB_ROW1_GPIO_Port, KB_ROW2_GPIO_Port, KB_ROW3_GPIO_Port, KB_ROW4_GPIO_Port,
    KB_ROW5_GPIO_Port, KB_ROW6_GPIO_Port, KB_ROW7_GPIO_Port,
};

uint16_t row_pins[Keyboard::Q10_Rows] = {
    KB_ROW1_Pin, KB_ROW2_Pin, KB_ROW3_Pin, KB_ROW4_Pin, KB_ROW5_Pin, KB_ROW6_Pin, KB_ROW7_Pin,
};

static constexpr uint16_t kb_ring_buff_sz = 5;
RingBuffer<uint8_t> kb_buff(kb_ring_buff_sz);

Keyboard keyboard(col_ports, col_pins, row_ports, row_pins, kb_buff, 150, 150);

uint32_t timeout = 0;

bool ptt_down = false;
const uint8_t ptt_channel = 0;

bool ptt_ai_down = false;
const uint8_t ptt_ai_channel = 1;

const uint8_t text_channel = 2;

int app_main()
{
    HAL_TIM_Base_Start_IT(&htim2);

    if (cmox_initialize(nullptr) != CMOX_INIT_SUCCESS)
    {
        Error("main", "cmox failed to initialise");
    }

    mls_ctx.add_epoch(0, sframe::input_bytes{reinterpret_cast<const uint8_t*>(&mls_key[0]), 16});

    Renderer renderer(screen, keyboard);

    audio_chip.Init();
    audio_chip.StartI2S();
    InitScreen();
    LEDS(LOW, LOW, LOW);
    serial.StartReceive();
    LEDS(HIGH, HIGH, HIGH);

    uint32_t redraw = uwTick;

    uint32_t blinky = 0;
    uint32_t next_print = 0;

    // Test in case the audio chip settings change and something looks suspicious
    CountNumAudioInterrupts(audio_chip, sleeping);

    // TODO remove once we have a proper loading screen/view implementation
    const uint32_t loading_done_timeout = HAL_GetTick();
    bool done_booting = false;

    UI_LOG_INFO("Starting main loop");
    while (1)
    {
        Heartbeat(UI_LED_R_GPIO_Port, UI_LED_R_Pin);

        if (!done_booting && HAL_GetTick() - loading_done_timeout >= 2000)
        {
            renderer.ChangeView(Renderer::View::Chat);
            done_booting = true;
        }

        if (HAL_GetTick() > next_print)
        {
            // UI_LOG_ERROR("rx %lu, tx %lu", num_packets_rx, num_packets_tx);
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

        CheckPTT();
        CheckPTTAI();

        HandleKeypress();

        RaiseFlag(Rx_Audio_Companded);
        RaiseFlag(Rx_Audio_Transmitted);

        HandleRecvLinkPackets();

        renderer.Render(ticks_ms);
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
            return;
        }

        // UI_LOG_INFO("Got a packet, type %d, first byte %d", (int)link_packet->type,
        //             (int)link_packet->payload[0]);
        ++num_packets_rx;
        switch ((ui_net_link::Packet_Type)link_packet->type)
        {
        case ui_net_link::Packet_Type::PowerOnReady:
        {
            // TODO: Stop loading screen?
            break;
        }
        case ui_net_link::Packet_Type::TalkStart:
        case ui_net_link::Packet_Type::TalkStop:
        case ui_net_link::Packet_Type::GetAudioLinkPacket:
        {
            break;
        }
        case ui_net_link::Packet_Type::TextMessage:
        {
            UI_LOG_INFO("Got text message");

            if (!TryUnprotect(link_packet))
            {
                UI_LOG_ERROR("Failed to decrypt text message");
                continue;
            }

            // Ignore the first byte
            constexpr uint8_t payload_offset = 6;
            char* text = (char*)(link_packet->payload + payload_offset);

            screen.CommitText(text, link_packet->length - payload_offset);
            break;
        }
        case ui_net_link::Packet_Type::AiResponse:
        {
            UI_LOG_INFO("Got an ai response");

            HandleAiResponse(link_packet);
            break;
        }
        case ui_net_link::Packet_Type::PttMultiObject:
        case ui_net_link::Packet_Type::PttObject:
        {

            if (!TryUnprotect(link_packet))
            {
                UI_LOG_ERROR("Failed to decrypt ptt object");
                continue;
            }

            // TODO we need to know if it is mono or stereo data
            // that we have received
            // For now assume we receive mono
            ui_net_link::Deserialize(*link_packet, play_frame);
            AudioCodec::ALawExpand(play_frame.data, constants::Audio_Phonic_Sz,
                                   audio_chip.TxBuffer(), constants::Audio_Buffer_Sz,
                                   constants::Stereo, true);
            break;
        }
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
            UI_LOG_INFO("got a wifi packet, status %d", (int)link_packet->payload[0]);

            break;
        }
        default:
        {
            UI_LOG_ERROR("Unhandled packet type %d, %u, %lu, %lu", (int)link_packet->type,
                         link_packet->length, num_audio_req_packets, num_packets_rx);
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

inline void AudioCallback()
{
    audio_chip.ISRCallback();
    est_time_ms += 20;
    ticks_ms = HAL_GetTick();
    CheckFlags();
    WakeUp();

    HAL_GPIO_WritePin(UI_READY_GPIO_Port, UI_READY_Pin, HIGH);
    HAL_TIM_Base_Start_IT(&htim3);

    ++num_audio_req_packets;
    RaiseFlag(Audio_Interrupt);
}

void CheckPTT()
{
    // Send talk start and sot packets
    if (HAL_GPIO_ReadPin(PTT_BTN_GPIO_Port, PTT_BTN_Pin) == GPIO_PIN_SET && !ptt_down)
    {
        // TODO channel id
        ui_net_link::TalkStart talk_start = {0};
        talk_start.channel_id = ptt_channel;
        ui_net_link::Serialize(talk_start, message_packet);
        serial.Write(message_packet);
        ptt_down = true;
        LedGOn();
    }
    else if (HAL_GPIO_ReadPin(PTT_BTN_GPIO_Port, PTT_BTN_Pin) == GPIO_PIN_RESET && ptt_down)
    {
        ui_net_link::TalkStop talk_stop = {0};
        talk_stop.channel_id = ptt_channel;
        ui_net_link::Serialize(talk_stop, message_packet);
        serial.Write(message_packet);
        ptt_down = false;
        SendAudio(ptt_channel, ui_net_link::Packet_Type::PttObject, true);
        LedGOff();
    }

    if (ptt_down)
    {
        SendAudio(ptt_channel, ui_net_link::Packet_Type::PttObject, false);
    }
}

void CheckPTTAI()
{

    // Send talk start and sot packets
    if (HAL_GPIO_ReadPin(PTT_AI_BTN_GPIO_Port, PTT_AI_BTN_Pin) == GPIO_PIN_SET && !ptt_ai_down)
    {
        // TODO channel id
        ui_net_link::TalkStart talk_start = {0};
        talk_start.channel_id = ptt_ai_channel;
        ui_net_link::Serialize(talk_start, message_packet);
        serial.Write(message_packet);
        ptt_ai_down = true;
        LedBOn();
    }
    else if (HAL_GPIO_ReadPin(PTT_AI_BTN_GPIO_Port, PTT_AI_BTN_Pin) == GPIO_PIN_RESET
             && ptt_ai_down)
    {
        ui_net_link::TalkStop talk_stop = {0};
        talk_stop.channel_id = ptt_ai_channel;
        ui_net_link::Serialize(talk_stop, message_packet);
        serial.Write(message_packet);
        ptt_ai_down = false;
        SendAudio(ptt_ai_channel, ui_net_link::Packet_Type::PttAiObject, true);
        LedBOff();

        FakeChangeChannelPacket();
    }

    if (ptt_ai_down)
    {
        SendAudio(ptt_ai_channel, ui_net_link::Packet_Type::PttAiObject, false);
    }
}

void SendAudio(const uint8_t channel_id, const ui_net_link::Packet_Type packet_type, bool last)
{
    AudioCodec::ALawCompand(audio_chip.RxBuffer(), constants::Audio_Buffer_Sz, talk_frame.data,
                            constants::Audio_Phonic_Sz, true, constants::Stereo);

    talk_frame.channel_id = channel_id;
    ui_net_link::Serialize(talk_frame, packet_type, last, message_packet);

    if (!TryProtect(&message_packet))
    {
        UI_LOG_ERROR("Failed to encrypt audio packet");
        return;
    }

    // LedRToggle();
    serial.Write(message_packet);
    ++num_packets_tx;
}

void HandleAiResponse(link_packet_t* packet)
{
    UI_LOG_INFO("ai response len %d", packet->length);
    if (!TryUnprotect(packet))
    {
        UI_LOG_ERROR("Failed to decrypt ai response packet");
        return;
    }

    auto* response =
        static_cast<ui_net_link::AIResponseChunk*>(static_cast<void*>(packet->payload + 1));

    // UI_LOG_INFO("type %d", (int)response->type);
    // UI_LOG_INFO("request id %lu", response->request_id);
    // UI_LOG_INFO("content type %d", (int)response->content_type);
    // UI_LOG_INFO("last chunk %d", (int)response->last_chunk);
    // UI_LOG_INFO("content length %lu", response->chunk_length);
    // UI_LOG_INFO("start %d end %d", (int)response->chunk_data[0],
    //             (int)response->chunk_data[response->chunk_length - 1]);

    if (response->chunk_data[0] == '{' && response->chunk_data[response->chunk_length - 1] == '}')
    {
        UI_LOG_INFO("IS JSON");
        serial.Write(*packet);
    }
    else
    {
        response->chunk_data[response->chunk_length] = 0;
        UI_LOG_INFO("[AI] %s", response->chunk_data);
    }
}

void HandleKeypress()
{
    while (keyboard.NumAvailable() > 0)
    {
        const uint8_t ch = keyboard.Read();

        HAL_GPIO_WritePin(UI_LED_R_GPIO_Port, UI_LED_R_Pin, GPIO_PIN_RESET);

        switch (ch)
        {
        case Keyboard::Ent:
        {
            ui_net_link::Serialize(text_channel, screen.UserText(), screen.UserTextLength(),
                                   message_packet);

            if (!TryProtect(&message_packet))
            {
                UI_LOG_ERROR("Failed to encrypt text packet");
                return;
            }

            UI_LOG_INFO("Transmit text message");
            serial.Write(message_packet);
            ++num_packets_tx;
            screen.ClearUserText();
            break;
        }
        case Keyboard::Bak:
        {
            screen.BackspaceUserText();
            break;
        }
        default:
        {
            screen.AppendUserText(ch);
            break;
        }
        }
    }
}

bool TryProtect(link_packet_t* packet)
try
{
    uint8_t ct[link_packet_t::Payload_Size];
    auto payload = mls_ctx.protect(
        0, 0, ct, sframe::input_bytes{packet->payload, packet->length}.subspan(1), {});

    std::memcpy(packet->payload + 1, payload.data(), payload.size());
    packet->length = payload.size() + 1;
    return true;
}
catch (const std::exception& e)
{
    return false;
}

bool TryUnprotect(link_packet_t* packet)
try
{
    auto payload = mls_ctx.unprotect(
        sframe::output_bytes{packet->payload, link_packet_t::Payload_Size}.subspan(1),
        sframe::input_bytes{packet->payload, packet->length}.subspan(1), {});
    packet->length = payload.size() + 1;
    return true;
}
catch (const std::exception& e)
{
    UI_LOG_ERROR("%s", e.what());
    return false;
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
        keyboard.Scan(HAL_GetTick());
    }
    else if (htim->Instance == TIM3)
    {
        // LedBToggle();
        HAL_GPIO_WritePin(UI_READY_GPIO_Port, UI_READY_Pin, LOW);
        HAL_TIM_Base_Stop_IT(&htim3);
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
    ui_net_link::AudioObject talk_frame = {0, 0};
    // Fill the tmp audio buffer with random
    for (int i = 0; i < constants::Audio_Buffer_Sz; ++i)
    {
        talk_frame.data[i] = i;
    }

    ui_net_link::Serialize(talk_frame, ui_net_link::Packet_Type::PttObject, false, packet);

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
    tmp.type = (uint8_t)ui_net_link::Packet_Type::PttObject;
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
        end = HAL_GetTick();

        // Compare the two
        for (size_t i = 0; i < buff_sz; ++i)
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
    tmp.type = (uint8_t)ui_net_link::Packet_Type::PttObject;
    tmp.length = buff_sz;
    for (size_t i = 0; i < buff_sz; ++i)
    {
        tmp.payload[i] = rand() % 0xFF;
    }

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
    for (int i = 0; i < net_ui_serial_rx_buff_sz; ++i)
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

void FakeChangeChannelPacket()
{
    link_packet_t packet;
    packet.type = (uint8_t)ui_net_link::Packet_Type::AiResponse;
    packet.length = 237;
    uint8_t fake_data[] = {
        0,   3,   0,   0,   0,   0,   1,   1,   225, 0,   0,   0,   123, 34,  99,  104, 97,
        110, 110, 101, 108, 67,  111, 110, 102, 105, 103, 34,  58,  34,  49,  34,  44,  34,
        99,  104, 97,  110, 110, 101, 108, 95,  110, 97,  109, 101, 34,  58,  34,  112, 108,
        117, 109, 98,  105, 110, 103, 34,  44,  34,  99,  111, 100, 101, 99,  34,  58,  34,
        112, 99,  109, 34,  44,  34,  108, 97,  110, 103, 117, 97,  103, 101, 34,  58,  34,
        101, 110, 45,  85,  83,  34,  44,  34,  115, 97,  109, 112, 108, 101, 114, 97,  116,
        101, 34,  58,  56,  48,  48,  48,  44,  34,  116, 114, 97,  99,  107, 110, 97,  109,
        101, 34,  58,  34,  112, 99,  109, 95,  101, 110, 95,  56,  107, 104, 122, 95,  109,
        111, 110, 111, 95,  105, 49,  54,  34,  44,  34,  116, 114, 97,  99,  107, 110, 97,
        109, 101, 115, 112, 97,  99,  101, 34,  58,  91,  34,  109, 111, 113, 58,  47,  47,
        109, 111, 113, 46,  112, 116, 116, 46,  97,  114, 112, 97,  47,  118, 49,  34,  44,
        34,  111, 114, 103, 47,  97,  99,  109, 101, 34,  44,  34,  115, 116, 111, 114, 101,
        47,  49,  50,  51,  52,  34,  44,  34,  99,  104, 97,  110, 110, 101, 108, 47,  112,
        108, 117, 109, 98,  105, 110, 103, 34,  44,  34,  112, 116, 116, 34,  93,  125};
    for (int i = 0; i < packet.length; ++i)
    {
        packet.payload[i] = fake_data[i];
    }

    serial.Write(packet);
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