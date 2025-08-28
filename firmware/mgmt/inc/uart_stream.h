// #ifndef UART_STREAM_H
// #define UART_STREAM_H

// #include "state.h"
// #include "stm32f0xx_hal.h"
// #include <string.h>

// #define COMMAND_TIMEOUT 1000
// #define COMMAND_BUFF_SZ 32
// #define CONFIGURATOR_BUFF_SZ 128

// typedef enum _
// {
//     Ignore,
//     Passthrough,
//     Command,
//     Configuration
// } StreamMode;

// typedef enum
// {
//     None = 0,
//     Usb,
//     Ui,
//     Net,
//     Ui_Net,
//     Cmd,
// } Buffer_Direction;

// typedef struct
// {
//     UART_HandleTypeDef* uart;
//     uint8_t* buff;
//     const uint16_t size;
//     uint16_t idx;
// } receive_t;

// typedef struct
// {
//     UART_HandleTypeDef* uart;
//     uint8_t* buff;
//     const uint16_t size;
//     uint16_t read;
//     uint16_t write;
//     uint16_t unsent;
//     uint16_t num_sending;
//     uint8_t free;
// } transmit_t;

// typedef struct
// {
//     receive_t* rx;
//     transmit_t* tx;
//     StreamMode mode;
//     Buffer_Direction direction;
// } uart_stream_t;

// // Commands
// static const uint8_t ui_upload_cmd[] = "ui_upload";
// static const uint8_t net_upload_cmd[] = "net_upload";
// static const uint8_t debug_cmd[] = "debug";
// static const uint8_t ui_debug_cmd[] = "ui_debug";
// static const uint8_t net_debug_cmd[] = "net_debug";
// static const uint8_t reset_cmd[] = "reset";
// static const uint8_t reset_ui[] = "reset_ui";
// static const uint8_t reset_net[] = "reset_net";
// static const uint8_t configure[] = "configure";

// // Configure commands
// static const uint8_t quit_config[] = "quit";
// static const uint8_t set_ssid_0[] = "set_ssid_0";
// static const uint8_t set_pwd_0[] = "set_pwd_0";
// static const uint8_t set_ssid_1[] = "set_ssid_1";
// static const uint8_t set_pwd_1[] = "set_pwd_1";
// static const uint8_t set_ssid_2[] = "set_ssid_2";
// static const uint8_t set_pwd_2[] = "set_pwd_2";
// static const uint8_t set_moq_url[] = "set_moq_url";
// static const uint8_t set_sframe_key[] = "set_sframe_key";
// static const uint8_t get_ssid_0[] = "get_ssid_0";
// static const uint8_t get_ssid_1[] = "get_ssid_1";
// static const uint8_t get_ssid_2[] = "get_ssid_2";
// static const uint8_t get_moq_url[] = "get_moq_url";
// static const uint8_t clear_configuration[] = "clear_configuration";

// static const uint8_t ACK[] = {0x79};
// static const uint8_t READY[] = {0x80};
// static const uint8_t NACK[] = {0x1F};
// static const uint8_t HELLO[] = "WHO ARE YOU?";
// static const uint8_t HELLO_RES[] = "HELLO, I AM A HACTAR DEVICE";

// void ReceiveISR(uart_stream_t* stream, uint16_t num_received);
// uint8_t TryCommand(const char* buff, enum State* state, uart_stream_t* stream);
// void HandleCommands(uart_stream_t* stream, enum State* state);
// void HandleConfiguration(uart_stream_t* stream, enum State* state);
// void TxISR(transmit_t* stream, enum State* state);
// void Transmit(transmit_t* stream, enum State* state);
// void HandleTx(uart_stream_t* stream, enum State* state);
// void InitUartStream(uart_stream_t* stream);
// void StartUartReceive(uart_stream_t* stream);
// void SetStreamModes(const StreamMode usb_mode, const StreamMode ui_mode, const StreamMode
// net_mode); void RestartUartStream(uart_stream_t* stream);

// #endif