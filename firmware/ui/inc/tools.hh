#pragma once

// #include "app_main.hh"
#include "audio_chip.hh"
#include "logger.hh"
#include "stm32.h"

// Needs to be ran AFTER the audio chip has been started
// as it is used to count the number of interrupts that the audio
// chip generates over 1 second.
// Generally we want to see 50 iteration for 20ms of audio.
static uint32_t CountNumAudioInterrupts(const AudioChip& audio, volatile bool& sleeping)
{
    if (!audio.ReadFlag(AudioChip::AudioFlag::Running))
    {
        Error("CountNumAudioInterrupts", "Audio chip is not running");
    }

    constexpr uint32_t duration_ms = 1000;
    constexpr uint32_t expected_interrupt_count = duration_ms / constants::Audio_Time_Length_ms;
    uint32_t actual_interrupt_count = 0;
    uint32_t interrupt_count_timeout_ms = 0;

    sleeping = true;
    // Wait until the first interrupt occurs.
    while (sleeping)
    {
        __NOP();
    }

    interrupt_count_timeout_ms = HAL_GetTick() + duration_ms;
    while (true)
    {
        while (sleeping)
        {
            __NOP();
        }

        ++actual_interrupt_count;
        if (HAL_GetTick() >= interrupt_count_timeout_ms)
        {
            UI_LOG_ERROR("Audio interrupts - expected: %lu, actual: %lu", expected_interrupt_count,
                         actual_interrupt_count);

            if (actual_interrupt_count != (1000 / constants::Audio_Time_Length_ms))
            {
                // Note, will never leave this function
                Error("CountNumAudioInterrupts", "The expected number of interrupts was wrong.");
            }

            UI_LOG_INFO("Passed audio interrupt count test");
            break;
        }

        sleeping = true;
    }

    return actual_interrupt_count;
}

static void AlivePulse(GPIO_TypeDef* port, const uint16_t pin)
{
    static uint32_t blinky = 0;
    if (HAL_GetTick() > blinky)
    {
        HAL_GPIO_TogglePin(port, pin);
        blinky = HAL_GetTick() + 2000;
    }
}