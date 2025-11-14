#pragma once

#include "embedded_display.hh"
#include "stm32.h"

template <size_t Width, size_t Height>
class ST7789 : public EmbeddedDisplay<Width, Height>
{
private:
    using Super = EmbeddedDisplay<Width, Height>;

    static constexpr uint8_t Software_Reset = 0x01;
    static constexpr uint8_t Sleep_Out = 0x11;
    static constexpr uint8_t Colour_Mode = 0x3A;
    static constexpr uint8_t Colour_Mode_16Bit = 0x55;
    static constexpr uint8_t MADCTL = 0x36;
    static constexpr uint8_t No_Rotation = 0x00;
    static constexpr uint8_t Display_Inversion_On = 0x21;
    static constexpr uint8_t Normal_Display_Mode = 0x13;
    static constexpr uint8_t Display_On = 0x29;

public:
    using Super::Super;

    using Super::Reset;
    using Super::SetWritableWindow;
    using Super::WriteCommand;
    using Super::WriteData;

    void Initialize() override
    {
        Reset();

        WriteCommand(Software_Reset); // Software reset
        HAL_Delay(150);

        WriteCommand(Sleep_Out); // Sleep out
        HAL_Delay(120);

        WriteCommand(Colour_Mode);    // Color mode
        WriteData(Colour_Mode_16Bit); // 16-bit color

        WriteCommand(MADCTL);   // MADCTL
        WriteData(No_Rotation); // RGB, no rotation (try 0x60 or 0xC0 for other orientations)

        WriteCommand(Display_Inversion_On); // Display inversion ON
        WriteCommand(Normal_Display_Mode);  // Normal display mode
        WriteCommand(Display_On);           // Display ON
        HAL_Delay(20);

        SetWritableWindow(0, 0, Width, Height);

        uint8_t data[2] = {0x00, 0x00};
        for (uint32_t i = 0; i < Width * Height; i++)
        {
            WriteData(data, 2);
        }

        SetBacklight(GPIO_PIN_SET);
    }

    void DrawPixel(uint16_t x, uint16_t y, uint16_t colour)
    {
        SetWritableWindow(x, y, x, y);
        uint8_t data[] = {colour >> 8, colour & 0xFF};
        WriteData(data, 2);
    }

protected:
    std::span<const uint8_t> UpdateTransmit() override
    {
    }

    std::span<const uint8_t> UpdateReceive() override
    {
    }

private:
    void SetBacklight(const GPIO_PinState set)
    {
        HAL_GPIO_WritePin(this->bl_port, this->bl_pin, set);
    }
};