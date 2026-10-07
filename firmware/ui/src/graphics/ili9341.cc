#include "ili9341.hh"
#include "logger.hh"

ILI9341::ILI9341(uint16_t width,
                 uint16_t height,
                 SPI_HandleTypeDef& spi,
                 GPIO_TypeDef* cs_port,
                 uint16_t cs_pin,
                 GPIO_TypeDef* dc_port,
                 uint16_t dc_pin,
                 GPIO_TypeDef* reset_port,
                 uint16_t reset_pin,
                 GPIO_TypeDef* bl_port,
                 uint16_t bl_pin,
                 const Orientation orientation) :
    width(width),
    height(height),
    view_width(width),
    view_height(height),
    spi(&spi),
    cs_port(cs_port),
    cs_pin(cs_pin),
    dc_port(dc_port),
    dc_pin(dc_pin),
    reset_port(reset_port),
    reset_pin(reset_pin),
    bl_port(bl_port),
    bl_pin(bl_pin),
    spi_busy(false),
    orientation(orientation)
{
}

void ILI9341::Init()
{
    Reset();

    WriteCommand(Software_Reset);
    HAL_Delay(5U);

    const uint8_t power_control_a[] = {0x39U, 0x2CU, 0x00U, 0x34U, 0x02U};
    WriteCommand(Power_Control_A);
    WriteData(power_control_a, sizeof(power_control_a));

    const uint8_t power_control_b[] = {0x00U, 0xC1U, 0x30U};
    WriteCommand(Power_Control_B);
    WriteData(power_control_b, sizeof(power_control_b));

    const uint8_t driver_timing_a[] = {0x85U, 0x00U, 0x78U};
    WriteCommand(Driver_Timing_Control_A);
    WriteData(driver_timing_a, sizeof(driver_timing_a));

    const uint8_t driver_timing_b[] = {0x00U, 0x00U};
    WriteCommand(Driver_Timing_Control_B);
    WriteData(driver_timing_b, sizeof(driver_timing_b));

    const uint8_t power_on_sequence[] = {0x64U, 0x03U, 0x12U, 0x81U};
    WriteCommand(Power_On_Sequence_Control);
    WriteData(power_on_sequence, sizeof(power_on_sequence));

    const uint8_t pump_ratio = 0x20U;
    WriteCommand(Pump_Ratio_Control);
    WriteData(&pump_ratio, 1U);

    const uint8_t power_control_1 = 0x23U;
    WriteCommand(Power_Control_1);
    WriteData(&power_control_1, 1U);

    const uint8_t power_control_2 = 0x10U;
    WriteCommand(Power_Control_2);
    WriteData(&power_control_2, 1U);

    const uint8_t vcom_control_1[] = {0x3EU, 0x28U};
    WriteCommand(VCOM_Control_1);
    WriteData(vcom_control_1, sizeof(vcom_control_1));

    const uint8_t vcom_control_2 = 0x86U;
    WriteCommand(VCOM_Control_2);
    WriteData(&vcom_control_2, 1U);

    const uint8_t pixel_format = 0x55U;
    WriteCommand(Pixel_Format_Set);
    WriteData(&pixel_format, 1U);

    const uint8_t frame_rate[] = {0x00U, 0x18U};
    WriteCommand(Frame_Rate_Control);
    WriteData(frame_rate, sizeof(frame_rate));

    const uint8_t display_function[] = {0x08U, 0x82U, 0x27U};
    WriteCommand(Display_Function_Control);
    WriteData(display_function, sizeof(display_function));

    const uint8_t gamma_disabled = 0x00U;
    WriteCommand(Gamma_Function_Disable);
    WriteData(&gamma_disabled, 1U);

    const uint8_t gamma_curve = 0x01U;
    WriteCommand(Gamma_Curve_Select);
    WriteData(&gamma_curve, 1U);

    const uint8_t positive_gamma[] = {0x0FU, 0x31U, 0x2BU, 0x0CU, 0x0EU, 0x08U, 0x4EU, 0xF1U,
                                      0x37U, 0x07U, 0x10U, 0x03U, 0x0EU, 0x09U, 0x00U};
    WriteCommand(Positive_Gamma_Correction);
    WriteData(positive_gamma, sizeof(positive_gamma));

    const uint8_t negative_gamma[] = {0x00U, 0x0EU, 0x14U, 0x03U, 0x11U, 0x07U, 0x31U, 0xC1U,
                                      0x48U, 0x08U, 0x0FU, 0x0CU, 0x31U, 0x36U, 0x0FU};
    WriteCommand(Negative_Gamma_Correction);
    WriteData(negative_gamma, sizeof(negative_gamma));

    WriteCommand(Normal_Display_Mode_On);
    WriteCommand(Sleep_Out);
    HAL_Delay(120U);
    WriteCommand(Display_On);
    Rotate(this->orientation);
}

void ILI9341::Reset()
{
    HAL_GPIO_WritePin(reset_port, reset_pin, GPIO_PIN_RESET);
    HAL_Delay(5U);
    HAL_GPIO_WritePin(reset_port, reset_pin, GPIO_PIN_SET);
    HAL_Delay(120U);
}

void ILI9341::EnableBacklight()
{
    HAL_GPIO_WritePin(bl_port, bl_pin, GPIO_PIN_SET);
}

void ILI9341::DisableBacklight()
{
    HAL_GPIO_WritePin(bl_port, bl_pin, GPIO_PIN_RESET);
}

void ILI9341::Rotate(Orientation orientation)
{
    uint8_t value = BGR_Order;

    this->orientation = orientation;
    switch (this->orientation)
    {
    case Orientation::Portrait:
        value |= Mirror_X;
        view_width = height;
        view_height = width;
        break;
    case Orientation::Landscape:
        value |= Swap_XY;
        view_width = width;
        view_height = height;
        break;
    case Orientation::Portrait_Inverted:
        value |= Mirror_Y;
        view_width = height;
        view_height = width;
        break;
    case Orientation::Landscape_Inverted:
        value |= Mirror_X | Mirror_Y | Swap_XY;
        view_width = width;
        view_height = height;
        break;
    }

    WriteCommand(Memory_Access_Control);
    WriteData(&value, 1U);
}

void ILI9341::SetRenderRegion(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end)
{
    if (x_start >= view_width || y_start >= view_height || x_end < x_start || y_end < y_start)
    {
        return;
    }

    if (x_end >= view_width)
    {
        x_end = view_width - 1U;
    }

    if (y_end >= view_height)
    {
        y_end = view_height - 1U;
    }

    const uint8_t column[] = {static_cast<uint8_t>(x_start >> 8U), static_cast<uint8_t>(x_start),
                              static_cast<uint8_t>(x_end >> 8U), static_cast<uint8_t>(x_end)};
    const uint8_t page[] = {static_cast<uint8_t>(y_start >> 8U), static_cast<uint8_t>(y_start),
                            static_cast<uint8_t>(y_end >> 8U), static_cast<uint8_t>(y_end)};

    UI_LOG_INFO("Set render region (%d, %d) (%d %d)", x_start, y_start, x_end, y_end);
    WriteCommand(Column_Address_Set);
    WriteData(column, sizeof(column));
    WriteCommand(Page_Address_Set);
    WriteData(page, sizeof(page));
    WriteCommand(Memory_Write);
}

bool ILI9341::Render(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0U || size > UINT16_MAX || spi_busy)
    {
        return false;
    }

    Select();
    HAL_GPIO_WritePin(dc_port, dc_pin, GPIO_PIN_SET);
    spi_busy = true;

    if (HAL_SPI_Transmit_DMA(spi, data, static_cast<uint16_t>(size)) != HAL_OK)
    {
        spi_busy = false;
        Deselect();
        return false;
    }

    return true;
}

bool ILI9341::IsSpiBusy() const
{
    return spi_busy;
}

void ILI9341::OnSpiTransmitComplete(SPI_HandleTypeDef* completed_spi)
{
    if (completed_spi != spi || !spi_busy)
    {
        return;
    }

    spi_busy = false;
    Deselect();
}

void ILI9341::Sleep()
{
    WriteCommand(Display_Off);
    WriteCommand(Sleep_In);
    HAL_Delay(5U);
}

void ILI9341::Wake()
{
    WriteCommand(Sleep_Out);
    HAL_Delay(120U);
    WriteCommand(Display_On);
}

uint16_t ILI9341::GetWidth() const
{
    return view_width;
}

uint16_t ILI9341::GetHeight() const
{
    return view_height;
}

void ILI9341::Select()
{
    HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_RESET);
}

void ILI9341::Deselect()
{
    HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_SET);
}

void ILI9341::WriteCommand(uint8_t command)
{
    Select();
    HAL_GPIO_WritePin(dc_port, dc_pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(spi, &command, 1U, Spi_Timeout);
    Deselect();
}

void ILI9341::WriteData(const uint8_t* data, uint16_t size)
{
    if (size == 0U)
    {
        return;
    }

    Select();
    HAL_GPIO_WritePin(dc_port, dc_pin, GPIO_PIN_SET);
    HAL_SPI_Transmit(spi, data, size, Spi_Timeout);
    Deselect();
}
