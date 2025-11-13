#pragma once

#include "embedded_display.hh"
#include "stm32.h"

template <size_t Length, size_t Width>
class ST7789 : public EmbeddedDisplay
{
private:
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
    ST7789(SPI_HandleTypeDef* hspi,
           GPIO_TypeDef* cs_port,
           const uint16_t cs_pin,
           GPIO_TypeDef* dc_port,
           const uint16_t dc_pin,
           GPIO_TypeDef* rst_port,
           const uint16_t rst_pin,
           GPIO_TypeDef* bl_port,
           const uint16_t bl_pin) :
        EmbeddedDisplay(hspi, cs_port, cs_pin, dc_port, dc_pin, rst_port, rst_pin, bl_port, bl_pin)
    {
    }

    void Initialize() override
    {
        Select();
        Reset();

        WriteCommand(0x01U); // Software reset
        HAL_Delay(150);

        WriteCommand(0x11); // Sleep out
        HAL_Delay(120);

        WriteCommand(0x3A); // Color mode
        WriteData(0x55);    // 16-bit color

        WriteCommand(0x36); // MADCTL
        WriteData(0x00);    // RGB, no rotation (try 0x60 or 0xC0 for other orientations)

        WriteCommand(0x21); // Display inversion ON
        WriteCommand(0x13); // Normal display mode
        WriteCommand(0x29); // Display ON
        HAL_Delay(20);

        SetWriteablePixels(0, 0, 239, 239);

        uint8_t data[2] = {0x00, 0xFF};
        for (uint32_t i = 0; i < 240 * 240; i++)
        {
            WriteData(data, 2);
        }

        SetBacklight(GPIO_PIN_SET);
    }
    void
    SetDrawingWindow(const uint16_t x0, const uint16_t y0, const uint16_t x1, const uint16_t y1)
    {
        WriteCommand(0x2A); // Column address set
        uint8_t data_x[] = {x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF};
        WriteData(data_x, 4);

        WriteCommand(0x2B); // Row address set
        uint8_t data_y[] = {y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF};
        WriteData(data_y, 4);

        WriteCommand(0x2C); // Write to RAM
    }

    void Reset() override
    {
        HAL_GPIO_WritePin(rst_port, rst_pin, GPIO_PIN_RESET);
        HAL_Delay(50);
        HAL_GPIO_WritePin(rst_port, rst_pin, GPIO_PIN_SET);
        HAL_Delay(10);
    }

    void DrawPixel(uint16_t x, uint16_t y, uint16_t colour)
    {
        SetDrawingWindow(x, y, x, y);
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
        HAL_GPIO_WritePin(bl_port, bl_pin, set);
    }
};