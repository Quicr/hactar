#pragma once

#include "display.hh"
#include "stm32.h"

template <size_t Length, size_t Width>
class ST7789 : public Display
{
public:
    ST7789(SPI_HandleTypeDef& hspi,
           GPIO_TypeDef* cs_port,
           const uint16_t cs_pin,
           GPIO_TypeDef* dc_port,
           const uint16_t dc_pin,
           GPIO_TypeDef* rst_port,
           const uint16_t rst_pin,
           GPIO_TypeDef* bl_port,
           const uint16_t bl_pin) :
        spi(&hspi),
        cs_port(cs_port),
        cs_pin(cs_pin),
        dc_port(dc_port),
        dc_pin(dc_pin),
        rst_port(rst_port),
        rst_pin(rst_pin),
        bl_port(bl_port),
        bl_pin(bl_pin)
    {
    }

    void Initialize()
    {
        Select();
        Reset();

        WriteCommand(0x01); // Software reset
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

        SetDrawingWindow(0, 0, 239, 239);
        WriteCommand(0x2C); // RAMWR

        uint8_t data[2] = {0x00, 0x00};
        for (uint32_t i = 0; i < 240UL * 240UL; i++)
        {
            WriteData(data, 2);
        }

        SetBacklight(GPIO_PIN_SET);
    }

    void Reset()
    {
        HAL_GPIO_WritePin(rst_port, rst_pin, GPIO_PIN_RESET);
        HAL_Delay(50);
        HAL_GPIO_WritePin(rst_port, rst_pin, GPIO_PIN_SET);
        HAL_Delay(10);
    }

    void WriteCommand(const uint8_t cmd)
    {
        // DC low for commands
        HAL_GPIO_WritePin(dc_port, dc_pin, GPIO_PIN_RESET);
        Transmit(&cmd, 1);
    }

    void WriteData(const uint8_t data)
    {
        HAL_GPIO_WritePin(dc_port, dc_pin, GPIO_PIN_SET);
        Transmit(&data, 1);
    }

    void WriteData(const uint8_t* data, const size_t len)
    {
        HAL_GPIO_WritePin(dc_port, dc_pin, GPIO_PIN_SET);
        Transmit(data, len);
    }

    void Select()
    {
        HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_RESET);
    }

    void Deselect()
    {
        HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_SET);
    }
    void DrawPixel(uint16_t x, uint16_t y, uint16_t color)
    {
        SetDrawingWindow(x, y, x, y);
        uint8_t data[] = {color >> 8, color & 0xFF};
        WriteData(data, 2);
    }

private:
    void Transmit(const uint8_t* data, const size_t len)
    {
        Select();
        HAL_SPI_Transmit(spi, data, len, HAL_MAX_DELAY);
        Deselect();
    }

    void SetBacklight(const GPIO_PinState set)
    {
        HAL_GPIO_WritePin(bl_port, bl_pin, set);
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

    SPI_HandleTypeDef* spi;

    GPIO_TypeDef* cs_port;
    const uint16_t cs_pin;
    GPIO_TypeDef* dc_port;
    const uint16_t dc_pin;
    GPIO_TypeDef* rst_port;
    const uint16_t rst_pin;
    GPIO_TypeDef* bl_port;
    const uint16_t bl_pin;
};