#pragma once

#include <cstdint>
#include <limits>

class LEDDriver
{
public:
    enum class Polarity
    {
        Low,
        High
    };

    LEDDriver(const Polarity polarity) :
        polarity(polarity)
    {
    }

    virtual void Set(const uint16_t value) = 0;

    void On()
    {
        Set(std::numeric_limits<uint16_t>().max());
    }

    void Off()
    {
        Set(std::numeric_limits<uint16_t>().min());
    }

protected:
    const Polarity polarity;
};
