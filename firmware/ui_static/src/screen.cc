#include "screen.hh"

// TODO vertical scrolling

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
    orientation(orientation),
    view_height(HEIGHT),
    view_width(WIDTH),
    memories{ 0 },
    memories_in_use(0),
    memory_write_idx(0),
    row(0),
    updating(false),
    restart_update(false),
    matrix{ Colour::BLACK },
    scan_window{ 0 },
    title_buffer{ 0 },
    text_idx(0),
    texts_in_use(0),
    scroll_offset(0),
    text_buffer{ 0 },
    usr_buffer{ 0 }
{

}


// NOTE, we never deselect the screen because its the only
// thing on our spi bus.
void Screen::Init()
{
    Select();
    Reset();

    WriteCommand(SF_RST);
    HAL_Delay(5); // wait 5ms

    // Set power control A
    WriteCommand(PWRC_A);
    uint8_t power_a_data[5] = { 0x39, 0x2C, 0x00, 0x34, 0x02 };
    WriteDataWithSet(power_a_data, 5);

    // Set power control B
    WriteCommand(PWRC_B);
    uint8_t power_b_data[3] = { 0x00, 0xC1, 0x30 };
    WriteDataWithSet(power_b_data, 3);

    // Driver timing control A
    WriteCommand(TIMC_A);
    uint8_t timer_a_data[3] = { 0x85, 0x00, 0x78 };
    WriteDataWithSet(timer_a_data, 3);

    // Driver timing control B
    WriteCommand(TIMC_B);
    uint8_t timer_b_data[2] = { 0x00, 0x00 };
    WriteDataWithSet(timer_b_data, 2);

    // Power on sequence control
    WriteCommand(PWR_ON);
    uint8_t power_data[4] = { 0x64, 0x03, 0x12, 0x81 };
    WriteDataWithSet(power_data, 4);

    // Pump ratio control
    WriteCommand(PMP_RA);
    WriteDataWithSet(0x20);

    // Power control VRH[5:0]
    WriteCommand(PC_VRH); // 0xC0
    WriteDataWithSet(0x23);

    // Power control SAP[2:0];BT[3:0]
    WriteCommand(PC_SAP); // 0xC1
    WriteDataWithSet(0x10);

    // VCM Control 1
    WriteCommand(VCM_C1);
    uint8_t vcm_control[2] = { 0x3E, 0x28 };
    WriteDataWithSet(vcm_control, 2);

    // VCM Control 2
    WriteCommand(VCM_C2);
    WriteDataWithSet(0x86);

    // Memory access control
    WriteCommand(MEM_CR);
    WriteDataWithSet(0x48);

    // Pixel format
    WriteCommand(PIX_FM);
    WriteDataWithSet(0x55);

    // Frame ratio control. RGB Color
    WriteCommand(FR_CTL); // 0xB1
    uint8_t fr_control_data[2] = { 0x00, 0x18 };
    WriteDataWithSet(fr_control_data, 2);

    // Display function control
    WriteCommand(DIS_CT); // 0xB6
    uint8_t df_control_data[3] = { 0x08, 0x82, 0x27 };
    WriteDataWithSet(df_control_data, 3);

    // 3Gamma function
    WriteCommand(GAMM_3); // 0xF2
    WriteDataWithSet(0x00);

    // Gamma curve selected
    WriteCommand(GAMM_C); // 0x26
    WriteDataWithSet(0x01);

    // Positive Gamma correction
    WriteCommand(GAM_PC); // 0xE0
    uint8_t positive_gamma_correction_data[15] = { 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1,
                              0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00 };
    WriteDataWithSet(positive_gamma_correction_data, 15);

    // Negative gamma correction
    WriteCommand(GAM_NC);
    uint8_t negative_gamma_correction_data[15] = { 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1,
                              0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F };
    WriteDataWithSet(negative_gamma_correction_data, 15);

    WriteCommand(NORON); // 0x13
    // Exit sleep
    WriteCommand(END_SL); // 0x11

    HAL_Delay(120);

    // Display on
    WriteCommand(DIS_ON); // 0x29

    // Set the orientation of the screen
    SetOrientation(orientation);
}

// TODO use different draw code based on the orientation?
// TODO what happens if the matrix gets updated in rows
// that have already been drawn?
void Screen::Draw(uint32_t timeout)
{
    UNUSED(timeout);
    if (restart_update)
    {
        row = 0;
        restart_update = false;
        updating = true;
        SetWriteablePixels(0, view_width - 1, 0, view_height - 1);
    }

    if (!updating)
    {
        return;
    }

    // TODO different orientations
    const uint16_t end_row = row + NUM_ROWS;

    // Go through all of the memories
    for (uint16_t j = 0; j < Num_Memories; ++j)
    {
        // Get a memory
        DrawMemory& memory = memories[j];

        switch (memory.status)
        {
            case MemoryStatus::Free:
            {
                continue;
                break;
            }
            case MemoryStatus::In_Progress:
            {
                memory.callback(*this, memory, row, end_row);

                if (end_row >= memory.y2)
                {
                    memory.status = MemoryStatus::Free;
                    memory.read_idx = 0;
                    memory.write_idx = 0;
                    --memories_in_use;
                }
                break;
            }
            default:
            {
                // Should never be able to get here.
                Error_Handler();
                break;
            }
        }
    }

    // Fill the row
    uint16_t mod = 0;
    int16_t y_idx = 0;
    for (uint16_t i = row; i < end_row; ++i)
    {
        // const uint16_t y_idx = (i - row) * view_width * 2;
        y_idx = i;
        if (y_idx > Top_Fixed_Area && y_idx < Bottom_Fixed_Area)
        {
            // So many tabs ew.
            y_idx = (int16_t)Top_Fixed_Area - scroll_offset;
            if (y_idx < Top_Fixed_Area)
            {
                if (y_idx < 0)
                {
                    y_idx *= -1;
                }

                // Get the y_idx according to the offset amt
                y_idx = Scroll_Area_Bottom - y_idx;
            }

        }
        for (uint16_t j = 0; j < view_width; ++j)
        {
            const uint16_t idx = mod * 480 + (j * 2);
            scan_window[idx] = colour_map[(size_t)matrix[y_idx][j]] >> 8;
            scan_window[idx + 1] = colour_map[(size_t)matrix[y_idx][j]] & 0xFF;
        }

        WriteDataAsync(&scan_window[mod * 480], view_width * 2);
        mod = !mod;
    }

    // Send all of the data off.
    row += NUM_ROWS;
    if (row >= view_height)
    {
        row = 0;
        updating = false;
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
        static_cast<uint8_t>(x1 >> 8), static_cast<uint8_t>(x1),
        static_cast<uint8_t>(x2 >> 8), static_cast<uint8_t>(x2),
    };

    uint8_t scan_window [] = {
        static_cast<uint8_t>(y1 >> 8), static_cast<uint8_t>(y1),
        static_cast<uint8_t>(y2 >> 8), static_cast<uint8_t>(y2),
    };

    WriteCommand(CA_SET);
    WriteDataWithSet(col_data, sizeof(col_data));

    WriteCommand(RA_SET);
    WriteDataWithSet(scan_window, sizeof(scan_window));

    WriteCommand(WR_RAM);
    SetPinToData();
}

void Screen::SetOrientation(const Screen::Orientation orientation)
{
    // TODO fix
    // TODO add scroll area math
    switch (orientation)
    {
        case Orientation::portrait:
            view_width = WIDTH;
            view_height = HEIGHT;

            WriteCommand(MAD_CT);
            WriteDataWithSet(PORTRAIT_DATA);
            break;
        case Orientation::flipped_portrait:
            view_width = WIDTH;
            view_height = HEIGHT;

            WriteCommand(MAD_CT);
            WriteDataWithSet(FLIPPED_PORTRAIT_DATA);
            break;
        case Orientation::left_landscape:
            view_width = HEIGHT;
            view_height = WIDTH;

            WriteCommand(MAD_CT);
            WriteDataWithSet(LEFT_LANDSCAPE_DATA);
            break;
        case Orientation::right_landscape:
            view_width = HEIGHT;
            view_height = WIDTH;

            WriteCommand(MAD_CT);
            WriteDataWithSet(RIGHT_LANDSCAPE_DATA);
            break;
        default:
            // Do nothing
            break;
    }

    row_bytes = view_width * 2;
    this->orientation = orientation;

    DefineScrollArea(Top_Fixed_Area, Scroll_Area_Height, Bottom_Fixed_Area);

}


void Screen::FillRectangle(uint16_t x1, uint16_t x2,
    uint16_t y1, uint16_t y2, const Colour colour)
{
    BoundCheck(x1, x2, y1, y2);

    // DrawMemory
    DrawMemory& memory = RetrieveMemory();

    memory.callback = Screen::FillRectangleProcedure;
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
    memory.x1 = x1;
    memory.x2 = x2;
    memory.y1 = y1;
    memory.y2 = y2;
    memory.colour = colour;

    PushMemoryParameter(memory, thickness, 2);
}

void Screen::DrawCharacter(uint16_t x, uint16_t y, const char ch,
    const Font& font, const Colour fg, const Colour bg)
{
    const uint16_t x2 = x + font.width;
    const uint16_t y2 = y + font.height;
    const uint16_t offset = (ch - 32) * font.height * (font.width / 8 + 1);
    const uintptr_t ch_addr = (uintptr_t)(font.data + offset);

    DrawMemory& memory = RetrieveMemory();

    memory.callback = DrawCharacterProcedure;
    memory.x1 = x;
    memory.y1 = y;
    memory.x2 = x2;
    memory.y2 = y2;
    memory.colour = fg;

    PushMemoryParameter(memory, static_cast<uint32_t>(bg), 1);
    PushMemoryParameter(memory, ch_addr, 4);
}

void Screen::DrawString(uint16_t x, uint16_t y, const char* str,
    const uint16_t length, const Font& font,
    const Colour fg, const Colour bg)
{
    // Get the maximum num of characters for a single line
    const uint16_t len = x + length * static_cast<uint16_t>(font.width) > WIDTH ? (WIDTH - x) / font.width : length;
    const uint16_t width = len * font.width;

    const uint16_t x2 = x + width;
    const uint16_t y2 = y + font.height;

    DrawMemory& memory = RetrieveMemory();

    memory.callback = DrawStringProcedure;
    memory.x1 = x;
    memory.x2 = x2;
    memory.y1 = y;
    memory.y2 = y2;
    memory.colour = fg;

    PushMemoryParameter(memory, static_cast<uint8_t>(bg), 1);
    PushMemoryParameter(memory, (uint32_t)str, 4);
    PushMemoryParameter(memory, font.width, 1);
    PushMemoryParameter(memory, font.height, 1);
    PushMemoryParameter(memory, (uint32_t)font.data, 4);
}

void Screen::DefineScrollArea(const uint16_t tfa_idx,
    const uint16_t vsa_idx, const uint16_t bfa_idx)
{
    switch (orientation)
    {
        // TODO note- I have no idea if flipped portrait is the same as portrait
        case Orientation::portrait:
        case Orientation::left_landscape:
        {
            uint8_t vert_scroll_def_data [] = {
                static_cast<uint8_t>(tfa_idx >> 8), static_cast<uint8_t>(tfa_idx),
                static_cast<uint8_t>(vsa_idx >> 8), static_cast<uint8_t>(vsa_idx),
                static_cast<uint8_t>(bfa_idx >> 8), static_cast<uint8_t>(bfa_idx),
            };
            WriteCommand(VSCRDEF);
            WriteDataWithSet(vert_scroll_def_data, 6);

            break;
        }
        case Orientation::flipped_portrait:
        case Orientation::right_landscape:
        {
            uint8_t vert_scroll_def_data [] = {
                static_cast<uint8_t>(bfa_idx >> 8), static_cast<uint8_t>(bfa_idx),
                static_cast<uint8_t>(vsa_idx >> 8), static_cast<uint8_t>(vsa_idx),
                static_cast<uint8_t>(tfa_idx >> 8), static_cast<uint8_t>(tfa_idx),
            };
            WriteCommand(VSCRDEF);
            WriteDataWithSet(vert_scroll_def_data, 6);

            break;
        }
        default:
        {
            return;
        }
    }
}

void Screen::ScrollScreen(const uint16_t scroll_idx, bool up)
{
    static uint16_t scroll_d = 0;

    switch (orientation)
    {
        case Orientation::portrait:
        case Orientation::left_landscape:
        {
            if (up)
            {
                scroll_d = scroll_idx;
            }
            else
            {
                scroll_d = view_height - scroll_idx;
            }
            break;
        }
        case Orientation::flipped_portrait:
        case Orientation::right_landscape:
        {
            if (up)
            {
                scroll_d = view_height - scroll_idx;
            }
            else
            {
                scroll_d = scroll_idx;
            }
            break;
        }
        default:
        {
            scroll_d = 0;
            return;
        }
    }

    uint8_t vert_scroll_idx_data [] = {
        static_cast<uint8_t>(scroll_d >> 8), static_cast<uint8_t>(scroll_d)
    };

    WriteCommand(VSCRSADD);
    WriteDataWithSet(vert_scroll_idx_data, 2);
}

void Screen::AppendText(const char* text, const uint32_t len)
{
    // Work our way down
    const uint32_t num_bytes = Max_Characters < len ? Max_Characters : len;
    char* text_buf = text_buffer[text_idx];
    char& text_len = text_lens[text_idx];

    size_t idx = 0;
    while (text_len < Max_Characters && idx < num_bytes)
    {
        text_buf[text_len++] = text[idx++];
    }
}

void Screen::CommitText()
{
    // TODO rethink on how to do this.. might need two different
    // functions
    static uint16_t y = 0;
    const uint16_t idx = text_idx;

    if (++text_idx >= Max_Texts)
    {
        text_idx = 0;
    }

    y = Text_Start_Y + font5x8.height * (texts_in_use);
    // Send a scroll text command if we have a full text buff
    if (texts_in_use >= Max_Texts)
    {
        text_lens[text_idx] = 0;
        y = Text_Start_Y + font5x8.height * (texts_in_use - 1);
        // Scroll, this will happen before the draw string
        // TODO fix the scroll idx
        ++scroll_idx;
        ScrollScreen(Top_Fixed_Area + (font5x8.height * scroll_idx), true);
        if (scroll_idx > Max_Texts)
        {
            scroll_idx = 0;
        }

        // Draw a rectangle to remove the text from the screen.
        // TODO replace colour with a class variable
        FillRectangle(0, view_width, y, y + font5x8.height, Screen::Colour::BLACK);
        // return;
    }
    else
    {
        ++texts_in_use;
    }


    // Enqueue the string draw
    DrawString(0, y,
        text_buffer[idx], text_lens[idx],
        font5x8, Colour::WHITE, Colour::BLACK);
}

// Private functions

inline void Screen::WaitForSPIComplete()
{
    while (spi->State != HAL_SPI_STATE_READY)
    {
        __NOP();
    }
}

inline void Screen::SetPinToCommand()
{
    HAL_GPIO_WritePin(dc_port, dc_pin, GPIO_PIN_RESET);
}

inline void Screen::SetPinToData()
{
    HAL_GPIO_WritePin(dc_port, dc_pin, GPIO_PIN_SET);
}

void Screen::WriteCommand(uint8_t command)
{
    WaitForSPIComplete();
    SetPinToCommand();
    HAL_SPI_Transmit(spi, &command, sizeof(command), HAL_MAX_DELAY);
}

// SetPinToData needs to be called before this function
void Screen::WriteData(uint8_t* data, const uint32_t data_size)
{
    WaitForSPIComplete();
    HAL_SPI_Transmit(spi, data, data_size, HAL_MAX_DELAY);
}

// SetPinToData needs to be called before this function
void Screen::WriteDataAsync(uint8_t* data, const uint32_t data_size)
{
    WaitForSPIComplete();
    HAL_SPI_Transmit_DMA(spi, data, data_size);
}

void Screen::WriteDataWithSet(uint8_t data)
{
    WaitForSPIComplete();
    SetPinToData();
    HAL_SPI_Transmit(spi, &data, sizeof(data), HAL_MAX_DELAY);
}

void Screen::WriteDataWithSet(uint8_t* data, const uint32_t data_size)
{
    WaitForSPIComplete();
    SetPinToData();
    HAL_SPI_Transmit(spi, data, data_size, HAL_MAX_DELAY);
}

Screen::DrawMemory& Screen::RetrieveMemory()
{
    if (memories_in_use >= Num_Memories)
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
    memory.status = MemoryStatus::In_Progress;
    return memory;
}

void Screen::FillRectangleProcedure(Screen& screen, DrawMemory& memory,
    const uint16_t y1, const uint16_t y2)
{
    // See if rectangle is in the current scan line
    if (y1 > memory.y2 || y2 < memory.y1)
    {
        return;
    }

    // Get the y bounds
    const uint16_t y_start = y1 > memory.y1 ? y1 : memory.y1;
    const uint16_t y_end = y2 < memory.y2 ? y2 : memory.y2;

    // Get the colour
    for (uint16_t i = y_start; i < y_end; ++i)
    {
        for (uint16_t j = memory.x1; j < memory.x2; ++j)
        {
            screen.matrix[i][j] = memory.colour;
        }
    }
}


void Screen::DrawRectangleProcedure(Screen& screen, DrawMemory& memory,
    const uint16_t y1, const uint16_t y2)
{
    // See if rectangle is in the current scan line
    if (y1 > memory.y2 || y2 < memory.y1)
    {
        return;
    }

    const uint16_t thickness = PullMemoryParameter<uint16_t>(memory);

    // Get the y bounds
    const uint16_t y_start = y1 > memory.y1 ? y1 : memory.y1;
    const uint16_t y_end = y2 < memory.y2 ? y2 : memory.y2;
    const uint16_t y1_thick = memory.y1 + thickness < y_end ? memory.y1 + thickness : y_end;
    const uint16_t y2_thick = memory.y2 - thickness > y_start ? memory.y2 - thickness : y_start;

    const uint16_t x1_thick = memory.x1 + thickness;
    const uint16_t x2_thick = memory.x2 - thickness;

    // Check for the "top rectangle"
    if (y1 <= memory.y1)
    {
        // In the bounds of the top rectangle
        for (uint16_t i = y_start; i < y1_thick; ++i)
        {
            for (uint16_t j = memory.x1; j < memory.x2; ++j)
            {
                screen.matrix[i][j] = memory.colour;
            }
        }
    }

    // Check for the "bottom rectangle"
    if (y2 >= memory.y2)
    {
        // In the bounds of the top rectangle
        for (uint16_t i = y2_thick; i < y_end; ++i)
        {
            for (uint16_t j = memory.x1; j < memory.x2; ++j)
            {
                screen.matrix[i][j] = memory.colour;
            }
        }
    }

    // Do the side rectangles
    for (uint16_t i = y_start; i < y_end; ++i)
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
}

void Screen::DrawCharacterProcedure(Screen& screen, DrawMemory& memory,
    const uint16_t y1, const uint16_t y2)
{
    const Colour bg = static_cast<Colour>(PullMemoryParameter<uint8_t>(memory));
    uint8_t* ch_ptr = (uint8_t*)PullMemoryParameter<uint32_t>(memory);

    uint16_t w_off = 0;

    const uint16_t y_start = y1 > memory.y1 ? y1 : memory.y1;
    const uint16_t y_end = y2 < memory.y2 ? y2 : memory.y2;

    for (uint16_t i = y_start; i < y_end; ++i)
    {
        w_off = 0;
        for (uint16_t j = memory.x1; j < memory.x2; ++j)
        {
            if ((*ch_ptr << w_off) & 0x80)
            {
                screen.matrix[i][j] = memory.colour;
            }
            else
            {
                screen.matrix[i][j] = bg;
            }

            ++w_off;
            if (w_off >= 8)
            {
                w_off = 0;

                // At the end of the byte's bits, if a font > 8 bits
                // then we need to slide to the next byte. which is in the same
                // column
                ++ch_ptr;
            }
        }

        // Slide the pointer to the next row byte
        ++ch_ptr;
    }

    ch_ptr = nullptr;
}

void Screen::DrawStringProcedure(Screen& screen, DrawMemory& memory,
    const uint16_t y1, const uint16_t y2)
{
    // TODO put into function
    if (y1 > memory.y2 || y2 < memory.y1)
    {
        return;
    }

    const Colour bg = static_cast<Colour>(PullMemoryParameter<uint8_t>(memory));
    const uint8_t* str = (uint8_t*)PullMemoryParameter<uint32_t>(memory);
    const uint8_t font_width = PullMemoryParameter<uint8_t>(memory);
    const uint8_t font_height = PullMemoryParameter<uint8_t>(memory);
    uint8_t* font_data = (uint8_t*)PullMemoryParameter<uint32_t>(memory);

    const uint16_t bytes_per_char = (font_width / 8) + 1;
    const uint16_t y_start = y1 > memory.y1 ? y1 : memory.y1;
    const uint16_t y_end = y2 < memory.y2 ? y2 : memory.y2;

    uint16_t ch_idx = 0;
    uint8_t* ch_ptr = nullptr;

    // How many iterations we've been at the same character
    // If it exceeds the font width that means we are on the next
    // character in the string
    uint16_t x_char_iter = 0;
    uint16_t w_off = 0;

    // Get the current number of scan lines already passed
    uint16_t y_char_iter = 0;
    if (y1 > memory.y1)
    {
        // We have done some lines
        y_char_iter = (y1 - memory.y1);
    }

    for (uint16_t i = y_start; i < y_end; ++i)
    {
        x_char_iter = 0;
        w_off = 0;
        ch_idx = 0;
        ch_ptr = GetCharAddr(font_data, str[ch_idx], font_width,
            font_height) + (y_char_iter * bytes_per_char);

        for (uint16_t j = memory.x1; j < memory.x2; ++j)
        {
            if ((*ch_ptr << w_off) & 0x80)
            {
                screen.matrix[i][j] = memory.colour;
            }
            else
            {
                screen.matrix[i][j] = bg;
            }

            ++w_off;
            ++x_char_iter;

            // Check if we are on the next character
            if (x_char_iter >= font_width)
            {
                x_char_iter = 0;

                // Get the character
                ++ch_idx;
                ch_ptr = GetCharAddr(font_data, str[ch_idx], font_width,
                    font_height) + (y_char_iter * bytes_per_char);

                // Next character and reset w_off
                w_off = 0;
            }

            if (w_off >= 8)
            {
                w_off = 0;

                // At the end of the byte's bits, if a font > 8 bits
                // then we need to slide to the next byte. which is in the same
                // column
                ++ch_ptr;
            }
        }

        // Slide the pointer is the next row
        ++y_char_iter;
    }

    ch_ptr = nullptr;
    font_data = nullptr;
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

inline uint8_t* Screen::GetCharAddr(
    uint8_t* font_data,
    const uint8_t ch,
    const uint16_t font_width,
    const uint16_t font_height)
{
    return font_data + ((ch - 32) * font_height * (font_width / 8 + 1));
}

inline void Screen::PushMemoryParameter(DrawMemory& memory, const uint32_t val,
    const int16_t num_bytes)
{
    const int16_t bytes = num_bytes < 4 ? num_bytes : 4;

    uint8_t& idx = memory.write_idx;
    for (int16_t i = 0; i < bytes; ++i)
    {
        // We are relying on truncation to save a cycle
        memory.parameters[idx] = (val >> (8 * i));
        ++idx;
    }
}

// Non-destructive retrieval
template<typename T, typename std::enable_if<std::is_integral<T>::value, bool>::type = 0>
inline T Screen::PullMemoryParameter(DrawMemory& memory)
{
    const int16_t bytes = sizeof(T);
    T output = 0;
    uint8_t& idx = memory.read_idx;

    for (int16_t i = 0; i < bytes && i < memory.write_idx; ++i)
    {
        output |= memory.parameters[idx] << (8 * i);
        ++idx;
    }

    // Restart where we read from if the index is zero
    if (idx >= memory.write_idx)
    {
        idx = 0;
    }

    return output;
}