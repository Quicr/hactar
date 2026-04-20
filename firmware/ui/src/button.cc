#include "button.hh"

Button::Button(GPIO_TypeDef* port,
               const uint16_t pin,
               const Polarity polarity,
               const uint32_t debounce_ms,
               const uint32_t repeat_ms) :
    port(port),
    pin(pin),
    polarity(polarity),
    debounce_ms(debounce_ms),
    repeat_ms(repeat_ms),
    last_raw_press(false),
    stable_press(false),
    debounce_deadline_ms(0),
    press_ms(0),
    pending_events(0),
    num_press(0),
    double_press_deadline_ms(0)
{
}

void Button::Update(const uint32_t tick_ms)
{
    const bool raw_press = ReadRaw();

    if (raw_press != last_raw_press)
    {
        last_raw_press = raw_press;
        debounce_deadline_ms = tick_ms;
        return;
    }

    if (tick_ms - debounce_deadline_ms < debounce_ms)
    {
        return;
    }

    if (stable_press != raw_press)
    {
        stable_press = raw_press;

        if (stable_press)
        {
            press_ms = tick_ms;
        }
        else
        {
            // Released.
            if (press_ms >= Long_Press_Timeout_ms)
            {
                RaiseEvent(Event::Long);
                return;
            }
            else
            {
                double_press_deadline_ms = tick_ms;
                num_press++;
            }

            press_ms = 0;
        }
    }

    if (num_press >= Double_Press_Cnt
        && tick_ms - double_press_deadline_ms <= Double_Press_Timeout_ms)
    {
        num_press = 0;
        double_press_deadline_ms = 0;
        RaiseEvent(Event::Double);
    }
    else if (num_press && tick_ms - double_press_deadline_ms > Double_Press_Timeout_ms)
    {
        num_press = 0;
        double_press_deadline_ms = 0;
        RaiseEvent(Event::Short);
    }

    if (stable_press && stable_press == raw_press && tick_ms - press_ms >= repeat_ms)
    {
        // If repeating,
        num_press = 0;
        press_ms = tick_ms;
        RaiseEvent(Event::Short);
    }
}

bool Button::IsHeld()
{
    return stable_press;
}

bool Button::ShortPress()
{
    return ConsumeEvent(Event::Short);
}

bool Button::LongPress()
{
    return ConsumeEvent(Event::Long);
}

bool Button::DoublePress()
{
    return ConsumeEvent(Event::Double);
}

bool Button::ReadRaw()
{
    return HAL_GPIO_ReadPin(port, pin) == static_cast<GPIO_PinState>(polarity);
}

void Button::RaiseEvent(const Event event)
{
    pending_events |= event;
}

bool Button::ConsumeEvent(const Event event)
{
    const bool ret = (pending_events & event) > 0;
    pending_events &= ~event;
    return ret;
}
