#pragma once

#include "stm32.h"

class Button
{
public:
    enum class Polarity
    {
        Low = 0,
        High,
    };

    static constexpr uint32_t Double_Press_Cnt = 2;

    Button(GPIO_TypeDef* port,
           const uint16_t pin,
           const Polarity polarity,
           const uint32_t debounce_ms,
           const uint32_t repeat_ms,
           const uint32_t long_press_ms,
           const uint32_t double_press_ms);
    ~Button() = default;

    void Update(const uint32_t tick_ms);

    bool IsHeld();
    bool WasReleased();
    bool ShortPress();
    bool LongPress();
    bool DoublePress();
    bool RepeatedPress();
    // Should I have a "has event" function?
    // or a no event function

private:
    enum class State
    {
        Released = 0,
        Pressed,
    };

    enum Event : uint8_t
    {
        None = 0,
        Released = 1 << 0,
        Short = 1 << 1,
        Long = 1 << 2,
        Double = 1 << 3,
        Repeat = 1 << 4,
    };

    bool ReadRaw();
    void RaiseEvent(const Event event);
    bool ConsumeEvent(const Event event);

    GPIO_TypeDef* port;
    const uint16_t pin;
    const Polarity polarity;
    const uint32_t debounce_ms;
    const uint32_t repeat_ms;
    const uint32_t long_press_ms;
    const uint32_t double_press_ms;

    bool last_raw_press;
    bool stable_press;
    uint32_t debounce_deadline_ms;
    uint32_t press_ms;
    uint32_t repeat_deadline_ms;
    uint32_t double_press_deadline_ms;
    uint8_t pending_events;
    uint8_t num_press;
};
