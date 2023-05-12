#include <algorithm>
#include "Helper.h"
#include "Screen.hh"
#include "RingBuffer.hh"

#define DELAY HAL_MAX_DELAY

Screen::Screen(SPI_HandleTypeDef &hspi,
               port_pin cs,
               port_pin dc,
               port_pin rst,
               port_pin bl,
               Orientation orientation) :
    spi_handle(&hspi),
    cs(cs),
    dc(dc),
    rst(rst),
    bl(bl),
    orientation(orientation),
    view_height(0),
    view_width(0),
    spi_busy(0)
{
}

Screen::~Screen()
{
}

void Screen::Begin()
{
    // Ensure the spi is initialize at this point
    if (spi_handle == nullptr) return;

    // Setup GPIO for the screen.
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    EnablePortIf(cs.port);
    HAL_GPIO_WritePin(cs.port, cs.pin, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = cs.pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(cs.port, &GPIO_InitStruct);

    EnablePortIf(rst.port);
    HAL_GPIO_WritePin(rst.port, rst.pin, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = rst.pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(rst.port, &GPIO_InitStruct);

    EnablePortIf(dc.port);
    HAL_GPIO_WritePin(dc.port, dc.pin, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = dc.pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(dc.port, &GPIO_InitStruct);

    EnablePortIf(bl.port);
    HAL_GPIO_WritePin(bl.port, bl.pin, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = bl.pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(bl.port, &GPIO_InitStruct);

    Deselect();
    Select();
    Reset();

    WriteCommand(SF_RST);
    HAL_Delay(1000);

    // Set power control A
    WriteCommand(PWRC_A);
    // TODO there are many memory leaks in this file
    uint8_t power_a_data[5] = {0x39, 0x2C, 0x00, 0x34, 0x02};
    WriteData(power_a_data, 5);

    // Set power control B
    WriteCommand(PWRC_B);
    uint8_t power_b_data[3] = {0x00, 0xC1, 0x30};
    WriteData(power_b_data, 3);

    // Driver timing control A
    WriteCommand(TIMC_A);
    uint8_t timer_a_data[3] = {0x85, 0x00, 0x78};
    WriteData(timer_a_data, 3);

    // Driver timing control B
    WriteCommand(TIMC_B);
    uint8_t timer_b_data[2] = {0x00, 0x00};
    WriteData(timer_b_data, 2);

    // Power on sequence control
    WriteCommand(PWR_ON);
    uint8_t power_data[4] = {0x64, 0x03, 0x12, 0x81};
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
    uint8_t vcm_control[2] = {0x3E, 0x28 };
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
    uint8_t fr_control_data[2] = {0x00, 0x18};
    WriteData(fr_control_data, 2);

    // Display function control
    WriteCommand(DIS_CT); // 0xB6
    uint8_t df_control_data[3] = {0x08, 0x82, 0x27};
    WriteData(df_control_data, 3);

    // 3Gamma function
    WriteCommand(GAMM_3); // 0xF2
    WriteData(0x00);

    // Gamma curve selected
    WriteCommand(GAMM_C); // 0x26
    WriteData(0x01);

    // Positive Gamma correction
    WriteCommand(GAM_PC); // 0xE0
    uint8_t positive_gamma_correction_data[15] = {0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1,
                              0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00};
    WriteData(positive_gamma_correction_data, 15);

    // Negative gamma correction
    WriteCommand(GAM_NC);
    uint8_t negative_gamma_correction_data[15] = {0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1,
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

void Screen::Select()
{
    // Set pin LOW for selection
    HAL_GPIO_WritePin(cs.port, cs.pin, GPIO_PIN_RESET);
}

void Screen::Deselect()
{
    // Set the pin to HIGH to deselect
    HAL_GPIO_WritePin(cs.port, cs.pin, GPIO_PIN_SET);
}

void Screen::Reset()
{
    HAL_GPIO_WritePin(rst.port, rst.pin, GPIO_PIN_RESET);
    HAL_Delay(50);
    HAL_GPIO_WritePin(rst.port, rst.pin, GPIO_PIN_SET);
}

void Screen::WriteCommand(uint8_t command)
{
    HAL_GPIO_WritePin(dc.port, dc.pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(spi_handle, &command, sizeof(command), HAL_MAX_DELAY);
}

// Note select needs to be called prior
// and deselect following
void Screen::WriteData(uint8_t* data, uint32_t data_size)
{
    HAL_GPIO_WritePin(dc.port, dc.pin, GPIO_PIN_SET);

    // Split data into chunks
    while (data_size > 0)
    {
        // Set the chunk size
        uint32_t chunk_size = data_size > 32768 ? 32768 : data_size;

        // Send the data to the spi interface
        HAL_SPI_Transmit(spi_handle, data, chunk_size, HAL_MAX_DELAY);

        // Slide the buffer over
        data += chunk_size;

        // Reduce the size of the buffer by the chunk size.
        data_size -= chunk_size;
    }
}
// TODO comments
// Note Select needs to be called prior
void Screen::WriteData(uint8_t data)
{
    HAL_GPIO_WritePin(dc.port, dc.pin, GPIO_PIN_SET);
    HAL_SPI_Transmit(spi_handle, &data, 1, DELAY);
}

void Screen::WriteDataDMA(uint8_t* data, const uint32_t data_size)
{
    spi_busy = 1;
    HAL_SPI_Transmit_DMA(spi_handle, data, data_size);

    // Wait for the SPI IT call complete to invoked
    uint32_t next_blink = 0;
    while (spi_busy)
    {
        if (HAL_GetTick() > next_blink)
        {
            next_blink += 500;
        }
        __NOP();
    }
}


/**
 * SetWritablePixels
 *
 * Sets the addressable location that can be drawn on the screen, smaller
 * updates are faster
 *
 */
void Screen::SetWritablePixels(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end)
{
    uint8_t col_data[] = { static_cast<uint8_t>(x_start >> 8), static_cast<uint8_t>(x_start),
                           static_cast<uint8_t>(x_end   >> 8), static_cast<uint8_t>(x_end) };
    uint8_t row_data[] = { static_cast<uint8_t>(y_start >> 8), static_cast<uint8_t>(y_start),
                           static_cast<uint8_t>(y_end   >> 8), static_cast<uint8_t>(y_end) };

    // Set drawable column ILI9341 command
    WriteCommand(CA_SET);
    WriteData(col_data, sizeof(col_data));

    // Set drawable row ILI9341 command
    WriteCommand(RA_SET);
    WriteData(row_data, sizeof(row_data));

    WriteCommand(WR_RAM);

    // Tell the ILI9341 that we are going to send data next
    HAL_GPIO_WritePin(dc.port, dc.pin, GPIO_PIN_SET);
}

void Screen::DrawRectangle(const uint16_t x_start, const uint16_t y_start,
                           const uint16_t x_end, const uint16_t y_end,
                           const uint16_t thickness, const uint16_t colour)
{
    // Fill top
    FillRectangle(x_start, y_start, x_end, y_start + thickness, colour);

    // Fill left
    FillRectangle(x_start, y_start, x_start + thickness, y_end, colour);

    // Fill right
    FillRectangle(x_end - thickness, y_start, x_end, y_end, colour);

    // Fill bottom
    FillRectangle(x_start, y_end - thickness, x_end, y_end, colour);
}

void Screen::DrawPixel(const uint16_t x, const uint16_t y, const uint16_t colour)
{
    if (x >= view_width || y >= view_height) return;
    Select();

    SetWritablePixels(x, y, x, y);

    // Draw pixel
    uint8_t pixel[2] = { static_cast<uint8_t>(colour >> 8),
                            static_cast<uint8_t>(colour) };
    WriteDataDMA(pixel, 2);

    Deselect();
}

// TODO consider adding x2 and y2 so it has a bounding box.
void Screen::DrawBlockAnimateString(const uint16_t x, const uint16_t y,
                                    const String &str, const Font &font,
                                    const uint16_t fg, const uint16_t bg,
                                    const uint16_t delay)
{
    // TODO word wrap..
    uint16_t x_pos = x;
    uint16_t x_end = x + font.width;
    uint16_t y_end = y + font.height;
    for (unsigned int i = 0; i < str.length(); i++)
    {
        // Fill in a rectangle
        FillRectangle(x_pos, y, x_end, y_end, fg);

        HAL_Delay(delay);

        // Clear the rectangle we drew
        FillRectangle(x_pos, y, x_end, y_end, bg);

        // Draw the string finally
        DrawText(x_pos, y, str[i], font, fg, bg);

        x_pos += font.width;
        x_end += font.width;
    }
}

void Screen::DrawText(const uint16_t x, const uint16_t y, const String &str,
    const Font &font, const uint16_t fg, const uint16_t bg,
    const bool wordwrap, uint32_t max_chunk_size)
{
    // TODO clean up.
    // If larger than the view port, it wouldn't be seen so skip
    if (x > view_width || y > view_height) return;

    // Bound the max size
    if (max_chunk_size > Screen::Max_Chunk_Size)
        max_chunk_size = Screen::Max_Chunk_Size;

    Select();

    uint32_t num_chr_per_line = (view_width - x) / font.width;
    uint32_t num_chr_remaining = str.length();
    uint32_t width = font.width * str.length();
    const uint32_t height = font.height;
    const uint32_t lines = 1 + ((font.width * str.length()) / (view_width - x));

    // Get the chunk size
    const uint32_t chunk = std::min<uint32_t>(width * height, max_chunk_size);
    uint32_t data_idx = 0;
    uint8_t* data = new uint8_t[chunk*2];

    uint32_t ch_idx = 0;
    uint32_t ch_idx_offset = 0;
    char ch;
    const uint8_t* ch_ptr = nullptr;
    uint32_t ptr_offset=0;
    uint8_t bits_read = 0;
    uint8_t bit_mask = 0;
    uint16_t num_chrs;

    for (uint32_t i = 0; i < lines; ++i)
    {
        num_chrs = std::min<uint32_t>(
            num_chr_remaining, num_chr_per_line);
        width = num_chrs * font.width;

        SetWritablePixels(x, y + (font.height * i), x + width - 1,
                          y + (font.height * i) + height - 1);

        for (uint32_t h_idx = 0; h_idx < height; h_idx++)
        {
            for (uint32_t w_idx = 0; w_idx < width; w_idx++)
            {
                // Get the next character in the string
                if (bits_read % font.width == 0)
                {
                    ch = str[ch_idx + ch_idx_offset];
                    ch_idx++;

                    /*
                    * Each Character is offset by the height number of bytes
                    * multiplied by the number of bytes in the width - width/8
                    * Then we need to account for what line we are currently
                    * reading from based on the font height and width.
                    */
                    ptr_offset = ((ch - 32) * font.height * (font.width/8 + 1))
                        + (h_idx % font.height * (font.width/8 + 1));

                    // Get the pointer to the current row we are reading from
                    ch_ptr = &font.data[ptr_offset];

                    // Reset the bits we've read from the this row
                    bits_read = 0;

                    // Back to bit shift of 0
                    bit_mask = 0;
                }

                // Check if the pixel is activated
                if ((*ch_ptr << (bit_mask++ % 8)) & 0x80)
                {
                    data[data_idx++] = static_cast<uint8_t>(fg >> 8);
                    data[data_idx++] = static_cast<uint8_t>(fg);
                }
                else
                {
                    data[data_idx++] = static_cast<uint8_t>(bg >> 8);
                    data[data_idx++] = static_cast<uint8_t>(bg);
                }

                // If the buffer is full, send the data and clear it
                if (data_idx >= chunk * 2)
                {
                    WriteDataDMA(data, data_idx);
                    data_idx = 0;
                }

                // If we've exhausted the bits in the current byte then slide
                // over to the next byte
                if (++bits_read % 8 == 0)
                {
                    // Get the next byte if it is the end of the byte
                    ch_ptr++;
                    bit_mask = 0;
                }
            }

            ch_idx = 0;
        }


        // If there is remaining data to send
        if (data_idx > 0)
        {
            WriteDataDMA(data, data_idx);
            data_idx = 0;
        }

        ch_idx_offset += num_chrs;
        num_chr_remaining -= num_chrs;
    }

    delete [] data;
    ch_ptr = nullptr;

    Deselect();

    return;
}

// TODO update
void Screen::DrawTextbox(uint16_t x_pos,
                         uint16_t y_pos,
                         const uint16_t x_window_start,
                         const uint16_t y_window_start,
                         const uint16_t x_window_end,
                         const uint16_t y_window_end,
                         const String &str,
                         const Font &font,
                         const uint16_t fg,
                         const uint16_t bg)
{
    Vector<String> words;

    // Find each word in put into a vector
    bool found_space = false;
    char ch;
    uint16_t sz = str.length();
    String word;
    for (uint16_t i = 0; i < sz; i++)
    {
        ch = str[i];
        found_space = ch == ' ';
        word.push_back(ch);

        // If we found the space this is the end of the word (including the space)
        if (found_space)
        {
            words.push_back(word);
            word.clear();
        }
    }
    if (word.length() > 0) words.push_back(word);

    uint16_t num_words = words.size();

    uint16_t i, j;

    Select();

    for (i = 0; i < num_words; i++)
    {
        word = words[i];

        // If we are clipping the end of the box then just stop processing
        // Minus the space at the end of the word
        if (x_pos + ((word.length() - 1) * font.width) >= x_window_end)
        {
            x_pos = x_window_start;
            y_pos += font.height;
        }

        if (y_pos >= y_window_end) break;

        sz = word.length();
        for (j = 0; j < sz; j++)
        {
            ch = word[j];
            DrawCharacter(x_pos, y_pos, x_window_start, y_window_start,
                x_window_end, y_window_end, ch, font, fg, bg);
            x_pos += font.width;
        }
    }

    Deselect();
}
void Screen::FillRectangle(const uint16_t x_start,
                           const uint16_t y_start,
                           uint16_t x_end,
                           uint16_t y_end,
                           const uint16_t colour,
                           uint32_t max_chunk_size)
{
    // Clip to the size of the screen
    if (x_start >= view_width || y_start >= view_height) return;

    if (x_end >= view_width) x_end = view_width;
    if (y_end >= view_height) y_end = view_height;

    if (max_chunk_size > Screen::Max_Chunk_Size)
        max_chunk_size = Screen::Max_Chunk_Size;

    Select();
    SetWritablePixels(x_start, y_start, x_end - 1, y_end - 1);
    HAL_GPIO_WritePin(dc.port, dc.pin, GPIO_PIN_SET);

    uint16_t y_pixels = y_end - y_start;
    uint16_t x_pixels = x_end - x_start;

    uint32_t total_pixels = y_pixels * x_pixels;

    uint32_t chunk = std::min<uint32_t>(total_pixels, max_chunk_size);
    uint8_t* data = new uint8_t[chunk*2];

    // Copy colour to data
    for (uint32_t i = 0; i < chunk*2; i+=2)
    {
        data[i] = static_cast<uint8_t>(colour >> 8);
        data[i+1] = static_cast<uint8_t>(colour);
    }

    while (total_pixels > 0)
    {
        WriteDataDMA(data, chunk*2);

        total_pixels -= chunk;

        // Get the size of data we are sending that is remaining
        chunk = std::min<uint32_t>(total_pixels, max_chunk_size);
    }
    delete [] data;

    Deselect();
}

void Screen::FillScreen(const uint16_t colour)
{
    FillRectangle(0, 0, view_width, view_height, colour);
}

void Screen::SetOrientation(Orientation orientation)
{
    switch (orientation)
    {
        case Orientation::portrait:
            view_width = WIDTH;
            view_height = HEIGHT;

            WriteCommand(MAD_CT);
            WriteData(PORTRAIT_DATA);
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
}

void Screen::EnableBackLight()
{
    HAL_GPIO_WritePin(bl.port, bl.pin, GPIO_PIN_SET);
}

void Screen::DisableBackLight()
{
    HAL_GPIO_WritePin(bl.port, bl.pin, GPIO_PIN_RESET);
}

void Screen::ReleaseSPI()
{
    spi_busy = 0;
}

/***** PRIVATE FUNCTIONS *****/

void Screen::Clip(const uint16_t x_start,
                  const uint16_t y_start,
                  uint16_t &x_end,
                  uint16_t &y_end)
{
    if (x_start + x_end - 1 >= view_width) x_end = view_width - x_start;
    if (y_start + y_end - 1 >= view_height) y_end = view_height - y_start;
}

// Note - The select() and deselect() should be called before
//        and after invoking this function, respectively.
void Screen::DrawCharacter(uint16_t x_start,
                           uint16_t y_start,
                           const uint16_t x_window_begin,
                           const uint16_t y_window_begin,
                           const uint16_t x_window_end,
                           const uint16_t y_window_end,
                           const char ch,
                           const Font &font,
                           const uint16_t fg,
                           const uint16_t bg)
{
    // Clip to the window
    uint16_t x_end = x_start + font.width;
    uint16_t y_end = y_start + font.height;

    if (x_start < x_window_begin) x_start = x_window_begin+1;
    if (y_start < y_window_begin) y_start = y_window_begin+1;
    if (x_end > x_window_end) x_end = x_window_end;
    if (y_end > y_window_end) y_end = y_window_end;

    SetWritablePixels(x_start, y_start, x_end - 1, y_end - 1);

    // Get the offset based on the font height and width
    // If the width > 8 we need to account for the extra bytes
    uint32_t offset = (ch-32) * font.height * (font.width/8 + 1);

    // Get the address for the start of the character
    const uint8_t *ch_ptr = &font.data[offset];

    uint8_t data[2]; // The pixel data
    uint16_t idx_h;  // height index
    uint16_t idx_w;  // Width index

    HAL_GPIO_WritePin(dc.port, dc.pin, GPIO_PIN_SET);
    for (idx_h = 0; idx_h < font.height; idx_h++)
    {
        // If the current row is *above* the window skip the whole row
        if (y_start + idx_h < y_window_begin)
        {
            // Add how many bytes we would be skipping for the next row
            // ex font.width = 15, therefore, it takes up 2 bytes, so we need
            // to skip by 2 bytes to get the next row.
            uint8_t num_bytes = 1 + static_cast<uint8_t>(font.width / 8);
            ch_ptr += num_bytes;
            continue;
        }

        // Going past the end of the window can just skip the remaining drawing
        if (y_start + idx_h > y_window_end) break;

        for (idx_w = 0; idx_w < font.width; idx_w++)
        {
            if (x_start + idx_w >= x_window_begin &&
                x_start + idx_w <= x_window_end)
            {
                // Go through each bit in the data and shift it over by the
                // current width index to get if the pixel is activated or not
                // Important to modulus the idx so we don't shift past a byte on
                // the next byte
                if ((*ch_ptr << (idx_w % 8)) & 0x80)
                {
                    // Draw character fg
                    data[0] = static_cast<uint8_t>(fg >> 8);
                    data[1] = fg;
                    WriteDataDMA(data, 2);
                }
                else
                {
                    // Draw character bg
                    data[0] = static_cast<uint8_t>(bg >> 8);
                    data[1] = bg;
                    WriteDataDMA(data, 2);
                }
            }

            // At the end of the byte for a pixel's width
            // slide the ptr to the next byte
            if (idx_w % 8 == 7)
            {
                ch_ptr++;
            }
        }

        // Slide the pointer over to the next byte
        ch_ptr++;
    }
}

uint16_t Screen::ViewWidth() const
{
    return view_width;
}

uint16_t Screen::ViewHeight() const
{
    return view_height;
}