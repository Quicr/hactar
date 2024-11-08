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
    updating(false),
    restart_update(false),
    matrix({ Colour::BLACK }),
    scan_window({ 0 })
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
    if (restart_update)
    {
        restart_update = false;
        updating = true;
        row = 0;
    }

    if (!updating)
    {
        return;
    }

    if (memory_read_idx >= Num_Memories)
    {
        memory_read_idx = 0;
    }

    constexpr uint16_t num_rows = 32;
    const uint16_t end_row = row + num_rows;
    Select();
    // TODO different orientations
    SetWriteablePixels(0, view_width, row, end_row);
    uint16_t read_idx = memory_read_idx;
    uint16_t num_mems = memories_in_use;
    // Go through all of the memories
    for (uint16_t j = 0; j < num_mems; ++j)
    {
        if (read_idx >= Num_Memories)
        {
            read_idx = 0;
        }

        // Get a memory
        DrawMemory& memory = memories[read_idx++];

        switch (memory.status)
        {
            case MemoryStatus::Unused:
            {
                // TODO fix
                Error_Handler();
                continue;
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
                // Skip this memory
                continue;
            }
            default:
            {
                // Should never be able to get here.
                Error_Handler();
                break;
            }
        }

        memory.callback(*this, memory, row, end_row);
    }

    // Fill the row
    for (uint16_t i = row; i < end_row; ++i)
    {
        const uint16_t y_idx = (i - row) * view_width * 2;
        for (uint16_t j = 0; j < view_width; ++j)
        {
            const uint16_t idx = y_idx + j * 2;
            scan_window[idx] = colour_map[(size_t)matrix[i][j]] >> 8;
            scan_window[idx + 1] = colour_map[(size_t)matrix[i][j]] & 0xFF;
        }
    }

    // Send all of the data off.
    WriteData(scan_window, num_rows * view_width * 2);
    row += num_rows;
    if (row >= view_height)
    {
        row = 0;
    }

    Deselect();
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

    uint8_t scan_window [] = {
        static_cast<const uint8_t>(y1 >> 8), static_cast<const uint8_t>(y1),
        static_cast<const uint8_t>(y2 >> 8), static_cast<const uint8_t>(y2),
    };

    WriteCommand(CA_SET);
    WriteData(col_data, sizeof(col_data));

    WriteCommand(RA_SET);
    WriteData(scan_window, sizeof(scan_window));

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


void Screen::FillRectangle(uint16_t x1, uint16_t x2,
    uint16_t y1, uint16_t y2, const Colour colour)
{
    BoundCheck(x1, x2, y1, y2);

    // DrawMemory
    DrawMemory& memory = RetrieveMemory();

    memory.callback = Screen::FillRectangleProcedure;
    memory.status = MemoryStatus::In_Progress;
    memory.x1 = x1;
    memory.x2 = x2;
    memory.y1 = y1;
    memory.y2 = y2;
    memory.colour = colour;
}

void Screen::DrawRectangle(uint16_t x1, uint16_t x2,
    uint16_t y1, uint16_t y2, const uint16_t thickness,
    const Colour colour)
{
    BoundCheck(x1, x2, y1, y2);

    DrawMemory& memory = RetrieveMemory();

    memory.callback = Screen::DrawRectangleProcedure;
    memory.status = MemoryStatus::In_Progress;
    memory.x1 = x1;
    memory.x2 = x2;
    memory.y1 = y1;
    memory.y2 = y2;
    memory.colour = colour;

    memory.parameters[0] = thickness >> 8;
    memory.parameters[1] = thickness & 0xFF;
}

// Private functions

void Screen::WriteCommand(uint8_t command)
{
    HAL_GPIO_WritePin(dc_port, dc_pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(spi, &command, sizeof(command), HAL_MAX_DELAY);
}

void Screen::WriteData(uint8_t* data, const uint32_t data_size)
{
    // TODO async
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

    restart_update = true;
    ++memories_in_use;
    return memory;
}

void Screen::FillRectangleProcedure(Screen& screen, DrawMemory& memory,
    const uint16_t y1, const uint16_t y2)
{
    // See if rectangle is in the current scan line
    if (y1 < memory.y1 && y2 > memory.y2)
    {
        return;
    }

    // Get the y bounds
    const uint16_t y_min = y1 > memory.y1 ? y1 : memory.y1;
    const uint16_t y_max = y2 < memory.y2 ? y2 : memory.y2;

    // Get the colour
    for (uint16_t i = y_min; i < y_max; ++i)
    {
        for (uint16_t j = memory.x1; j < memory.x2; ++j)
        {
            screen.matrix[i][j] = memory.colour;
        }
    }

    if (y2 >= memory.y2)
    {
        memory.status = MemoryStatus::Complete;
    }
}


void Screen::DrawRectangleProcedure(Screen& screen, DrawMemory& memory,
    const uint16_t y1, const uint16_t y2)
{
    // See if rectangle is in the current scan line
    if (y1 < memory.y1 && y2 > memory.y2)
    {
        return;
    }

    const uint16_t thickness = memory.parameters[0] << 8 | memory.parameters[1];

    // Get the y bounds
    const uint16_t y_min = y1 > memory.y1 ? y1 : memory.y1;
    const uint16_t y_max = y2 < memory.y2 ? y2 : memory.y2;
    const uint16_t y1_thick = memory.y1 + thickness > y_max ? memory.y1 + thickness : y_max;
    const uint16_t y2_thick = memory.y2 - thickness < y_min ? memory.y2 - thickness : y_min;

    const uint16_t x1_thick = memory.x1 + thickness;
    const uint16_t x2_thick = memory.x2 - thickness;


    // Check for the "top rectangle"
    if (y1 < memory.y1)
    {
        // In the bounds of the top rectangle
        for (uint16_t i = y_min; i < y1_thick; ++i)
        {
            for (uint16_t j = memory.x1; j < memory.x2; ++j)
            {
                screen.matrix[i][j] = memory.colour;
            }
        }
    }

    // Check for the "bottom rectangle"
    if (y_max > y2_thick)
    {
        // In the bounds of the top rectangle
        for (uint16_t i = y2_thick; i < y_max; ++i)
        {
            for (uint16_t j = memory.x1; j < memory.x2; ++j)
            {
                screen.matrix[i][j] = memory.colour;
            }
        }
    }

    // Do the side rectangles
    for (uint16_t i = y_min; i < y_max; ++i)
    {
        for (uint16_t j = memory.x1; j < x1_thick; ++j)
        {
            screen.matrix[i][j] = memory.colour;
        }
        for (uint16_t j = x2_thick; j < memory.x2; ++j)
        {
            screen.matrix[i][j] = memory.colour;
        }
    }

    if (y2 >= memory.y2)
    {
        memory.status = MemoryStatus::Complete;
    }
}

void Screen::BoundCheck(uint16_t& x1, uint16_t& x2, uint16_t& y1, uint16_t& y2)
{
    if (x1 >= view_width || y1 >= view_height)
    {
        return;
    }

    if (x2 >= view_width)
    {
        x2 = view_width;
    }

    if (x1 > x2)
    {
        const uint16_t tmp = x1;
        x1 = x2;
        x2 = tmp;
    }

    if (y1 > y2)
    {
        const uint16_t tmp = y1;
        y1 = y2;
        y2 = tmp;
    }
}
