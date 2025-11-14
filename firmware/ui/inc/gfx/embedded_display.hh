#pragma once

#include "display.hh"
#include "shapes/shape.hh"
#include "spi_device.hh"
#include <stddef.h>
#include <stdint-gcc.h>

template <size_t Width, size_t Height>
class EmbeddedDisplay : public Display, public SPIDevice
{
private:
    static constexpr uint8_t Column_Address_Set = 0x2A;
    static constexpr uint8_t Row_Address_Set = 0x2B;
    static constexpr uint8_t Write_RAM = 0x2C;

public:
    EmbeddedDisplay(SPI_HandleTypeDef* hspi,
                    GPIO_TypeDef* cs_port,
                    const uint16_t cs_pin,
                    GPIO_TypeDef* dc_port,
                    const uint16_t dc_pin,
                    GPIO_TypeDef* rst_port,
                    const uint16_t rst_pin,
                    GPIO_TypeDef* bl_port,
                    const uint16_t bl_pin) :
        SPIDevice(hspi, cs_port, cs_pin),
        dc_port(dc_port),
        dc_pin(dc_pin),
        rst_port(rst_port),
        rst_pin(rst_pin),
        bl_port(bl_port),
        bl_pin(bl_pin)
    {
    }

    virtual void Render() override
    {
    }
    virtual void Update() override
    {
    }

    virtual void Reset() override
    {
        HAL_GPIO_WritePin(rst_port, rst_pin, GPIO_PIN_RESET);
        HAL_Delay(50);
        HAL_GPIO_WritePin(rst_port, rst_pin, GPIO_PIN_SET);
        HAL_Delay(10);
    }

    virtual void PushShape(Shape shape) override
    {
    }

    virtual void WriteCommand(const uint8_t cmd)
    {
        // DC low for commands
        HAL_GPIO_WritePin(dc_port, dc_pin, GPIO_PIN_RESET);
        Transmit(&cmd, 1);
    }

    virtual void WriteData(const uint8_t data)
    {
        HAL_GPIO_WritePin(dc_port, dc_pin, GPIO_PIN_SET);
        Transmit(&data, 1);
    }

    virtual void WriteData(const uint8_t* data, const size_t len)
    {
        HAL_GPIO_WritePin(dc_port, dc_pin, GPIO_PIN_SET);
        Transmit(data, len);
    }

protected:
    virtual void SetWritableWindow(const uint16_t x,
                                   const uint16_t y,
                                   const uint16_t width,
                                   const uint16_t height)
    {
        const uint8_t col_data[] = {
            static_cast<uint8_t>(x >> 8),
            static_cast<uint8_t>(x),
            static_cast<uint8_t>((width - 1) >> 8),
            static_cast<uint8_t>((width - 1)),
        };

        const uint8_t row_data[] = {
            static_cast<uint8_t>(y >> 8),
            static_cast<uint8_t>(y),
            static_cast<uint8_t>((height - 1) >> 8),
            static_cast<uint8_t>((height - 1)),
        };

        WriteCommand(Column_Address_Set);
        WriteData(col_data, sizeof(col_data));

        WriteCommand(Row_Address_Set);
        WriteData(row_data, sizeof(row_data));

        WriteCommand(Write_RAM);
    }

    // Variables
    GPIO_TypeDef* dc_port;
    const uint16_t dc_pin;
    GPIO_TypeDef* rst_port;
    const uint16_t rst_pin;
    GPIO_TypeDef* bl_port;
    const uint16_t bl_pin;

    // Each byte stores two pixels
    uint8_t matrix[(Width * Height) / 2];
};