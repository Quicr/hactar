#ifndef __NET_PINS__
#define __NET_PINS__

#include "driver/gpio.h"

#define NET_DEBUG_1         GPIO_NUM_7
#define NET_DEBUG_2         GPIO_NUM_15
#define NET_DEBUG_3         GPIO_NUM_16
#define UI_RX2_NET          GPIO_NUM_17
#define UI_TX2_NET          GPIO_NUM_18
#define NET_STAT            GPIO_NUM_21
#define NET_LED_B           GPIO_NUM_36
#define NET_LED_G           GPIO_NUM_37
#define NET_LED_R           GPIO_NUM_38

#define LED_MASK            1ULL << NET_LED_B | 1ULL << NET_LED_G
#define NET_STAT_MASK       1ULL << NET_STAT
#define NET_DEBUG_MASK      1ULL << NET_DEBUG_1 | 1ULL << NET_DEBUG_2 | 1ULL << NET_DEBUG_3

#define UART1 UART_NUM_1

#endif