#pragma once

#include "gfx/display.hh"
#include "gfx/graphic_memory.hh"
#include "gfx/shapes/solid_rectangle.hh"
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
    static constexpr uint16_t Num_Rows = Height / 10;

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
        bl_pin(bl_pin),
        scan_window(),
        scan_window_y0(0),
        scan_window_y1(0),
        matrix({0})
    {
    }

    virtual void Render() override
    {
        const uint16_t y0 = scan_window_y0;
        const uint16_t y1 = scan_window_y1;

        bool buff_updated = false;

        uint16_t memory_offset = 0;
        while (memory.TailHasAllocatedMemory())
        {
            Polygon::Type type = static_cast<Polygon::Type>(memory.RetrieveMemory(memory_offset));

            switch (type)
            {
            case Polygon::Type::SolidRectangle:
            {
                auto buff = memory.RetrieveMemory(memory_offset, sizeof(SolidRectangle));
                SolidRectangle* rectangle =
                    static_cast<SolidRectangle*>(static_cast<void*>(buff.data()));

                SolidRectangle::Render(rectangle, matrix, Width, y0, y1);
            }
            default:
            {
                break;
            }
            }
        }
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

    void DrawRectangle()
    {
    }

    void DrawSolidRectangle(
        uint16_t x, uint16_t y, const uint16_t width, const uint16_t height, Colour colour)
    {
        SolidRectangle::Draw(memory, x, y, width, height, colour);
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

    GraphicMemory memory;

    uint8_t scan_window[Width * 2];
    uint16_t scan_window_y0;
    uint16_t scan_window_y1;

    // Each byte stores two pixels
    uint8_t matrix[(Width * Height) / 2];
};