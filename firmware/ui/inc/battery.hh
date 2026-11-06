#pragma once

#include "graphics.hh"

class Battery
{
public:
    enum class State
    {
        Critical,
        Low,
        Half_Full,
        Full,
        Charging
    };

    Battery();

    const Bitmap* GetIcon() const;
    void Charging();
    void Draining();
    void ChargeLevel(const uint8_t charge_level);
    uint8_t ChargeLevel();
    bool IsCritical();
    State GetState();

private:
    static constexpr uint8_t Critical_Level = 10;
    static constexpr uint8_t Low_Level = 30;
    static constexpr uint8_t Half_Full_Level = 75;
    static constexpr uint8_t Full_Level = 100;

    uint8_t charge;
    bool release_charging;
    State state;
};
