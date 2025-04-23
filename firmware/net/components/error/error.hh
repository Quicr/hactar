#pragma once

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// #include "../../core/inc/net_pins.hh"

static void ErrorState(const char* caller,
                       const unsigned long LED_R_State,
                       const unsigned long LED_G_State,
                       const unsigned long LED_B_State)
{
    // gpio_set_level(NET_LED_R, LED_R_State);
    // gpio_set_level(NET_LED_G, LED_G_State);
    // gpio_set_level(NET_LED_B, LED_B_State);

    // while (true)
    // {
    //     printf("An error has occured -- ");
    //     printf(caller);
    //     printf("\n");

    //     vTaskDelay(1000 / portTICK_PERIOD_MS);
    // }
}
