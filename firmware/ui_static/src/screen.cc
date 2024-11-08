#include "screen.hh"

Screen::Screen(
    SPI_HandleTypeDef& hspi,
    GPIO_TypeDef* cs_port,
    const uint16_t cs_pin,
    GPIO_TypeDef* dc_port,
    const uint16_t dc_pin,
    GPIO_TypeDef* rst_port,
    const uint16_t rst_pin,
    GPIO_TypeDef* bl_port,
    const uint16_t bl_pin,
    Orientation orientation
):
    spi(&hspi),
    cs_port(cs_port),
    cs_pin(cs_pin),
    dc_port(dc_port),
    dc_pin(dc_pin),
    rst_port(rst_port),
    rst_pin(rst_pin),
    bl_port(bl_port),
    bl_pin(bl_pin),
    orientation(portrait),
    view_height(HEIGHT),
    view_width(WIDTH),
    memories({ 0 }),
    memories_in_use(0),
    memory_read_idx(0),
    memory_write_idx(0),
    row(0),
    matrix({ Colour::BLACK }),
    row_data({ 0 })
{

}

void Screen::Init()
{
    Select();
    Reset();

    WriteCommand(SF_RST);
    HAL_Delay(5); // wait 5ms

    // Set power control A
    WriteCommand(PWRC_A);
    uint8_t power_a_data[5] = { 0x39, 0x2C, 0x00, 0x34, 0x02 };
    WriteData(power_a_data, 5);

    // Set power control B
    WriteCommand(PWRC_B);
    uint8_t power_b_data[3] = { 0x00, 0xC1, 0x30 };
    WriteData(power_b_data, 3);

    // Driver timing control A
    WriteCommand(TIMC_A);
    uint8_t timer_a_data[3] = { 0x85, 0x00, 0x78 };
    WriteData(timer_a_data, 3);

    // Driver timing control B
    WriteCommand(TIMC_B);
    uint8_t timer_b_data[2] = { 0x00, 0x00 };
    WriteData(timer_b_data, 2);

    // Power on sequence control
    WriteCommand(PWR_ON);
    uint8_t power_data[4] = { 0x64, 0x03, 0x12, 0x81 };
    WriteData(power_data, 4);

    // Pump ratio control
    WriteCommand(PMP_RA);
    WriteData(0x20);

    // Power control VRH[5:0]
    WriteCommand(PC_VRH); // 0xC0
    WriteData(0x23);

    // Power control SAP[2:0];BT[3:0]
    WriteCommand(PC_SAP); // 0xC1
    WriteData(0x10);

    // VCM Control 1
    WriteCommand(VCM_C1);
    uint8_t vcm_control[2] = { 0x3E, 0x28 };
    WriteData(vcm_control, 2);

    // VCM Control 2
    WriteCommand(VCM_C2);
    WriteData(0x86);

    // Memory access control
    WriteCommand(MEM_CR);
    WriteData(0x48);

    // Pixel format
    WriteCommand(PIX_FM);
    WriteData(0x55);

    // Frame ratio control. RGB Color
    WriteCommand(FR_CTL); // 0xB1
    uint8_t fr_control_data[2] = { 0x00, 0x18 };
    WriteData(fr_control_data, 2);

    // Display function control
    WriteCommand(DIS_CT); // 0xB6
    uint8_t df_control_data[3] = { 0x08, 0x82, 0x27 };
    WriteData(df_control_data, 3);

    // 3Gamma function
    WriteCommand(GAMM_3); // 0xF2
    WriteData(0x00);

    // Gamma curve selected
    WriteCommand(GAMM_C); // 0x26
    WriteData(0x01);

    // Positive Gamma correction
    WriteCommand(GAM_PC); // 0xE0
    uint8_t positive_gamma_correction_data[15] = { 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1,
                              0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00 };
    WriteData(positive_gamma_correction_data, 15);

    // Negative gamma correction
    WriteCommand(GAM_NC);
    uint8_t negative_gamma_correction_data[15] = { 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1,
                              0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F };
    WriteData(negative_gamma_correction_data, 15);

    // Exit sleep
    WriteCommand(END_SL); // 0x11

    HAL_Delay(120);

    // Display on
    WriteCommand(DIS_ON); // 0x29

    // Set the orientation of the screen
    SetOrientation(orientation);

    Deselect();
}

// TODO use different draw code based on the orientation?
// TODO what happens if the matrix gets updated in rows
// that have already been drawn?
void Screen::Draw(uint32_t timeout)
{
    if (memories_in_use > 0)
    {
        // TODO adjust based on how long the following function takes to
        // breakout
        const uint32_t adjusted_timeout = timeout - 100;

        // While we have time compute
        while (uwTick < adjusted_timeout && memories_in_use > 0)
        {
            if (memory_read_idx >= Num_Memories)
            {
                memory_read_idx = 0;
            }

            DrawMemory& memory = memories[memory_read_idx];

            switch (memory.status)
            {
                case MemoryStatus::Unused:
                {
                    // If we somehow get here break
                    Error_Handler();
                    break;
                }
                case MemoryStatus::In_Progress:
                {
                    // Use this memory
                    break;
                }
                case MemoryStatus::Complete:
                {
                    --memories_in_use;
                    ++memory_read_idx;
                    memory.status = MemoryStatus::Unused;
                    // TODO should I zero out the values?
                }
                default:
                {
                    // Should never be able to get here.
                    break;
                }
            }

            memory.callback(*this, memory);
        }
    }
    else
    {
        Select();
        // TODO different orientations
        SetWriteablePixels(0, view_width, row, row+20);
        for (uint16_t i = row; i < row + 20; ++i)
        {
            // Fill the row
            for (uint16_t j = 0; j < view_width; ++j)
            {
                row_data[j * 2] = colour_map[(size_t)matrix[i][j]] >> 8;
                row_data[j * 2 + 1] = colour_map[(size_t)matrix[i][j]] & 0xFF;
            }
            WriteData(row_data, view_width * 2);
        }

        row += 20;
        if (row == view_height)
        {
            row = 0;
        }

        Deselect();
    }
}

void Screen::Sleep()
{
    // TODO
}

void Screen::Wake()
{
    // TODO
}

void Screen::Reset()
{
    HAL_GPIO_WritePin(rst_port, rst_pin, GPIO_PIN_RESET);
    HAL_Delay(50);
    HAL_GPIO_WritePin(rst_port, rst_pin, GPIO_PIN_SET);
}

void Screen::Select()
{
    HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_RESET);
}

void Screen::Deselect()
{
    HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_SET);
}

void Screen::EnableBacklight()
{
    HAL_GPIO_WritePin(bl_port, bl_pin, GPIO_PIN_SET);
}

void Screen::DisableBacklight()
{
    HAL_GPIO_WritePin(bl_port, bl_pin, GPIO_PIN_RESET);
}

void Screen::SetWriteablePixels(const uint16_t x1, const uint16_t x2,
    const uint16_t y1, const uint16_t y2)
{
    uint8_t col_data [] = {
        static_cast<const uint8_t>(x1 >> 8), static_cast<const uint8_t>(x1),
        static_cast<const uint8_t>(x2 >> 8), static_cast<const uint8_t>(x2),
    };

    uint8_t row_data [] = {
        static_cast<const uint8_t>(y1 >> 8), static_cast<const uint8_t>(y1),
        static_cast<const uint8_t>(y2 >> 8), static_cast<const uint8_t>(y2),
    };

    WriteCommand(CA_SET);
    WriteData(col_data, sizeof(col_data));

    WriteCommand(RA_SET);
    WriteData(row_data, sizeof(row_data));

    WriteCommand(WR_RAM);
}

void Screen::SetOrientation(const Screen::Orientation orientation)
{
    // TODO fix
    switch (orientation)
    {
        case Orientation::portrait:
            view_width = WIDTH;
            view_height = HEIGHT;

            WriteCommand(MAD_CT);
            WriteData(PORTRAIT_DATA);
            break;
        case Orientation::flipped_portrait:
            view_width = WIDTH;
            view_height = HEIGHT;

            WriteCommand(MAD_CT);
            WriteData(FLIPPED_PORTRAIT_DATA);
            break;
        case Orientation::left_landscape:
            view_width = HEIGHT;
            view_height = WIDTH;

            WriteCommand(MAD_CT);
            WriteData(LEFT_LANDSCAPE_DATA);
            break;
        case Orientation::right_landscape:
            view_width = HEIGHT;
            view_height = WIDTH;

            WriteCommand(MAD_CT);
            WriteData(RIGHT_LANDSCAPE_DATA);
            break;
        default:
            // Do nothing
            break;
    }

    row_bytes = view_width * 2;
    this->orientation = orientation;
}


void Screen::FillRectangle(const uint16_t x1, const uint16_t x2,
    const uint16_t y1, const uint16_t y2, const Colour colour)
{
    uint16_t _x1 = x1;
    uint16_t _x2 = x2;
    uint16_t _y1 = y1;
    uint16_t _y2 = y2;

    if (_x1 >= view_width || _y1 >= view_height)
    {
        return;
    }

    if (_x2 >= view_width)
    {
        _x2 = view_width;
    }

    if (_x1 > _x2)
    {
        const uint16_t tmp = _x1;
        _x1 = _x2;
        _x2 = tmp;
    }

    if (_y1 > _y2)
    {
        const uint16_t tmp = _y1;
        _y1 = _y2;
        _y2 = tmp;
    }

    // Fill the matrix
    // for (uint16_t i = _y1; i < _y2; ++i)
    // {
    //     for (uint16_t j = _x1; j < _x2; ++j)
    //     {
    //         matrix[i][j] = colour;
    //     }
    // }

    // DrawMemory
    DrawMemory& memory = RetrieveMemory();

    memory.callback = Screen::FillRectangleProcedure;
    memory.status = MemoryStatus::In_Progress;
    memory.x1 = _x1;
    memory.x2 = _x2;
    memory.y1 = _y1;
    memory.y2 = _y2;
    memory.colour = colour;
}

// Private functions

void Screen::WriteCommand(uint8_t command)
{
    HAL_GPIO_WritePin(dc_port, dc_pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(spi, &command, sizeof(command), HAL_MAX_DELAY);
}

void Screen::WriteData(uint8_t* data, const uint32_t data_size)
{
    HAL_GPIO_WritePin(dc_port, dc_pin, GPIO_PIN_SET);
    HAL_SPI_Transmit(spi, data, data_size, HAL_MAX_DELAY);
}

void Screen::WriteData(uint8_t data)
{
    HAL_GPIO_WritePin(dc_port, dc_pin, GPIO_PIN_SET);
    HAL_SPI_Transmit(spi, &data, sizeof(data), HAL_MAX_DELAY);
}

Screen::DrawMemory& Screen::RetrieveMemory()
{
    if (memories_in_use == Num_Memories)
    {
        Error_Handler();
    }

    if (memory_write_idx >= Num_Memories)
    {
        memory_write_idx = 0;
    }

    Screen::DrawMemory& memory = memories[memory_write_idx++];

    ++memories_in_use;
    return memory;
}

void Screen::DrawRectangleProcedure(Screen& screen, DrawMemory& memory)
{

}


void Screen::FillRectangleProcedure(Screen& screen, DrawMemory& memory)
{
    // TODO fix
    if (memory.status == MemoryStatus::Complete)
    {
        return;
    }

    // See if rectangle is in the current scan line
    if (screen.row < memory.y1 || screen.row > memory.y2)
    {
        return;
    }

    // Within the bounds
    Screen::Colour* mat_row = screen.matrix[screen.row];
    for (uint16_t i = memory.x1; i < memory.x2; ++i)
    {
        mat_row[i++] = memory.colour;
    }

    if (screen.row == memory.y2)
    {
        memory.status = MemoryStatus::Complete;
    }
}