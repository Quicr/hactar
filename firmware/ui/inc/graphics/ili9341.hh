#pragma once

#include "stm32.h"
#include <cstddef>

class ILI9341
{
public:
    enum class Orientation : uint8_t
    {
        Portrait,
        Landscape,
        Portrait_Inverted,
        Landscape_Inverted,
    };

    ILI9341(uint8_t* buff,
            const uint16_t width,
            const uint16_t height,
            SPI_HandleTypeDef& spi,
            GPIO_TypeDef* cs_port,
            const uint16_t cs_pin,
            GPIO_TypeDef* dc_port,
            const uint16_t dc_pin,
            GPIO_TypeDef* reset_port,
            const uint16_t reset_pin,
            GPIO_TypeDef* bl_port,
            const uint16_t bl_pin,
            const Orientation orientation);

    void Init();
    void Reset();
    void EnableBacklight();
    void DisableBacklight();
    void Rotate(Orientation orientation);
    void SetRenderRegion(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end);
    bool Render(const uint8_t* data, size_t size);
    bool IsSpiBusy() const;
    void OnSpiTransmitComplete(SPI_HandleTypeDef* completed_spi);
    void Sleep();
    void Wake();

    uint16_t GetWidth() const;
    uint16_t GetHeight() const;

private:
    static constexpr uint8_t Software_Reset = 0x01U;
    static constexpr uint8_t Sleep_Out = 0x11U;
    static constexpr uint8_t Display_On = 0x29U;
    static constexpr uint8_t Display_Off = 0x28U;
    static constexpr uint8_t Sleep_In = 0x10U;
    static constexpr uint8_t Column_Address_Set = 0x2AU;
    static constexpr uint8_t Page_Address_Set = 0x2BU;
    static constexpr uint8_t Memory_Write = 0x2CU;
    static constexpr uint8_t Memory_Access_Control = 0x36U;
    static constexpr uint8_t Pixel_Format_Set = 0x3AU;
    static constexpr uint8_t Power_Control_A = 0xCBU;
    static constexpr uint8_t Power_Control_B = 0xCFU;
    static constexpr uint8_t Driver_Timing_Control_A = 0xE8U;
    static constexpr uint8_t Driver_Timing_Control_B = 0xEAU;
    static constexpr uint8_t Power_On_Sequence_Control = 0xEDU;
    static constexpr uint8_t Pump_Ratio_Control = 0xF7U;
    static constexpr uint8_t Power_Control_1 = 0xC0U;
    static constexpr uint8_t Power_Control_2 = 0xC1U;
    static constexpr uint8_t VCOM_Control_1 = 0xC5U;
    static constexpr uint8_t VCOM_Control_2 = 0xC7U;
    static constexpr uint8_t Frame_Rate_Control = 0xB1U;
    static constexpr uint8_t Display_Function_Control = 0xB6U;
    static constexpr uint8_t Gamma_Function_Disable = 0xF2U;
    static constexpr uint8_t Gamma_Curve_Select = 0x26U;
    static constexpr uint8_t Positive_Gamma_Correction = 0xE0U;
    static constexpr uint8_t Negative_Gamma_Correction = 0xE1U;
    static constexpr uint8_t Normal_Display_Mode_On = 0x13U;

    static constexpr uint8_t BGR_Order = 0x08U;
    static constexpr uint8_t Mirror_X = 0x40U;
    static constexpr uint8_t Mirror_Y = 0x80U;
    static constexpr uint8_t Swap_XY = 0x20U;
    static constexpr uint32_t Spi_Timeout = 1000U;

    void Select();
    void Deselect();
    void WriteCommand(uint8_t command);
    void WriteData(const uint8_t* data, uint16_t size);

    const uint16_t width;
    const uint16_t height;
    uint16_t view_width;
    uint16_t view_height;
    SPI_HandleTypeDef* spi;
    GPIO_TypeDef* cs_port;
    uint16_t cs_pin;
    GPIO_TypeDef* dc_port;
    uint16_t dc_pin;
    GPIO_TypeDef* reset_port;
    uint16_t reset_pin;
    GPIO_TypeDef* bl_port;
    uint16_t bl_pin;
    volatile bool spi_busy;
};
