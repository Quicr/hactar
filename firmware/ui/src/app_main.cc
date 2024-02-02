#include "main.h"
#include "app_main.hh"

#include "String.hh"
#include "Font.hh"
#include "PortPin.hh"
#include "PushReleaseButton.hh"

#include "Screen.hh"
#include "Q10Keyboard.hh"
#include "EEPROM.hh"
#include "UserInterfaceManager.hh"

#include "SerialStm.hh"
#include "Led.hh"
#include "AudioCodec.hh"

#include <memory>

// Handlers
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

extern SPI_HandleTypeDef hspi1;
extern I2C_HandleTypeDef hi2c1;
extern I2S_HandleTypeDef hi2s3;
extern TIM_HandleTypeDef htim2;
extern RNG_HandleTypeDef hrng;


port_pin cs = { DISP_CS_GPIO_Port, DISP_CS_Pin };
port_pin dc = { DISP_DC_GPIO_Port, DISP_DC_Pin };
port_pin rst = { DISP_RST_GPIO_Port, DISP_RST_Pin };
port_pin bl = { DISP_BL_GPIO_Port, DISP_BL_Pin };

Screen screen(hspi1, cs, dc, rst, bl, Screen::Orientation::left_landscape);
Q10Keyboard* keyboard = nullptr;
SerialStm* mgmt_serial_interface = nullptr;
SerialStm* net_serial_interface = nullptr;
UserInterfaceManager* ui_manager = nullptr;
EEPROM* eeprom = nullptr;
AudioCodec* audio = nullptr;
bool rx_busy = false;


// TODO Get the osc working correctly from an external signal
int app_main()
{
    audio = new AudioCodec(hi2s3, hi2c1);

    // Reserve the first 32 bytes, and the total size is 255 bytes - 1k bits
    eeprom = new EEPROM(hi2c1, 32, 255);

    screen.Begin();

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
    net_serial_interface = new SerialStm(&huart2);

    ui_manager = new UserInterfaceManager(screen, *keyboard, *net_serial_interface, *eeprom);

    SerialPacketManager mgmt_serial(mgmt_serial_interface);

    uint8_t start_message [] = "UI: start\n\r";
    HAL_UART_Transmit(&huart1, start_message, 12, 1000);
    HAL_GPIO_WritePin(UI_LED_R_GPIO_Port, UI_LED_R_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(UI_LED_G_GPIO_Port, UI_LED_G_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(UI_LED_B_GPIO_Port, UI_LED_B_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(ADC_UI_STAT_GPIO_Port, ADC_UI_STAT_Pin, GPIO_PIN_SET);

    uint16_t RX_SOUND_BUFFER_SZ = 256;
    uint16_t rx_buffer[RX_SOUND_BUFFER_SZ] = { 0 };
    rx_buffer[0] = 1;
    rx_buffer[1] = 2000;
    uint16_t tx_buffer[RX_SOUND_BUFFER_SZ] = { 0 };

    // HAL_GPIO_TogglePin(UI_LED_B_GPIO_Port, UI_LED_B_Pin);

    // TODO
    // Generate a simple triangle wave
    // for (uint16_t i = 0; i < BUFFER_SIZE; ++i)
    // {
    //     // buff[i] = (uint16_t)((double(i) / BUFFER_SIZE) * 65535);
    //     buff[i] = 1;
    // }
    // Sanity check numbers
    // buff[0] = 1;
    // buff[1] = 0xF0F0;
    // buff[2] = 65535;
    // buff[4] = 0xAAAA;
    // buff[5] = 0x5555;
    // buff[BUFFER_SIZE - 1] = 1;
    // buff[BUFFER_SIZE - 2] = 0x00AA;

    // Delayed condition
    uint32_t blink = HAL_GetTick() + 5000;
    while (1)
    {
        ui_manager->Run();

        // HAL_DAC_SetValue()
        // // mgmt_serial.RxTx(HAL_GetTick());

        // // screen.FillRectangle(0, 200, 20, 220, C_YELLOW);
        if (HAL_GetTick() > blink)
        {

            //     // auto packet = std::make_unique<SerialPacket>();
            //     // packet->SetData(SerialPacket::Types::Debug, 0, 1);
            //     // packet->SetData(mgmt_serial.NextPacketId(), 1, 2);
            //     // packet->SetData(5, 1, 2);
            //     // packet->SetData('h', 1);
            //     // packet->SetData('e', 1);
            //     // packet->SetData('l', 1);
            //     // packet->SetData('l', 1);
            //     // packet->SetData('o', 1);
            //     // mgmt_serial.EnqueuePacket(std::move(packet));
            // audio->Send1KHzSignal();

            // HAL_StatusTypeDef res = HAL_I2S_Transmit_DMA(&hi2s3, buff, BUFFER_SIZE * sizeof(uint16_t));
            // screen.DrawText(0, 100, String::int_to_string((int)res), font7x12, C_WHITE, C_BLACK);
            // if (res == HAL_OK)
            // {
            // }
            blink = HAL_GetTick() + 5000;
            // HAL_GPIO_TogglePin(UI_LED_R_GPIO_Port, UI_LED_R_Pin);
            //     uint8_t test_message [] = "UI: Test\n\r";
            //     HAL_UART_Transmit(&huart1, test_message, 10, 1000);

            if (rx_busy)
                continue;
            // We need this because if the mic stays on it will write to USART3 in
            // bootloader mode which locks up the main chip
            // TODO remove enable mic bits [2,3]
            audio->XorRegister(0x19, 0b0'0000'1100);

            rx_busy = true;

            uint32_t num = 0;
            HAL_RNG_GenerateRandomNumber(&hrng, &num);
            screen.DrawText(0, 82, String::int_to_string(num), font7x12, C_GREEN, C_BLACK);

            // TODO Rx_Buffer_Sz might need to be in bytes not element sz.
            // auto output = HAL_I2S_(&hi2s3, rx_buffer, RX_SOUND_BUFFER_SZ, HAL_MAX_DELAY);
            auto output = HAL_I2SEx_TransmitReceive_DMA(&hi2s3, tx_buffer, rx_buffer, RX_SOUND_BUFFER_SZ);
            screen.DrawText(0, 100, String::int_to_string((int)output), font7x12, C_GREEN, C_BLACK);
            screen.DrawText(0, 112, String::int_to_string((int)rx_buffer[0]), font7x12, C_GREEN, C_BLACK);
            screen.DrawText(0, 124, String::int_to_string((int)rx_buffer[1]), font7x12, C_GREEN, C_BLACK);
            screen.DrawText(0, 136, String::int_to_string((int)rx_buffer[2]), font7x12, C_GREEN, C_BLACK);
            screen.DrawText(0, 148, String::int_to_string((int)rx_buffer[3]), font7x12, C_GREEN, C_BLACK);

        }
    }

    return 0;
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
    if (huart->Instance == USART2)
    {
        net_serial_interface->TxEvent();
    }
    else if (huart->Instance == USART1)
    {
        HAL_GPIO_TogglePin(UI_LED_G_GPIO_Port, UI_LED_G_Pin);
        mgmt_serial_interface->TxEvent();
    }
}

void HAL_I2SEx_TxRxCpltCallback(I2S_HandleTypeDef* hi2s)
{
    audio->RxComplete();
    // rx_busy = false;
    // audio->XorRegister(0x19, 0b0'0000'1100);

    HAL_GPIO_TogglePin(UI_LED_G_GPIO_Port, UI_LED_G_Pin);
}

void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef* hi2s)
{

    HAL_GPIO_TogglePin(UI_LED_G_GPIO_Port, UI_LED_G_Pin);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart)
{
    uint16_t err;
    if (huart->Instance == USART2)
    {
        net_serial_interface->Reset();
        HAL_GPIO_TogglePin(UI_LED_B_GPIO_Port, UI_LED_B_Pin);

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

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef* hspi)
{
    UNUSED(hspi);
    screen.ReleaseSPI();
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim)
{
    // Keyboard timer callback!
    if (htim->Instance == TIM2)
    {
        keyboard->Read();
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
        ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
        /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
