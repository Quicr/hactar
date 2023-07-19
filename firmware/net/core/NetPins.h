#ifndef __NET_PINS__
#define __NET_PINS__

#include "driver/gpio.h"

#define LED_R_Pin           GPIO_NUM_5
#define LED_G_Pin           GPIO_NUM_6
#define LED_B_Pin           GPIO_NUM_7
#define NET_STAT_Pin        GPIO_NUM_21
#define NET_DBG5_Pin        GPIO_NUM_35
#define NET_DBG6_Pin        GPIO_NUM_36

#define LEDS_OUTPUT_SEL     1ULL << LED_B_Pin | 1ULL << LED_G_Pin | 1ULL << LED_R_Pin
#define NET_STAT_SEL        1ULL << NET_STAT_Pin
#define NET_DBG_SEL         1ULL << NET_DBG5_Pin | 1ULL << NET_DBG6_Pin

#define UART1 UART_NUM_1

#endif