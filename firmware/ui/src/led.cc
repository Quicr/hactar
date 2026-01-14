#include "led.hh"

LED::LED(GPIO_TypeDef* port, int pin, LED_Polarity polarity) :
    port(port),
    pin(pin),
    polarity(polarity)
{
}

void LED::On()
{
}

void LED::Off()
{
}

void LED::Breathe()
{
}

void LED::Flash()
{
}

void LED::Set(int pwm_value)
{
}

void Callback(LED& led)
{
}
