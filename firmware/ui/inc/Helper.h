#ifndef __HELPER__
#define __HELPER__

#include "stm32.h"

static void EnablePortIf(GPIO_TypeDef *port)
{
    if (port == GPIOA && !(RCC->AHB1ENR & RCC_AHB1ENR_GPIOAEN))
    {
        __HAL_RCC_GPIOA_CLK_ENABLE();
    }

    if (port == GPIOB && !(RCC->AHB1ENR & RCC_AHB1ENR_GPIOBEN))
    {
        __HAL_RCC_GPIOB_CLK_ENABLE();
    }

    if (port == GPIOC && !(RCC->AHB1ENR & RCC_AHB1ENR_GPIOCEN))
    {
        __HAL_RCC_GPIOC_CLK_ENABLE();
    }
}

#endif