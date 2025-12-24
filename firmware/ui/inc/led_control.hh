#pragma once

#include "main.h"

constexpr GPIO_PinState HIGH = GPIO_PIN_SET;
constexpr GPIO_PinState LOW = GPIO_PIN_RESET;

extern "C" {
void LedR(GPIO_PinState state);
void LedG(GPIO_PinState state);
void LedB(GPIO_PinState state);
void Leds(GPIO_PinState r, GPIO_PinState g, GPIO_PinState b);
void LedROn();
void LedROff();
void LedRToggle();
void LedBOn();
void LedBOff();
void LedBToggle();
void LedGOn();
void LedGOff();
void LedGToggle();
void LedsOn();
void LedsOff();
void LedsToggle();
}
