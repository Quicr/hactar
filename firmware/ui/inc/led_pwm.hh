#pragma once

#include "led_driver.hh"
#include "stm32.h"
#include "stm32f4xx_hal_tim.h"
#include <cstdint>

class LEDPwm : public LEDDriver
{
private:
    union Effect
    {
        struct HoldState
        {
            uint16_t state = 0;
            uint16_t prev_state = 0;
        } hold;
        struct BreatheState
        {
            int32_t state = 0;
            uint16_t low = 0;
            uint16_t high = 0;
            int32_t rate = 0;
        } breathe;
        struct PulseState
        {
            uint16_t low = 0;
            uint16_t high = 0;
            uint16_t pattern_mask = 0;
            uint16_t num_patterns = 0;
            uint16_t timeout_ms = 0;
            uint16_t last_update_ms = 0;
            uint16_t bit = 0;
        } pulse;
    };

    static constexpr uint16_t Num_Pattern_Bits =
        ((sizeof(Effect::PulseState::pattern_mask) * 8) - 1);

public:
    enum class Mode
    {
        Hold,
        Pulse,
        Breathe
    };

    LEDPwm(TIM_HandleTypeDef& htim, const uint32_t channel, const Polarity polarity);
    ~LEDPwm() = default;

    void Start();
    void Stop();

    // Note- Sets the mode to Hold
    void Set(const uint16_t value) override;
    void Toggle() override;
    void Pulse(const uint16_t low,
               const uint16_t high,
               const uint16_t pattern_mask,
               const uint16_t num_patterns,
               const uint16_t timeout_ms);
    void Breathe(const uint16_t low, const uint16_t high, const int32_t rate);

    void Update(const uint32_t ticks);

private:
    void Update();
    void SetPwm(const uint16_t pwm);
    TIM_HandleTypeDef& timer;
    const uint32_t channel;
    Effect effect;
    Mode mode;
};
