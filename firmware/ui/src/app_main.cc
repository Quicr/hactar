#include "app_main.hh"
#include "audio_chip.hh"
#include "audio_codec.hh"
#include "config_storage.hh"
#include "keyboard.hh"
#include "keyboard_display.hh"
#include "led_control.hh"
#include "link_packet_handler.hh"
#include "link_packet_t.hh"
#include "logger.hh"
#include "main.h"
#include "protector.hh"
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
inline void CheckPTT(Protector& protector);
inline void CheckPTTAI(Protector& protector);
inline void SendAudio(Protector& protector,
                      const ui_net_link::Channel_Id channel_id,
                      const ui_net_link::Packet_Type packet_type,
                      bool last);
inline void HandleMedia(link_packet_t* packet);
inline void HandleAiResponse(link_packet_t* packet);

// Handlers
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

extern SPI_HandleTypeDef hspi1;
extern I2C_HandleTypeDef hi2c1;
extern I2S_HandleTypeDef hi2s3;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;
extern TIM_HandleTypeDef htim5;
extern RNG_HandleTypeDef hrng;

// Global variables that need to exist for hardware callbacks
// Buffer allocations
static constexpr uint16_t net_ui_serial_tx_buff_sz = 2048;
uint8_t net_ui_serial_tx_buff[net_ui_serial_tx_buff_sz] = {0};
static constexpr uint16_t net_ui_serial_rx_buff_sz = 2048;
uint8_t net_ui_serial_rx_buff[net_ui_serial_rx_buff_sz] = {0};
static constexpr uint16_t net_ui_serial_num_rx_packets = 7;

static constexpr uint16_t mgmt_ui_serial_tx_buff_sz = 128;
uint8_t mgmt_ui_serial_tx_buff[mgmt_ui_serial_tx_buff_sz] = {0};
static constexpr uint16_t mgmt_ui_serial_rx_buff_sz = 128;
uint8_t mgmt_ui_serial_rx_buff[mgmt_ui_serial_rx_buff_sz] = {0};
static constexpr uint16_t mgmt_ui_serial_num_rx_packets = 1;

static AudioChip audio_chip(hi2s3, hi2c1);

static Serial net_serial(&huart2,
                         net_ui_serial_num_rx_packets,
                         *net_ui_serial_tx_buff,
                         net_ui_serial_tx_buff_sz,
                         *net_ui_serial_rx_buff,
                         net_ui_serial_rx_buff_sz,
                         false);
static Serial mgmt_serial(&huart1,
                          mgmt_ui_serial_num_rx_packets,
                          *mgmt_ui_serial_tx_buff,
                          mgmt_ui_serial_tx_buff_sz,
                          *mgmt_ui_serial_rx_buff,
                          mgmt_ui_serial_rx_buff_sz,
                          false);

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

volatile bool sleeping = true;
volatile bool error = false;

constexpr uint32_t Expected_Flags = (1 << Timer_Flags_Count) - 1;
uint32_t flags = Expected_Flags;

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

enum class Ptt_Btn_State
{
    Released = 0,
    Pressed,
    Releasing,
    Last_Send,
};

Ptt_Btn_State ptt_state;
Ptt_Btn_State ptt_ai_state;

int app_main()
{
    HAL_TIM_Base_Start_IT(&htim2);

    uint32_t ticks_ms = 0;
    ConfigStorage config_storage(hi2c1);
    Protector protector(config_storage);
    Renderer renderer(screen, keyboard);

    audio_chip.Init();
    audio_chip.StartI2S();

    // Test in case the audio chip settings change and something looks suspicious
    // CountNumAudioInterrupts(audio_chip, sleeping);

    InitScreen(screen);
    Leds(HIGH, HIGH, HIGH);
    net_serial.StartReceive();
    mgmt_serial.StartReceive();

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

        while (sleeping)
        {
            // LowPowerMode();
            __NOP();
        }

        ticks_ms = HAL_GetTick();

        if (error)
        {
            // Error("Main loop", "Flags did not match expected");
        }

        CheckPTT(protector);
        CheckPTTAI(protector);

        HandleKeypress(screen, keyboard, net_serial, protector);

        RaiseFlag(Rx_Audio_Companded);
        RaiseFlag(Rx_Audio_Transmitted);

        HandleNetLinkPackets(net_serial, protector, audio_chip, screen);
        HandleMgmtLinkPackets(mgmt_serial, config_storage);

        renderer.Render(ticks_ms);
        RaiseFlag(Draw_Complete);

        sleeping = true;
    }

    return 0;
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

inline void AudioCallback()
{
    audio_chip.ISRCallback();
    CheckFlags();
    WakeUp();

    HAL_GPIO_WritePin(UI_READY_GPIO_Port, UI_READY_Pin, HIGH);
    HAL_TIM_Base_Start_IT(&htim3);

    RaiseFlag(Audio_Interrupt);
}

void CheckPTT(Protector& protector)
{
    // Send talk start and sot packets
    if ((HAL_GPIO_ReadPin(PTT_BTN_GPIO_Port, PTT_BTN_Pin) == GPIO_PIN_SET
         || HAL_GPIO_ReadPin(MIC_IO_GPIO_Port, MIC_IO_Pin) == GPIO_PIN_RESET)
        && ptt_state != Ptt_Btn_State::Pressed)
    {
        HAL_TIM_Base_Stop(&htim4);
        ptt_state = Ptt_Btn_State::Pressed;
        LedGOn();
    }
    else if (HAL_GPIO_ReadPin(PTT_BTN_GPIO_Port, PTT_BTN_Pin) == GPIO_PIN_RESET
             && HAL_GPIO_ReadPin(MIC_IO_GPIO_Port, MIC_IO_Pin) == GPIO_PIN_SET
             && ptt_state == Ptt_Btn_State::Pressed)
    {
        ptt_state = Ptt_Btn_State::Releasing;
        HAL_TIM_Base_Start_IT(&htim4);
    }

    if (ptt_state == Ptt_Btn_State::Pressed || ptt_state == Ptt_Btn_State::Releasing)
    {
        SendAudio(protector, ui_net_link::Channel_Id::Ptt, ui_net_link::Packet_Type::Message,
                  false);
    }

    if (ptt_state == Ptt_Btn_State::Last_Send)
    {
        SendAudio(protector, ui_net_link::Channel_Id::Ptt, ui_net_link::Packet_Type::Message, true);
        ptt_state = Ptt_Btn_State::Released;
    }
}

void CheckPTTAI(Protector& protector)
{

    // Send talk start and sot packets
    if (HAL_GPIO_ReadPin(PTT_AI_BTN_GPIO_Port, PTT_AI_BTN_Pin) == GPIO_PIN_SET
        && ptt_ai_state != Ptt_Btn_State::Pressed)
    {
        HAL_TIM_Base_Stop(&htim5);
        ptt_ai_state = Ptt_Btn_State::Pressed;
        LedBOn();
    }
    else if (HAL_GPIO_ReadPin(PTT_AI_BTN_GPIO_Port, PTT_AI_BTN_Pin) == GPIO_PIN_RESET
             && ptt_ai_state == Ptt_Btn_State::Pressed)
    {
        ptt_ai_state = Ptt_Btn_State::Releasing;
        HAL_TIM_Base_Start_IT(&htim5);
    }

    if (ptt_ai_state == Ptt_Btn_State::Pressed || ptt_ai_state == Ptt_Btn_State::Releasing)
    {
        SendAudio(protector, ui_net_link::Channel_Id::Ptt_Ai, ui_net_link::Packet_Type::Message,
                  false);
    }

    if (ptt_ai_state == Ptt_Btn_State::Last_Send)
    {
        SendAudio(protector, ui_net_link::Channel_Id::Ptt_Ai, ui_net_link::Packet_Type::Message,
                  true);
        ptt_ai_state = Ptt_Btn_State::Released;
    }
}

void SendAudio(Protector& protector,
               const ui_net_link::Channel_Id channel_id,
               const ui_net_link::Packet_Type packet_type,
               bool last)
{
    ui_net_link::AudioObject talk_frame;
    talk_frame.channel_id = channel_id;

    AudioCodec::ALawCompand(audio_chip.RxBuffer(), constants::Audio_Buffer_Sz, talk_frame.data,
                            constants::Audio_Phonic_Sz, true, constants::Stereo);

    link_packet_t message_packet;
    ui_net_link::Serialize(talk_frame, packet_type, last, message_packet);

    if (!protector.TryProtect(&message_packet))
    {
        UI_LOG_ERROR("Failed to encrypt audio packet");
        return;
    }

    // LedRToggle();
    net_serial.Write(message_packet);
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart, uint16_t size)
{
    // UI_LOG_ERROR("rx %u", size);
    if (huart->Instance == Serial::UART(&net_serial)->Instance)
    {
        Serial::RxISR(&net_serial, size);
    }
    else if (huart->Instance == Serial::UART(&mgmt_serial)->Instance)
    {
        Serial::RxISR(&mgmt_serial, size);
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart)
{
    if (huart->Instance == Serial::UART(&net_serial)->Instance)
    {
        Serial::TxISR(&net_serial);
        RaiseFlag(Rx_Audio_Transmitted);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart)
{
    if (huart->Instance == Serial::UART(&net_serial)->Instance)
    {
        net_serial.Stop();
        net_serial.StartReceive();
    }
}

void HAL_UART_AbortReceiveCpltCallback(UART_HandleTypeDef* huart)
{
    if (huart->Instance == Serial::UART(&net_serial)->Instance)
    {
        net_serial.StartReceive();
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
    else if (htim->Instance == TIM4)
    {
        // Turn off ptt
        HAL_TIM_Base_Stop(&htim4);
        ptt_state = Ptt_Btn_State::Last_Send;
        LedGOff();
    }
    else if (htim->Instance == TIM5)
    {
        HAL_TIM_Base_Stop(&htim5);
        ptt_ai_state = Ptt_Btn_State::Last_Send;
        LedBOff();
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
    net_serial.Stop();

    HAL_GPIO_WritePin(UI_LED_B_GPIO_Port, UI_LED_B_Pin, HIGH);
    HAL_GPIO_WritePin(UI_LED_G_GPIO_Port, UI_LED_G_Pin, HIGH);
    while (1)
    {
        HAL_GPIO_TogglePin(UI_LED_R_GPIO_Port, UI_LED_R_Pin);
        HAL_Delay(1000);
    }
}
