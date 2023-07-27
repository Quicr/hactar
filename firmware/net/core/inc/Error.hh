#ifndef __ERROR_NET__
#define __ERROR_NET__

#include "freertos/task.h"
#include "NetPins.hh"

static void ErrorState(const char* caller,
                       const unsigned long LED_R_State,
                       const unsigned long LED_G_State,
                       const unsigned long LED_B_State)
{
    gpio_set_level(LED_R_Pin, LED_R_State);
    gpio_set_level(LED_G_Pin, LED_G_State);
    gpio_set_level(LED_B_Pin, LED_B_State);

    while (true)
    {
        printf("An error has occured -- ");
        printf(caller);
        printf("\n");

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

#endif