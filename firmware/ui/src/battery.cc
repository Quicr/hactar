#include "battery.hh"
#include "battery_icon.hh"

Battery::Battery() :
    charge(0),
    release_charging(false),
    state(State::Full)
{
}

const Bitmap* Battery::GetIcon() const
{
    switch (state)
    {
    case State::Critical:
        return &Battery_Critical_20x18::bitmap;
    case State::Low:
        return &Battery_Low_20x18::bitmap;
    case State::Half_Full:
        return &Battery_Half_20x18::bitmap;
    case State::Full:
        return &Battery_Full_20x18::bitmap;
    case State::Charging:
        return &Battery_Charging_20x18::bitmap;
    default:
        return &Battery_Critical_20x18::bitmap;
    }
}

void Battery::Charging()
{
    release_charging = false;
    state = State::Charging;
}

void Battery::Draining()
{
    release_charging = true;
}

void Battery::ChargeLevel(const uint8_t charge_level)
{
    // We don't want to modify the charge levels if we are charging
    // because with ev19 it will always be 100% if it is plugged in.
    if (state == State::Charging && !release_charging)
    {
        return;
    }

    charge = charge_level;

    if (charge < 10)
    {
        state = State::Critical;
    }
    else if (charge < 30)
    {
        state = State::Low;
    }
    else if (charge < 75)
    {
        state = State::Half_Full;
    }
    else
    {
        state = State::Full;
    }
}

uint8_t Battery::ChargeLevel()
{
    return charge;
}

bool Battery::IsCritical()
{
    return charge < Critical_Level;
}

Battery::State Battery::GetState()
{
    return state;
}
