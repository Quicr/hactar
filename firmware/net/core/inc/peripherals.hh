#ifndef PERIPHERALS_HH
#define PERIPHERALS_HH

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/uart.h"
#include "uart.hh"
#include <memory>

// Pin and peripheral definitions
#define NET_DEBUG_1 GPIO_NUM_5
#define NET_DEBUG_2 GPIO_NUM_6
#define NET_DEBUG_3 GPIO_NUM_7
#define NET_READY GPIO_NUM_8
#define UI_READY GPIO_NUM_9
#define SPI_MOSI GPIO_NUM_11
#define SPI_CLK GPIO_NUM_12
#define SPI_MISO GPIO_NUM_13
#define NET_RTS1_MGMT GPIO_NUM_15
#define NET_CTS1_MGMT GPIO_NUM_16
#define UI_RX2_NET GPIO_NUM_17
#define UI_TX2_NET GPIO_NUM_18
#define NET_STAT GPIO_NUM_21
#define NET_LED_B GPIO_NUM_36
#define NET_LED_G GPIO_NUM_37
#define NET_LED_R GPIO_NUM_38

#define LED_MASK 1ULL << NET_LED_B | 1ULL << NET_LED_G | 1ULL << NET_LED_R
#define NET_STAT_MASK 1ULL << NET_STAT
#define NET_DEBUG_MASK 1ULL << NET_DEBUG_1 | 1ULL << NET_DEBUG_2 | 1ULL << NET_DEBUG_3

#define UI_READY_MASK 1ULL << UI_READY

void InitializeGPIO();
void IntitializeLEDs();
void IntitializePWM();
void InitializeUIReadyISR(gpio_isr_t isr_handler);

#endif