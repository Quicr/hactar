#pragma once

#include "display.hh"
#include "spi_device.hh"
#include <stddef.h>
#include <stdint-gcc.h>

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

    virtual void
    SetWriteablePixels(const uint16_t x0, const uint16_t y0, const uint16_t x1, const uint16_t y1)
    {
        const uint8_t col_data[] = {
            static_cast<uint8_t>(x0 >> 8),
            static_cast<uint8_t>(x0),
            static_cast<uint8_t>(x1 >> 8),
            static_cast<uint8_t>(x1),
        };

        const uint8_t row_data[] = {
            static_cast<uint8_t>(y0 >> 8),
            static_cast<uint8_t>(y0),
            static_cast<uint8_t>(y1 >> 8),
            static_cast<uint8_t>(y1),
        };

        WriteCommand(Column_Address_Set);
        WriteData(col_data, sizeof(col_data));

        WriteCommand(Row_Address_Set);
        WriteData(row_data, sizeof(row_data));

        WriteCommand(Write_RAM);
    }

protected:
    GPIO_TypeDef* dc_port;
    const uint16_t dc_pin;
    GPIO_TypeDef* rst_port;
    const uint16_t rst_pin;
    GPIO_TypeDef* bl_port;
    const uint16_t bl_pin;
};