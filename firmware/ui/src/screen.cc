#include <algorithm>
#include "helper.hh"
#include "screen.hh"
#include "ring_buffer.hh"
#include "app_main.hh"
#include <vector>
#include <math.h>
#include <memory.h>
// For some reason M_PI is not defined when including math.h even though it
// should be. Therefore, we need to do it ourselves.

#ifndef  M_PI
#define  M_PI  3.1415926535897932384626433
#endif


Screen::Screen(SPI_HandleTypeDef& hspi,
    port_pin cs,
    port_pin dc,
    port_pin rst,
    port_pin bl,
    Orientation _orientation):
    spi_handle(&hspi),
    cs(cs),
    dc(dc),
    rst(rst),
    bl(bl),
    orientation(_orientation),
    view_height(0),
    view_width(0),
    video_buff(1024),
    video_write_buff(nullptr),
    memories({ 0 }),
    live_memory(memories),
    memories_write_idx(0),
    memories_read_idx(0),
    free_memories(Num_Memories)
{
}

Screen::~Screen()
{
}

/*****************************************************************************/
/***************** Begin General Interfacing functions ***********************/
/*****************************************************************************/

void Screen::Begin()
{
    // Ensure the spi is initialize at this point
    if (spi_handle == nullptr)
    {
        return;
    }

    // Setup GPIO for the screen.
    GPIO_InitTypeDef GPIO_InitStruct = {};

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

void Screen::Update(uint32_t current_tick)
{
    HandleReadyMemory();
}

void Screen::Draw()
{
    HandleVideoBuffer();
}


/*****************************************************************************/
/***************** Begin public command functions ****************************/
/*****************************************************************************/

void Screen::DisableBackLight()
{
    HAL_GPIO_WritePin(bl.port, bl.pin, GPIO_PIN_RESET);
}

void Screen::EnableBackLight()
{
    HAL_GPIO_WritePin(bl.port, bl.pin, GPIO_PIN_SET);
}

void Screen::Reset()
{
    HAL_GPIO_WritePin(rst.port, rst.pin, GPIO_PIN_RESET);
    HAL_Delay(50);
    HAL_GPIO_WritePin(rst.port, rst.pin, GPIO_PIN_SET);
}

void Screen::SetOrientation(Orientation _orientation)
{
    switch (_orientation)
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
    orientation = orientation;
}


void Screen::Sleep()
{
    // TODO
    WriteCommand(0x10);
}

void Screen::Wake()
{
    // TODO
    WriteCommand(0x11);
}

/*****************************************************************************/
/*************************** Begin sync functions ****************************/
/*****************************************************************************/



void Screen::DrawArrowBlocking(const uint16_t tip_x, const uint16_t tip_y,
    const uint16_t length, const uint16_t width,
    const Screen::ArrowDirection direction, const uint16_t colour)
{
    WaitUntilSPIFree();

    const uint16_t half_width = width / 2;
    const uint16_t quar_width = width / 4;
    const uint16_t tip_len = (length * 4) / 10;
    if (direction == ArrowDirection::Left)
    {
        // Left
        uint16_t points[7][2] = { {uint16_t(tip_x), uint16_t(tip_y)},
                                  {uint16_t(tip_x + tip_len), uint16_t(tip_y - half_width)},
                                  {uint16_t(tip_x + tip_len), uint16_t(tip_y - quar_width)},
                                  {uint16_t(tip_x + length), uint16_t(tip_y - quar_width)},
                                  {uint16_t(tip_x + length), uint16_t(tip_y + quar_width)},
                                  {uint16_t(tip_x + tip_len), uint16_t(tip_y + quar_width)},
                                  {uint16_t(tip_x + tip_len), uint16_t(tip_y + half_width)}
        };
        DrawPolygonBlocking(7, points, colour);

    }
    else if (direction == ArrowDirection::Up)
    {
        // Up
        uint16_t points[7][2] = { {uint16_t(tip_x), uint16_t(tip_y)},
                                  {uint16_t(tip_x + half_width), uint16_t(tip_y + tip_len)},
                                  {uint16_t(tip_x + quar_width), uint16_t(tip_y + tip_len)},
                                  {uint16_t(tip_x + quar_width), uint16_t(tip_y + length)},
                                  {uint16_t(tip_x - quar_width), uint16_t(tip_y + length)},
                                  {uint16_t(tip_x - quar_width), uint16_t(tip_y + tip_len)},
                                  {uint16_t(tip_x - half_width), uint16_t(tip_y + tip_len)}
        };
        DrawPolygonBlocking(7, points, colour);
    }
    else if (direction == ArrowDirection::Right)
    {
        // Right
        uint16_t points[7][2] = { {uint16_t(tip_x), uint16_t(tip_y)},
                                  {uint16_t(tip_x - tip_len), uint16_t(tip_y - half_width)},
                                  {uint16_t(tip_x - tip_len), uint16_t(tip_y - quar_width)},
                                  {uint16_t(tip_x - length), uint16_t(tip_y - quar_width)},
                                  {uint16_t(tip_x - length), uint16_t(tip_y + quar_width)},
                                  {uint16_t(tip_x - tip_len), uint16_t(tip_y + quar_width)},
                                  {uint16_t(tip_x - tip_len), uint16_t(tip_y + half_width)}
        };
        DrawPolygonBlocking(7, points, colour);
    }
    else if (direction == ArrowDirection::Down)
    {
        // Down
        uint16_t points[7][2] = { {uint16_t(tip_x), uint16_t(tip_y)},
                                  {uint16_t(tip_x - half_width), uint16_t(tip_y - tip_len)},
                                  {uint16_t(tip_x - quar_width), uint16_t(tip_y - tip_len)},
                                  {uint16_t(tip_x - quar_width), uint16_t(tip_y - length)},
                                  {uint16_t(tip_x + quar_width), uint16_t(tip_y - length)},
                                  {uint16_t(tip_x + quar_width), uint16_t(tip_y - tip_len)},
                                  {uint16_t(tip_x + half_width), uint16_t(tip_y - tip_len)}
        };

        DrawPolygonBlocking(7, points, colour);
    }
}

void Screen::DrawHorizontalLineBlocking(const uint16_t x1, const uint16_t x2,
    const uint16_t y, const uint16_t thickness,
    const uint16_t colour)
{
    if (x2 <= x1) return;
    if (thickness == 0) return;

    FillRectangleBlocking(x1, y, x2, y + thickness, colour);
}

void Screen::DrawLineBlocking(uint16_t x1, uint16_t y1,
    uint16_t x2, uint16_t y2,
    const uint16_t colour)
{
    // Bresenham line algorithm
    // https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm

    // Get the absolute difference
    int16_t diff_x = (x2 >= x1) ? x2 - x1 : -(x2 - x1);
    int16_t diff_y = (y2 >= y1) ? y2 - y1 : -(y2 - y1);

    // If x1 > x2 negative slope
    int16_t direction_x = (x1 < x2) ? 1 : -1;

    // If y2 > y1 negative slope
    int16_t direction_y = (y1 < y2) ? 1 : -1;

    // Get the error on our differences
    int16_t error = diff_x - diff_y;

    // Error will hop back and forth until we hit our end points
    while (x1 != x2 || y1 != y2)
    {
        DrawPixelBlocking(x1, y1, colour);

        // Move x position
        if (error * 2 > -diff_y)
        {
            error -= diff_y;
            x1 += direction_x;
        }
        else
        {
            // Move x position
            error += diff_x;
            y1 += direction_y;
        }
    }
}

void Screen::DrawPixelBlocking(const uint16_t x, const uint16_t y, const uint16_t colour)
{
    if (x >= view_width || y >= view_height) return;
    Select();

    SetWritablePixels(x, y, x, y);

    uint8_t data[2] = { static_cast<uint8_t>(colour >> 8),
                        static_cast<uint8_t>(colour) };

    // Draw pixel
    WriteDataSyncDMA(data, 2);

    Deselect();
}

void Screen::DrawPolygonBlocking(const size_t count,
    const uint16_t points [][2],
    const uint16_t colour)
{
    if (count < 3)
    {
        // A polygon is defined by 3 points
        return;
    }

    const uint8_t x = 0;
    const uint8_t y = 1;

    for (size_t i = 0; i < count - 1; ++i)
    {
        DrawLineBlocking(points[i][x], points[i][y],
            points[i + 1][x], points[i + 1][y], colour);
    }
    // Connect the final line
    DrawLineBlocking(points[count - 1][x], points[count - 1][y],
        points[0][x], points[0][y], colour);
}

void Screen::FillPolygonBlocking(const size_t count,
    const int16_t points [][2],
    const uint16_t colour)
{
    if (count < 3)
    {
        // A polygon is defined by 3 points
        return;
    }

    const uint8_t x = 0;
    const uint8_t y = 1;

    // Find the lowest and greatest pixel
    uint16_t polygon_left = points[count - 1][x];
    uint16_t polygon_right = points[count - 1][x];
    uint16_t polygon_top = points[count - 1][y];
    uint16_t polygon_bottom = points[count - 1][y];

    uint16_t p1_x;
    uint16_t p1_y;
    for (size_t i = 0; i < count - 1; ++i)
    {
        p1_x = points[i][x];
        p1_y = points[i][y];
        if (p1_x < polygon_left)
            polygon_left = p1_x;
        if (p1_x > polygon_right)
            polygon_right = p1_x;
        if (p1_y < polygon_top)
            polygon_top = p1_y;
        if (p1_y > polygon_bottom)
            polygon_bottom = p1_y;

        DrawLineBlocking(points[i][x], points[i][y],
            points[i + 1][x], points[i + 1][y], colour);
    }
    // Connect the final line
    DrawLineBlocking(points[count - 1][x], points[count - 1][y],
        points[0][x], points[0][y], colour);

    // X intersections
    uint16_t* intersections = new uint16_t[count]{ 0 };
    uint8_t num_intersect = 0;
    uint16_t curr_point = 0;
    uint16_t next_point = 0;

    uint16_t i;
    uint16_t j;
    uint16_t tmp;

    /** Scan line fill algorithm *
     * For each row of pixels that are in the upper and lower bounding box of
     * the polygon, send a ray cast through the polygon and
     * calculate the x intersection using the point-slope formulas
     *
     * Add the x intersections to an array and sort the list using
     * insertion sort
     *
     * Draw the line of pixels
    */

    // Loop through the rows of the polygon
    for (uint16_t pix_y = polygon_top; pix_y < polygon_bottom; ++pix_y)
    {
        // Get every x intersection
        num_intersect = 0;

        // Start at the end point
        next_point = count - 1;

        // Cycle around the points
        for (curr_point = 0; curr_point < count; ++curr_point)
        {
            // Check for intersection
            if ((points[curr_point][y] < (int16_t)pix_y && points[next_point][y] >= (int16_t)pix_y)
                || (points[next_point][y] < (int16_t)pix_y && points[curr_point][y] >= (int16_t)pix_y))
            {
                // Get the point of intersection using point slope
                // m = (y2 - y1) / (x2 - x1)
                // y - y1 = m(x - x1)
                // y = pix_y
                // find x
                intersections[num_intersect++] = (float)points[curr_point][x]
                    + (float)(pix_y - points[curr_point][y])
                    / (float)(points[next_point][y] - points[curr_point][y])
                    * (float)(points[next_point][x] - points[curr_point][x]);
            }

            // Swap around the current point index
            next_point = curr_point;
        }

        // Sort the intersections with insertion sort
        i = 1;
        while (i < num_intersect)
        {
            j = i;
            while (j > 0 && intersections[j - 1] > intersections[j])
            {
                tmp = intersections[j];
                intersections[j] = intersections[j - 1];
                intersections[j - 1] = tmp;
                --j;
            }
            ++i;
        }

        // Fill in the spaces between 2 intersections
        for (i = 0; i < num_intersect; i += 2)
        {
            DrawHorizontalLineBlocking(intersections[i],
                intersections[i + 1], pix_y, 1, colour);
        }

    }
    delete [] intersections;
}

void Screen::DrawRectangleBlocking(const uint16_t x_start, const uint16_t y_start,
    const uint16_t x_end, const uint16_t y_end,
    const uint16_t thickness, const uint16_t colour)
{
    // Fill top
    FillRectangleBlocking(x_start, y_start, x_end, y_start + thickness, colour);

    // Fill left
    FillRectangleBlocking(x_start, y_start, x_start + thickness, y_end, colour);

    // Fill right
    FillRectangleBlocking(x_end - thickness, y_start, x_end, y_end, colour);

    // Fill bottom
    FillRectangleBlocking(x_start, y_end - thickness, x_end, y_end, colour);
}

void Screen::DrawBlockAnimateStringBlocking(const uint16_t x, const uint16_t y,
    const std::string& str, const Font& font,
    const uint16_t fg, const uint16_t bg,
    const uint16_t delay)
{
    WaitUntilSPIFree();

    uint16_t x_pos = x;
    uint16_t x_end = x + font.width;
    uint16_t y_end = y + font.height;
    for (unsigned int i = 0; i < str.length(); i++)
    {
        // Fill in a rectangle
        FillRectangleBlocking(x_pos, y, x_end, y_end, fg);

        HAL_Delay(delay);

        // Clear the rectangle we drew
        FillRectangleBlocking(x_pos, y, x_end, y_end, bg);

        std::string letter;
        letter.push_back(str[i]);
        // Draw the string finally
        DrawTextBlocking(x_pos, y, letter, font, fg, bg);

        x_pos += font.width;
        x_end += font.width;
    }
}

void Screen::DrawTextBlocking(const uint16_t x, const uint16_t y, const std::string& str,
    const Font& font, const uint16_t fg, const uint16_t bg,
    const bool wordwrap, uint32_t max_chunk_size)
{
    UNUSED(wordwrap);

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

    // TODO move to using the chunk buffer instead of creating an array
    // during runtime.
    // Get the chunk size
    const uint32_t chunk = std::min<uint32_t>(width * height, max_chunk_size);
    uint32_t data_idx = 0;
    uint8_t* data = new uint8_t[chunk * 2];

    uint32_t ch_idx = 0;
    uint32_t ch_idx_offset = 0;
    char ch;
    const uint8_t* ch_ptr = nullptr;
    uint32_t ptr_offset = 0;
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
                    ptr_offset = ((ch - 32) * font.height * (font.width / 8 + 1))
                        + (h_idx % font.height * (font.width / 8 + 1));

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
                    WriteDataSyncDMA(data, data_idx);
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
            WriteDataSyncDMA(data, data_idx);
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

void Screen::DrawTextboxBlocking(uint16_t x_pos,
    uint16_t y_pos,
    const uint16_t x_window_start,
    const uint16_t y_window_start,
    const uint16_t x_window_end,
    const uint16_t y_window_end,
    const std::string& str,
    const Font& font,
    const uint16_t fg,
    const uint16_t bg)
{
    std::vector<std::string> words;

    // Find each word in put into a vector
    bool found_space = false;
    char ch;
    uint16_t sz = str.length();
    std::string word;
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
            DrawCharacterBlocking(x_pos, y_pos, x_window_start, y_window_start,
                x_window_end, y_window_end, ch, font, fg, bg);
            x_pos += font.width;
        }
    }

    Deselect();
}

void Screen::DrawTriangleBlocking(const uint16_t x1, const uint16_t y1,
    const uint16_t x2, const uint16_t y2,
    const uint16_t x3, const uint16_t y3,
    const uint16_t colour)
{
    uint16_t points[3][2] = { { x1, y1 }, {x2, y2}, {x3, y3} };
    DrawPolygonBlocking(3, points, colour);
}

void Screen::FillArrowBlocking(const uint16_t tip_x, const uint16_t tip_y,
    const uint16_t length, const uint16_t width,
    const Screen::ArrowDirection direction, const uint16_t colour)
{

    const uint16_t half_width = width / 2;
    const uint16_t quar_width = width / 4;
    const uint16_t tip_len = (length * 4) / 10;
    if (direction == ArrowDirection::Left)
    {
        // Left
        int16_t points[7][2] =
        {
            {(int16_t)tip_x, (int16_t)tip_y},
            {(int16_t)(tip_x + tip_len), (int16_t)(tip_y - half_width)},
            {(int16_t)(tip_x + tip_len), (int16_t)(tip_y - quar_width)},
            {(int16_t)(tip_x + length),  (int16_t)(tip_y - quar_width)},
            {(int16_t)(tip_x + length),  (int16_t)(tip_y + quar_width)},
            {(int16_t)(tip_x + tip_len), (int16_t)(tip_y + quar_width)},
            {(int16_t)(tip_x + tip_len), (int16_t)(tip_y + half_width)}
        };
        FillPolygonBlocking(7, points, colour);

    }
    else if (direction == ArrowDirection::Up)
    {
        // Up
        int16_t points[7][2] =
        {
            {(int16_t)tip_x, (int16_t)tip_y},
            {(int16_t)(tip_x + half_width), (int16_t)(tip_y + tip_len)},
            {(int16_t)(tip_x + quar_width), (int16_t)(tip_y + tip_len)},
            {(int16_t)(tip_x + quar_width), (int16_t)(tip_y + length)},
            {(int16_t)(tip_x - quar_width), (int16_t)(tip_y + length)},
            {(int16_t)(tip_x - quar_width), (int16_t)(tip_y + tip_len)},
            {(int16_t)(tip_x - half_width), (int16_t)(tip_y + tip_len)}
        };
        FillPolygonBlocking(7, points, colour);
    }
    else if (direction == ArrowDirection::Right)
    {
        // Right
        int16_t points[7][2] =
        {
            {(int16_t)tip_x, (int16_t)tip_y},
            {(int16_t)(tip_x - tip_len), (int16_t)(tip_y - half_width)},
            {(int16_t)(tip_x - tip_len), (int16_t)(tip_y - quar_width)},
            {(int16_t)(tip_x - length),  (int16_t)(tip_y - quar_width)},
            {(int16_t)(tip_x - length),  (int16_t)(tip_y + quar_width)},
            {(int16_t)(tip_x - tip_len), (int16_t)(tip_y + quar_width)},
            {(int16_t)(tip_x - tip_len), (int16_t)(tip_y + half_width)}
        };
        FillPolygonBlocking(7, points, colour);
    }
    else if (direction == ArrowDirection::Down)
    {
        // Down
        int16_t points[7][2] =
        {
            {(int16_t)tip_x, (int16_t)tip_y},
            {(int16_t)(tip_x - half_width), (int16_t)(tip_y - tip_len)},
            {(int16_t)(tip_x - quar_width), (int16_t)(tip_y - tip_len)},
            {(int16_t)(tip_x - quar_width), (int16_t)(tip_y - length)},
            {(int16_t)(tip_x + quar_width), (int16_t)(tip_y - length)},
            {(int16_t)(tip_x + quar_width), (int16_t)(tip_y - tip_len)},
            {(int16_t)(tip_x + half_width), (int16_t)(tip_y - tip_len)}
        };
        FillPolygonBlocking(7, points, colour);
    }
}

void Screen::FillCircleBlocking(const uint16_t x, const uint16_t y, const uint16_t r,
    const uint16_t fg, const uint16_t bg)
{
    // TODO error checking

    // Create the bounding box
    int16_t left = x - r;
    int16_t right = x + r;
    int16_t top = y - r;
    int16_t bottom = y + r;

    // printf("left = %d, right = %d, top = %d, bottom = %d", left, right, top, bottom);

    if (left < 0)
    {
        left = 0;
    }
    if (right > ViewWidth())
    {
        right = ViewWidth();
    }
    if (top < 0)
    {
        top = 0;
    }
    if (bottom > ViewHeight())
    {
        bottom = ViewHeight();
    }

    // Right now it is limited to 8 pixels per row
    // Scan through the circle
    const int16_t r_2 = r * r;
    const uint16_t cols = right - left + 1;
    const uint16_t rows = bottom - top + 1;

    // Get the number of bytes required to draw
    // TODO this math is wrong fix it.
    const size_t num_bytes = (rows * cols) * 2;
    uint8_t bytes[num_bytes] = { 0 };
    size_t idx;
    uint16_t row;
    uint16_t col;
    for (row = 0; row < rows; ++row)
    {
        for (col = 0; col < cols; ++col)
        {
            const uint16_t h_dis = ((left + col) - x) * ((left + col) - x);
            const uint16_t v_dis = ((top + row) - y) * ((top + row) - y);
            if (h_dis + v_dis <= r_2)
            {
                DrawPixelBlocking(left + col, top + row, fg);
                // Point falls in the circle
                // bytes[idx++] = static_cast<uint8_t>(fg >> 8);
                // bytes[idx++] = static_cast<uint8_t>(fg);
            }
            else
            {
                DrawPixelBlocking(left + col, top + row, bg);
                // bytes[idx++] = static_cast<uint8_t>(bg >> 8);
                // bytes[idx++] = static_cast<uint8_t>(bg);
            }
        }
    }
    // WaitUntilSPIFree();
    // Select();

    // SetWritablePixels(left, top, right - 1, bottom - 1);

    // HAL_GPIO_WritePin(dc.port, dc.pin, GPIO_PIN_SET);
    // WriteDataSyncDMA(bytes, num_bytes);

    // Deselect();
}

void Screen::FillRectangleBlocking(const uint16_t x_start,
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

    WaitUntilSPIFree();

    Select();
    SetWritablePixels(x_start, y_start, x_end - 1, y_end - 1);
    HAL_GPIO_WritePin(dc.port, dc.pin, GPIO_PIN_SET);

    uint16_t y_pixels = y_end - y_start;
    uint16_t x_pixels = x_end - x_start;

    uint32_t total_pixels = y_pixels * x_pixels * 2;

    uint32_t chunk = std::min<uint32_t>(total_pixels, video_buff.BufferSize());

    // Get the video buffer
    auto buff = video_buff.GetFront();

    // Copy colour to data
    for (uint32_t i = 0; i < chunk * 2; i += 2)
    {
        buff->data[i] = static_cast<uint8_t>(colour >> 8);
        buff->data[i + 1] = static_cast<uint8_t>(colour);
    }

    while (total_pixels > 0)
    {
        WriteDataSyncDMA(buff->data, chunk * 2);

        total_pixels -= chunk;

        // Get the size of data we are sending that is remaining
        chunk = std::min<uint32_t>(total_pixels, 128);
    }

    Deselect();
}

void Screen::FillTriangleBlocking(const uint16_t x1, const uint16_t y1,
    const uint16_t x2, const uint16_t y2,
    const uint16_t x3, const uint16_t y3,
    const uint16_t colour)
{
    int16_t points[3][2] = { { int16_t(x1), int16_t(y1) },
                             { int16_t(x2), int16_t(y2) },
                             { int16_t(x3), int16_t(y3) }
    };
    FillPolygonBlocking(3, points, colour);
}

void Screen::FillScreen(const uint16_t colour, bool async)
{
    if (async)
    {
        FillRectangleAsync(0, view_width, 0, view_height, colour);
    }
    else
    {
        FillRectangleBlocking(0, 0, view_width, view_height, colour);
    }
}

void Screen::SpiComplete()
{
    video_write_buff->is_ready = false;
}

uint16_t Screen::GetStringWidth(const uint16_t str_len, const Font& font) const
{
    return str_len * font.width;
}

uint16_t Screen::GetStringCenter(const uint16_t str_len, const Font& font) const
{
    return GetStringWidth(str_len, font) / 2;
}

uint16_t Screen::GetStringCenterMargin(const uint16_t str_len, const Font& font) const
{
    return GetStringLeftDistanceFromRightEdge(str_len, font) / 2;
}

uint16_t Screen::GetStringLeftDistanceFromRightEdge(const uint16_t str_len,
    const Font& font) const
{
    return ViewWidth() - GetStringWidth(str_len, font);
}

uint16_t Screen::RGB888ToRGB565(const uint32_t colour)
{
    // Get the 5 most significant bits of r
    uint16_t r = static_cast<uint16_t>((colour & 0xF80000U) >> 8);
    // Get the 6 most significant bits of g
    uint16_t g = static_cast<uint16_t>((colour & 0xFC00U) >> 5);
    // Get the 5 most significant bits of b
    uint16_t b = static_cast<uint16_t>((colour >> 3) & 0x1F);

    return r | g | b;
}

// Note - The select() and deselect() should be called before
//        and after invoking this function, respectively.
void Screen::DrawCharacterBlocking(uint16_t x_start,
    uint16_t y_start,
    const uint16_t x_window_begin,
    const uint16_t y_window_begin,
    const uint16_t x_window_end,
    const uint16_t y_window_end,
    const char ch,
    const Font& font,
    const uint16_t fg,
    const uint16_t bg)
{
    // Clip to the window
    uint16_t x_end = x_start + font.width;
    uint16_t y_end = y_start + font.height;

    if (x_start < x_window_begin) x_start = x_window_begin + 1;
    if (y_start < y_window_begin) y_start = y_window_begin + 1;
    if (x_end > x_window_end) x_end = x_window_end;
    if (y_end > y_window_end) y_end = y_window_end;

    SetWritablePixels(x_start, y_start, x_end - 1, y_end - 1);

    // Get the offset based on the font height and width
    // If the width > 8 we need to account for the extra bytes
    uint32_t offset = (ch - 32) * font.height * (font.width / 8 + 1);

    // Get the address for the start of the character
    const uint8_t* ch_ptr = &font.data[offset];

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
                    WriteDataSyncDMA(data, 2);
                }
                else
                {
                    // Draw character bg
                    data[0] = static_cast<uint8_t>(bg >> 8);
                    data[1] = bg;
                    WriteDataSyncDMA(data, 2);
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

/*****************************************************************************/
/**************************** Async functions ********************************/
/*****************************************************************************/
void Screen::DrawArrowAsync(const uint16_t tip_x, const uint16_t tip_y,
    const uint16_t length, const uint16_t width,
    const uint16_t thickness, const ArrowDirection direction,
    const uint16_t colour)
{
    const uint16_t half_width = width / 2;
    const uint16_t quar_width = width / 4;
    const uint16_t tip_len = (length * 4) / 10;
    if (direction == ArrowDirection::Left)
    {
        // Left
        uint16_t points[7][2] = { {uint16_t(tip_x), uint16_t(tip_y)},
                                  {uint16_t(tip_x + tip_len), uint16_t(tip_y - half_width)},
                                  {uint16_t(tip_x + tip_len), uint16_t(tip_y - quar_width)},
                                  {uint16_t(tip_x + length), uint16_t(tip_y - quar_width)},
                                  {uint16_t(tip_x + length), uint16_t(tip_y + quar_width)},
                                  {uint16_t(tip_x + tip_len), uint16_t(tip_y + quar_width)},
                                  {uint16_t(tip_x + tip_len), uint16_t(tip_y + half_width)}
        };
        DrawPolygonAsync(7, points, thickness, colour);

    }
    else if (direction == ArrowDirection::Up)
    {
        // Up
        uint16_t points[7][2] = { {uint16_t(tip_x), uint16_t(tip_y)},
                                  {uint16_t(tip_x + half_width), uint16_t(tip_y + tip_len)},
                                  {uint16_t(tip_x + quar_width), uint16_t(tip_y + tip_len)},
                                  {uint16_t(tip_x + quar_width), uint16_t(tip_y + length)},
                                  {uint16_t(tip_x - quar_width), uint16_t(tip_y + length)},
                                  {uint16_t(tip_x - quar_width), uint16_t(tip_y + tip_len)},
                                  {uint16_t(tip_x - half_width), uint16_t(tip_y + tip_len)}
        };
        DrawPolygonAsync(7, points, thickness, colour);
    }
    else if (direction == ArrowDirection::Right)
    {
        // Right
        uint16_t points[7][2] = { {uint16_t(tip_x), uint16_t(tip_y)},
                                  {uint16_t(tip_x - tip_len), uint16_t(tip_y - half_width)},
                                  {uint16_t(tip_x - tip_len), uint16_t(tip_y - quar_width)},
                                  {uint16_t(tip_x - length), uint16_t(tip_y - quar_width)},
                                  {uint16_t(tip_x - length), uint16_t(tip_y + quar_width)},
                                  {uint16_t(tip_x - tip_len), uint16_t(tip_y + quar_width)},
                                  {uint16_t(tip_x - tip_len), uint16_t(tip_y + half_width)}
        };
        DrawPolygonAsync(7, points, thickness, colour);
    }
    else if (direction == ArrowDirection::Down)
    {
        // Down
        uint16_t points[7][2] = { {uint16_t(tip_x), uint16_t(tip_y)},
                                  {uint16_t(tip_x - half_width), uint16_t(tip_y - tip_len)},
                                  {uint16_t(tip_x - quar_width), uint16_t(tip_y - tip_len)},
                                  {uint16_t(tip_x - quar_width), uint16_t(tip_y - length)},
                                  {uint16_t(tip_x + quar_width), uint16_t(tip_y - length)},
                                  {uint16_t(tip_x + quar_width), uint16_t(tip_y - tip_len)},
                                  {uint16_t(tip_x + half_width), uint16_t(tip_y - tip_len)}
        };

        DrawPolygonAsync(7, points, thickness, colour);
    }
}

void Screen::DrawCharacterAsync(uint16_t x, uint16_t y, const char ch,
    const Font& font, const uint16_t fg, const uint16_t bg)
{
    const uint16_t x2 = x + font.width;
    const uint16_t y2 = y + font.height;
    const uint16_t offset = (ch - 32) * font.height * (font.width / 8 + 1);
    const uintptr_t ch_addr = (uintptr_t)(font.data + offset);

    ScreenMemory* memory = SetWriteWindowAsync(x, x2, y, y2);

    memory->post_callback = DrawCharacterProcedure;
    memory->parameters[8] = font.width;
    memory->parameters[9] = font.height;
    memory->parameters[10] = ch_addr >> 24;
    memory->parameters[11] = ch_addr >> 16;
    memory->parameters[12] = ch_addr >> 8;
    memory->parameters[13] = ch_addr;
    memory->parameters[14] = fg >> 8;
    memory->parameters[15] = fg;
    memory->parameters[16] = bg >> 8;
    memory->parameters[17] = bg;
}

void Screen::DrawCircleAsync(const uint16_t x, const uint16_t y,
    const uint16_t r, const uint16_t colour)
{
    // Midpoint circle drawing algorithm
    // https://www.geeksforgeeks.org/mid-point-circle-drawing-algorithm/

    // Draw the 4 pixels that will be skipped in the below algorithm
    DrawPixelAsync(x + r, y, colour);
    DrawPixelAsync(x - r, y, colour);
    DrawPixelAsync(x, y + r, colour);
    DrawPixelAsync(x, y - r, colour);

    // Scan through the circle
    const int16_t r_2 = r * r;
    int16_t x_p = r;
    int16_t y_p = 0;
    int16_t p = 1 - r;

    while (x_p > y_p)
    {
        ++y_p;

        if (p <= 0)
        {
            // Mid point is inside of the circle
            p = p + (y_p << 2) + 1;
        }
        else
        {
            // Mid point is outside of the perimeter
            --x_p;
            p = p + (y_p << 2) - (x_p << 2) + 1;
        }

        DrawPixelAsync(x + x_p, y + y_p, colour);
        DrawPixelAsync(x - x_p, y + y_p, colour);
        DrawPixelAsync(x + x_p, y - y_p, colour);
        DrawPixelAsync(x - x_p, y - y_p, colour);
        DrawPixelAsync(x + y_p, y + x_p, colour);
        DrawPixelAsync(x - y_p, y + x_p, colour);
        DrawPixelAsync(x + y_p, y - x_p, colour);
        DrawPixelAsync(x - y_p, y - x_p, colour);
    }
}

void Screen::DrawLineAsync(uint16_t x1, uint16_t x2,
    uint16_t y1, uint16_t y2, const uint16_t thickness, const uint16_t colour)
{
    if (x1 == x2)
    {
        // Vertical line
        uint16_t thick_1 = thickness / 2;
        uint16_t thick_2 = thickness / 2;
        if (2 == thickness)
        {
            thick_1 = 0;
        }
        FillRectangleAsync(x1 - thick_1, x2 + 1 + thick_2, y1, y2, colour);
    }
    else if (y1 == y2)
    {
        // Horizontal line
        uint16_t thick_1 = thickness / 2;
        uint16_t thick_2 = thickness / 2;
        if (2 == thickness)
        {
            thick_1 = 0;
        }
        FillRectangleAsync(x1, x2, y1 - thick_1, y2 + 1 + thick_2, colour);
    }
    else
    {
        // Bresenham line algorithm
        // https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm

        // Get the absolute difference
        int16_t diff_x = (x2 >= x1) ? x2 - x1 : -(x2 - x1);
        int16_t diff_y = (y2 >= y1) ? y2 - y1 : -(y2 - y1);

        // Get the error on our differences
        int16_t error = diff_x - diff_y;

        // If x1 > x2 negative slope
        int16_t direction_x = (x1 < x2) ? 1 : -1;

        // If y2 > y1 negative slope
        int16_t direction_y = (y1 < y2) ? 1 : -1;

        double rise = abs(diff_y * direction_y);
        double run = abs(diff_x * direction_x);

        uint16_t angle = uint16_t(atan(rise / run) * (180 / M_PI));

        if (angle < 45)
        {
            if (x1 > thickness / 2)
            {
                x1 -= thickness / 2;
                x2 -= thickness / 2;
            }
            else
            {
                x2 -= x1;
                x1 = 0;
            }
        }
        else
        {
            if (y1 > thickness / 2)
            {
                y1 -= thickness / 2;
                y2 -= thickness / 2;
            }
            else
            {
                y2 -= y1;
                y1 = 0;
            }
        }

        ScreenMemory* memory = SetWriteWindowAsync(x1, x1 + 1, y1, y1 + 1);

        memory->post_callback = DrawLineAsyncProcedure;
        memory->parameters[8] = diff_x >> 8;
        memory->parameters[9] = diff_x;
        memory->parameters[10] = diff_y >> 8;
        memory->parameters[11] = diff_y;
        memory->parameters[12] = error >> 8;
        memory->parameters[13] = error;
        memory->parameters[14] = colour >> 8;
        memory->parameters[15] = colour;
        memory->parameters[16] = x1 >> 8;
        memory->parameters[17] = x1 & 0xFF;
        memory->parameters[18] = x2 >> 8;
        memory->parameters[19] = x2 & 0xFF;
        memory->parameters[20] = y1 >> 8;
        memory->parameters[21] = y1 & 0xFF;
        memory->parameters[22] = y2 >> 8;
        memory->parameters[23] = y2 & 0xFF;
        memory->parameters[24] = thickness >> 8;
        memory->parameters[25] = thickness;
        memory->parameters[26] = angle >> 8;
        memory->parameters[27] = angle;
    }
}

void Screen::DrawPixelAsync(const uint16_t x, const uint16_t y,
    const uint16_t colour)
{
    if (x >= view_width || y > view_height)
    {
        return;
    }

    ScreenMemory* memory = SetWriteWindowAsync(x, x + 1, y, y + 1);

    memory->post_callback = DrawPixelAsyncProcedure;
    memory->parameters[8] = colour >> 8;
    memory->parameters[9] = colour;
}

void Screen::DrawPolygonAsync(const size_t count, const uint16_t points [][2],
    const uint16_t thickness, const uint16_t colour)
{
    constexpr uint8_t x = 0;
    constexpr uint8_t y = 1;
    // Its simple, since we could have many more polygons than what we can
    // reasonably write at once, we should try to handle the memories
    // if they aren't being sent currently
    size_t i = 0;
    const size_t _count = count - 1;
    while (i < _count)
    {
        DrawLineAsync(points[i][x], points[i + 1][x],
            points[i][y], points[i + 1][y], thickness, colour);
        i++;
    }

    DrawLineAsync(points[_count][x], points[0][x],
        points[_count][y], points[0][y], thickness, colour);
}


void Screen::DrawRectangleAsync(const uint16_t x1, const uint16_t x2,
    const uint16_t y1, const uint16_t y2, const uint16_t thickness,
    const uint16_t colour)
{
    // Top
    FillRectangleAsync(x1, x2, y1, y1 + thickness, colour);

    // Right
    FillRectangleAsync(x2 - thickness, x2, y1, y2, colour);

    // Bottom
    FillRectangleAsync(x1, x2, y2 - thickness, y2, colour);

    // Left
    FillRectangleAsync(x1, x1 + thickness, y1, y2, colour);
}

void Screen::DrawStringAsync(const uint16_t x, const uint16_t y,
    const char* str, const uint16_t len,
    const Font& font, const uint16_t fg,
    const uint16_t bg, const bool word_wrap)
{

    // const uint16_t start_x = x;
    // const uint16_t end_x = view_width;
    // const uint16_t start_y = y;
    // const uint16_t end_y =  view_height;

    // uint16_t x_curr = start_x;
    // uint16_t y_curr = start_y;

    // uint16_t spc_idx = 0;
    // uint16_t word_len = 0;
    // for (int j = 0; j < len; ++j)
    // {
    //     DrawCharacterAsync(x_curr, y_curr, str[j], font, fg, bg);

    //     x_curr += font.width;
    //     if (x_curr >= end_x)
    //     {
    //         break;
    //     }
    // }

    if (word_wrap)
    {
        DrawStringBoxAsync(x, view_width, y, view_height, str, len, font, fg, bg, false);
    }
    else
    {
        DrawStringBoxAsync(x, view_width, y, y + font.height, str, len, font, fg, bg, false);
    }
}

void Screen::DrawStringAsync(const uint16_t x, const uint16_t y,
    const std::string str, const Font& font, const uint16_t fg,
    const uint16_t bg, const bool word_wrap)
{
    DrawStringAsync(x, y, str.c_str(), str.length(), font, fg, bg, word_wrap);
}

void Screen::DrawStringBoxAsync(const uint16_t x1, const uint16_t x2,
    const uint16_t y1, const uint16_t y2,
    const char* str, const uint16_t len,
    const Font& font, const uint16_t fg, const uint16_t bg,
    const bool draw_box)
{
    if (x1 + font.width > x2 || y1 + font.height > y2)
    {
        return;
    }

    if (draw_box)
    {
        DrawRectangleAsync(x1, x2, y1, y2, 1, fg);
    }

    const uint16_t start_x = draw_box ? x1 + 1 : x1;
    const uint16_t end_x = draw_box ? x2 - 1 : x2;
    const uint16_t start_y = draw_box ? y1 + 1 : y1;
    const uint16_t end_y = draw_box ? y2 - 1 : y2;

    uint16_t x_curr = start_x;
    uint16_t y_curr = start_y;

    uint16_t spc_idx = 0;
    uint16_t word_len = 0;

    uint16_t i = 0;
    uint16_t j = 0;
    while (i < len)
    {
        // Wonder if this could cause issues...?
        if (x_curr == start_x && ' ' == str[i])
        {
            // Draw the space and MOVE ON
            DrawCharacterAsync(x_curr, y_curr, str[j], font, fg, bg);

            x_curr += font.width;
            ++i;
            continue;
        }

        // Find next space
        for (spc_idx = i; spc_idx < len; ++spc_idx)
        {
            if (str[spc_idx] == ' ')
            {
                break;
            }
        }

        word_len = ((spc_idx + 1) - i) * font.width;

        if (word_len + x_curr > end_x)
        {
            // Go to next line
            y_curr += font.height;
            x_curr = start_x;
        }

        for (j = i; j <= spc_idx && j < len; ++j)
        {
            if (x_curr >= end_x)
            {
                x_curr = start_x;
                y_curr += font.height;
                // TODO fix
                if (y_curr + font.height >= end_y)
                {
                    // Force it to end
                    j = len;
                    break;
                }
            }
            DrawCharacterAsync(x_curr, y_curr, str[j], font, fg, bg);

            x_curr += font.width;
        }

        i += (j - i);
    }
}



void Screen::DrawStringBoxAsync(const uint16_t x1, const uint16_t x2,
    const uint16_t y1, const uint16_t y2,
    const std::string str, const Font& font,
    const uint16_t fg, const uint16_t bg,
    const bool draw_box)
{
    DrawStringBoxAsync(x1, x2, y1, y2, str.c_str(), str.length(), font, fg, bg, draw_box);
}

void Screen::DrawTriangleAsync(const uint16_t x1, const uint16_t y1,
    const uint16_t x2, const uint16_t y2,
    const uint16_t x3, const uint16_t y3,
    const uint16_t thickness, const uint16_t colour)
{
    const uint16_t points [][2] = { {x1, y1}, {x2, y2}, {x3, y3} };
    DrawPolygonAsync(3, points, thickness, colour);
}

void Screen::FillArrowAsync(const uint16_t tip_x, const uint16_t tip_y,
    const uint16_t length, const uint16_t width,
    const ArrowDirection direction, const uint16_t colour)
{
    const uint16_t half_width = width / 2;
    const uint16_t quar_width = width / 4;
    const uint16_t tip_len = (length * 4) / 10;
    if (direction == ArrowDirection::Left)
    {
        // Left
        uint16_t points[7][2] = { {uint16_t(tip_x), uint16_t(tip_y)},
                                  {uint16_t(tip_x + tip_len), uint16_t(tip_y - half_width)},
                                  {uint16_t(tip_x + tip_len), uint16_t(tip_y - quar_width)},
                                  {uint16_t(tip_x + length), uint16_t(tip_y - quar_width)},
                                  {uint16_t(tip_x + length), uint16_t(tip_y + quar_width)},
                                  {uint16_t(tip_x + tip_len), uint16_t(tip_y + quar_width)},
                                  {uint16_t(tip_x + tip_len), uint16_t(tip_y + half_width)}
        };
        FillPolygonAsync(7, points, colour);

    }
    else if (direction == ArrowDirection::Up)
    {
        // Up
        uint16_t points[7][2] = { {uint16_t(tip_x), uint16_t(tip_y)},
                                  {uint16_t(tip_x + half_width), uint16_t(tip_y + tip_len)},
                                  {uint16_t(tip_x + quar_width), uint16_t(tip_y + tip_len)},
                                  {uint16_t(tip_x + quar_width), uint16_t(tip_y + length)},
                                  {uint16_t(tip_x - quar_width), uint16_t(tip_y + length)},
                                  {uint16_t(tip_x - quar_width), uint16_t(tip_y + tip_len)},
                                  {uint16_t(tip_x - half_width), uint16_t(tip_y + tip_len)}
        };
        FillPolygonAsync(7, points, colour);
    }
    else if (direction == ArrowDirection::Right)
    {
        // Right
        uint16_t points[7][2] = { {uint16_t(tip_x), uint16_t(tip_y)},
                                  {uint16_t(tip_x - tip_len), uint16_t(tip_y - half_width)},
                                  {uint16_t(tip_x - tip_len), uint16_t(tip_y - quar_width)},
                                  {uint16_t(tip_x - length), uint16_t(tip_y - quar_width)},
                                  {uint16_t(tip_x - length), uint16_t(tip_y + quar_width)},
                                  {uint16_t(tip_x - tip_len), uint16_t(tip_y + quar_width)},
                                  {uint16_t(tip_x - tip_len), uint16_t(tip_y + half_width)}
        };
        FillPolygonAsync(7, points, colour);
    }
    else if (direction == ArrowDirection::Down)
    {
        // Down
        uint16_t points[7][2] = { {uint16_t(tip_x), uint16_t(tip_y)},
                                  {uint16_t(tip_x - half_width), uint16_t(tip_y - tip_len)},
                                  {uint16_t(tip_x - quar_width), uint16_t(tip_y - tip_len)},
                                  {uint16_t(tip_x - quar_width), uint16_t(tip_y - length)},
                                  {uint16_t(tip_x + quar_width), uint16_t(tip_y - length)},
                                  {uint16_t(tip_x + quar_width), uint16_t(tip_y - tip_len)},
                                  {uint16_t(tip_x + half_width), uint16_t(tip_y - tip_len)}
        };

        FillPolygonAsync(7, points, colour);
    }
}

void Screen::FillCircleAsync(const uint16_t x, const uint16_t y, const uint16_t r,
    const uint16_t colour)
{
    const int16_t left = std::max(x - r, 0);
    const int16_t right = std::min<int16_t>(x + r, view_width);
    const int16_t top = std::max(y - r, 0);
    const int16_t bottom = std::min<int16_t>(y + r, view_height);

    const uint16_t cols = right - left + 1;
    const uint16_t rows = (bottom - top + 1) / 2;

    if (cols == 0 || rows == 0)
    {
        return;
    }

    // Scan line algorithm
    // Scan through the circle
    const int16_t r_2 = r * r;
    for (uint16_t row = 0; row < rows; ++row)
    {
        for (uint16_t col = 0; col < cols; ++col)
        {
            const int16_t h_dis = int16_t(left + col - x) * int16_t(left + col - x);
            const int16_t v_dis = int16_t(top + row - y) * int16_t(top + row - y);

            if (h_dis + v_dis <= r_2)
            {
                FillRectangleAsync(left + col, (right - col) + 1, top + row, top + row + 1, colour);
                FillRectangleAsync(left + col, (right - col) + 1, bottom - row, (bottom - row) + 1, colour);
                break;
            }
        }
    }
    FillRectangleAsync(left, right + 1, top + rows, top + rows + 1, colour);
}

void Screen::FillPolygonAsync(const size_t count, const uint16_t points [][2],
    const uint16_t colour)
{
    if (count < 3)
    {
        // A polygon is defined by 3 points
        return;
    }

    const uint16_t x = 0;
    const uint16_t y = 1;

    // Find the y_min and y_max
    const uint16_t _count = count - 1;
    uint16_t y_min = points[_count][y];
    uint16_t y_max = y_min;
    for (size_t i = 0; i < _count; ++i)
    {
        if (points[i][y] > y_max)
        {
            y_max = points[i][y];
        }

        if (points[i][y] < y_min)
        {
            y_min = points[i][y];
        }
        // Draw the polygon while we are at it
        DrawLineAsync(points[i][x], points[i + 1][x],
            points[i][y], points[i + 1][y], 1, colour);
    }
    // Connect the final line
    DrawLineAsync(points[_count][x], points[0][x],
        points[_count][y], points[0][y], 1, colour);


    /** Scan line fill algorithm *
     * For each row of pixels that are in the upper and lower bounding box of
     * the polygon, send a ray cast through the polygon and
     * calculate the x intersection using the point-slope formulas
     *
     * Add the x intersections to an array and sort the list using
     * insertion sort
     *
     * Draw the line of pixels
    */

    uint16_t* intersections = new uint16_t[count];
    uint8_t num_intersect = 0;
    uint16_t curr_point = 0;
    uint16_t next_point = 0;

    uint16_t i;
    uint16_t j;
    uint16_t tmp;

    // Loop through the rows of the polygon
    for (uint16_t pix_y = y_min; pix_y < y_max; ++pix_y)
    {
        // Get every x intersection
        num_intersect = 0;

        // Start at the end point
        next_point = count - 1;

        // Cycle around the points
        for (curr_point = 0; curr_point < count; ++curr_point)
        {
            // Check for intersection
            if ((points[curr_point][y] < (int16_t)pix_y &&
                points[next_point][y] >= (int16_t)pix_y)
                ||
                (points[next_point][y] < (int16_t)pix_y &&
                    points[curr_point][y] >= (int16_t)pix_y))
            {
                // Get the point of intersection using point slope
                // m = (y2 - y1) / (x2 - x1)
                // y - y1 = m(x - x1)
                // y = pix_y
                // find x
                intersections[num_intersect++] = (float)points[curr_point][x]
                    + (float)(pix_y - points[curr_point][y])
                    / (float)(points[next_point][y] - points[curr_point][y])
                    * (float)(points[next_point][x] - points[curr_point][x]);
            }

            // Swap around the current point index
            next_point = curr_point;
        }

        // Sort the intersections with insertion sort
        i = 1;
        while (i < num_intersect)
        {
            j = i;
            while (j > 0 && intersections[j - 1] > intersections[j])
            {
                tmp = intersections[j];
                intersections[j] = intersections[j - 1];
                intersections[j - 1] = tmp;
                --j;
            }
            ++i;
        }

        // Fill in the spaces between 2 intersections
        for (i = 0; i < num_intersect; i += 2)
        {
            DrawLineAsync(intersections[i], intersections[i + 1] + 1,
                pix_y, pix_y, 1, colour);
        }
    }
    delete [] intersections;
}

void Screen::FillTriangleAsync(const uint16_t x1, const uint16_t y1,
    const uint16_t x2, const uint16_t y2,
    const uint16_t x3, const uint16_t y3,
    const uint16_t colour)
{
    const uint16_t points [][2] = { {x1, y1}, {x2, y2}, {x3, y3} };
    FillPolygonAsync(3, points, colour);
}

void Screen::FillRectangleAsync(uint16_t x1,
    uint16_t x2,
    uint16_t y1,
    uint16_t y2,
    const uint16_t colour)
{
    // Clip to the size of the screen
    if (x1 >= view_width || y1 >= view_height)
    {
        return;
    }

    if (x2 >= view_width)
    {
        x2 = view_width;
    }

    if (y2 >= view_height)
    {
        y2 = view_height;
    }

    if (x1 > x2)
    {
        uint16_t tmp = x1;
        x1 = x2;
        x2 = tmp;
    }

    if (y1 > y2)
    {
        uint16_t tmp = y1;
        y1 = y2;
        y2 = tmp;
    }

    ScreenMemory* memory = SetWriteWindowAsync(x1, x2, y1, y2);

    uint32_t num_byte_pixels = ((x2 - x1) * (y2 - y1) * 2);

    memory->post_callback = FillRectangleAsyncProcedure;

    memory->parameters[8] = colour >> 8;
    memory->parameters[9] = colour & 0xFF;

    memory->parameters[10] = num_byte_pixels >> 24;
    memory->parameters[11] = num_byte_pixels >> 16;
    memory->parameters[12] = num_byte_pixels >> 8;
    memory->parameters[13] = num_byte_pixels & 0xFF;
}

/*****************************************************************************/
/**************************** Helper helpers *********************************/
/*****************************************************************************/

uint16_t Screen::ViewWidth() const
{
    return view_width;
}

uint16_t Screen::ViewHeight() const
{
    return view_height;
}

/*****************************************************************************/
/**********************  Private GPIO functions ******************************/
/*****************************************************************************/

inline void Screen::Select()
{
    // Set pin LOW for selection
    HAL_GPIO_WritePin(cs.port, cs.pin, GPIO_PIN_RESET);
}

inline void Screen::Deselect()
{
    // Set the pin to HIGH to deselect
    HAL_GPIO_WritePin(cs.port, cs.pin, GPIO_PIN_SET);
}

/*****************************************************************************/
/********************* Private sync command functions ************************/
/*****************************************************************************/


/**
 * SetWritablePixels
 *
 * Sets the addressable location that can be drawn on the screen, smaller
 * updates are faster
 *
 */
void Screen::SetWritablePixels(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end)
{
    uint8_t col_data [] = { static_cast<uint8_t>(x_start >> 8), static_cast<uint8_t>(x_start),
                           static_cast<uint8_t>(x_end >> 8), static_cast<uint8_t>(x_end) };
    uint8_t row_data [] = { static_cast<uint8_t>(y_start >> 8), static_cast<uint8_t>(y_start),
                           static_cast<uint8_t>(y_end >> 8), static_cast<uint8_t>(y_end) };

    Select();

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

// Note Select needs to be called prior
void Screen::WriteData(uint8_t data)
{
    HAL_GPIO_WritePin(dc.port, dc.pin, GPIO_PIN_SET);
    HAL_SPI_Transmit(spi_handle, &data, 1, HAL_MAX_DELAY);
}

void Screen::WriteDataSyncDMA(uint8_t* data, const uint32_t data_size)
{
    HAL_SPI_Transmit_DMA(spi_handle, data, data_size);

    // Wait for the SPI IT call complete to invoked
    WaitUntilSPIFree();
}

/*****************************************************************************/
/********************* Private async command functions ***********************/
/*****************************************************************************/


Screen::ScreenMemory* Screen::SetWriteWindowAsync(uint16_t x1, uint16_t x2, uint16_t y1, uint16_t y2)
{
    // Get a memory
    ScreenMemory* memory = RetrieveFreeMemory();

    memory->callback = SetColumnsCommandAsync;
    memory->parameters[0] = x1 >> 8;
    memory->parameters[1] = x1 & 0xFF;
    memory->parameters[2] = x2 >> 8;
    memory->parameters[3] = x2 & 0xFF;
    memory->parameters[4] = y1 >> 8;
    memory->parameters[5] = y1 & 0xFF;
    memory->parameters[6] = y2 >> 8;
    memory->parameters[7] = y2 & 0xFF;
    memory->status = MemoryStatus::In_Progress;

    return memory;
}


void Screen::SetColumnsCommandAsync(Screen& screen, ScreenMemory& memory)
{
    memory.callback = SetColumnsDataAsync;

    SwapBuffer::swap_buffer_t* buff = screen.video_buff.GetBack();
    buff->data[0] = CA_SET;
    buff->len = 1;
    buff->dc_level = GPIO_PIN_RESET;
    buff->is_ready = true;
}

void Screen::SetColumnsDataAsync(Screen& screen, ScreenMemory& memory)
{
    memory.callback = SetRowsCommandAsync;

    SwapBuffer::swap_buffer_t* buff = screen.video_buff.GetBack();

    // Minus off 1 x2 because the screen expects a inclusive x1-x2
    buff->dc_level = GPIO_PIN_SET;
    buff->data[0] = memory.parameters[0];
    buff->data[1] = memory.parameters[1];
    buff->data[2] = memory.parameters[2];
    buff->data[3] = memory.parameters[3] - 1;
    buff->len = 4;
    buff->is_ready = true;
}

void Screen::SetRowsCommandAsync(Screen& screen, ScreenMemory& memory)
{
    memory.callback = SetRowsDataAsync;

    SwapBuffer::swap_buffer_t* buff = screen.video_buff.GetBack();

    buff->dc_level = GPIO_PIN_RESET;
    buff->data[0] = RA_SET;
    buff->len = 1;
    buff->is_ready = true;
}

void Screen::SetRowsDataAsync(Screen& screen, ScreenMemory& memory)
{
    memory.callback = WriteToRamCommandAsync;

    SwapBuffer::swap_buffer_t* buff = screen.video_buff.GetBack();

    // Minus off 1 y2 because the screen expects a inclusive y1-y2
    buff->dc_level = GPIO_PIN_SET;
    buff->data[0] = memory.parameters[4];
    buff->data[1] = memory.parameters[5];
    buff->data[2] = memory.parameters[6];
    buff->data[3] = memory.parameters[7] - 1;
    buff->len = 4;
    buff->is_ready = true;
}

void Screen::WriteToRamCommandAsync(Screen& screen, ScreenMemory& memory)
{
    SwapBuffer::swap_buffer_t* buff = screen.video_buff.GetBack();

    buff->dc_level = GPIO_PIN_RESET;
    buff->data[0] = WR_RAM;
    buff->len = 1;
    buff->is_ready = true;

    if (memory.post_callback != nullptr)
    {
        memory.callback = memory.post_callback;
    }
    else
    {
        memory.status = MemoryStatus::Complete;
    }
}

bool Screen::WriteAsync(SwapBuffer::swap_buffer_t* buff)
{
    Select();
    HAL_GPIO_WritePin(dc.port, dc.pin, buff->dc_level);
    auto res = HAL_SPI_Transmit_DMA(spi_handle, buff->data, buff->len);

    if (res == HAL_OK)
    {
        return true;
    }
    else
    {
        return false;
    }
}

Screen::ScreenMemory* Screen::RetrieveFreeMemory()
{
    WaitForFreeMemory();

    if (memories_write_idx >= Num_Memories)
    {
        memories_write_idx = 0;
    }

    --free_memories;
    memories[memories_write_idx].status = MemoryStatus::In_Progress;
    return &memories[memories_write_idx++];
}

void Screen::HandleReadyMemory()
{
    if (video_buff.BackIsReady())
    {
        return;
    }

    // Nothing to do skip
    if (free_memories >= Num_Memories)
    {
        return;
    }

    // Get the current memory if the memory is complete, get the next one etc
    uint32_t memory_count = 0;
    bool got_memory = false;
    ScreenMemory* memory;

    while (memory_count < Num_Memories)
    {
        if (memories_read_idx >= Num_Memories)
        {
            memories_read_idx = 0;
        }

        memory = &memories[memories_read_idx];

        // NOTE- we don't need to check for MemoryStatus::Unused
        // because if there are memories waiting it will always
        // be sequential
        if (memory->status == MemoryStatus::Complete)
        {
            ++memories_read_idx;

            ++free_memories;

            memory->status = MemoryStatus::Unused;

            memset(memory->parameters, 0, Memory_Size);
        }
        else if (memory->status == MemoryStatus::In_Progress)
        {
            got_memory = true;
            break;
        }
        memory_count++;
    }

    if (!got_memory)
    {
        // No live_memory to handle
        return;
    }

    live_memory = memory;


    // Now that we have a live_memory we can call the function and
    // feed itself to the function
    live_memory->callback(*this, *live_memory);
}

void Screen::HandleVideoBuffer()
{
    // Check if the back buffer is ready
    if (spi_handle->State != HAL_SPI_STATE_READY || !video_buff.BackIsReady())
    {
        return;
    }

    // Back is ready, so swap it to the front
    video_write_buff = video_buff.Swap();

    // Send it off
    bool sent = WriteAsync(video_write_buff);

    if (!sent)
    {
        // Buffer was sent, swap the buffers
        video_buff.Swap();
    }
}

/*****************************************************************************/
/********************* Async procedure functions *****************************/
/*****************************************************************************/

void Screen::DrawCharacterProcedure(Screen& screen, ScreenMemory& memory)
{
    uint16_t font_width = memory.parameters[8];
    uint16_t font_height = memory.parameters[9];
    uint8_t* ch_ptr = (uint8_t*)(memory.parameters[10] << 24
        | memory.parameters[11] << 16
        | memory.parameters[12] << 8
        | memory.parameters[13]);

    uint16_t fg = memory.parameters[14] << 8 | memory.parameters[15];
    uint16_t bg = memory.parameters[16] << 8 | memory.parameters[17];

    uint16_t w_off = 0;
    uint16_t buff_idx = 0;

    SwapBuffer::swap_buffer_t* buff = screen.video_buff.GetBack();

    // 2 bytes per pixel
    buff->len = font_height * font_width * 2;
    buff->dc_level = GPIO_PIN_SET;

    for (uint16_t idx_h = 0; idx_h < font_height; ++idx_h)
    {
        w_off = 0;
        for (uint16_t idx_w = 0; idx_w < font_width; ++idx_w)
        {
            // Go through each bit in the data and shift it over by the
            // current width index to get if the pixel is activated or not
            if ((*ch_ptr << w_off) & 0x80)
            {
                buff->data[buff_idx] = fg >> 8;
                buff->data[buff_idx + 1] = fg & 0xFF;
            }
            else
            {
                buff->data[buff_idx] = bg >> 8;
                buff->data[buff_idx + 1] = bg & 0xFF;
            }

            buff_idx += 2;

            ++w_off;
            if (w_off >= 8)
            {
                w_off = 0;

                // At the end of the byte's bits, if a font > 8 bits
                // then we need to slide to the next byte.
                ++ch_ptr;
            }
        }

        // Slide the pointer is the next row
        ++ch_ptr;
    }

    memory.status = MemoryStatus::Complete;
    buff->is_ready = true;
}

void Screen::DrawLineAsyncProcedure(Screen& screen, ScreenMemory& memory)
{
    int16_t diff_x = memory.parameters[8] << 8 | memory.parameters[9];
    int16_t diff_y = memory.parameters[10] << 8 | memory.parameters[11];

    // Error will hop back and forth until we hit our end points
    int16_t error = memory.parameters[12] << 8 | memory.parameters[13];

    uint16_t x1 = memory.parameters[0] << 8 | memory.parameters[1];
    uint16_t x2 = memory.parameters[18] << 8 | memory.parameters[19];
    uint16_t y1 = memory.parameters[4] << 8 | memory.parameters[5];
    uint16_t y2 = memory.parameters[22] << 8 | memory.parameters[23];


    uint16_t thickness = memory.parameters[24] << 8 | memory.parameters[25];

    // If x1 > x2 negative slope
    int16_t direction_x = (x1 < x2) ? 1 : -1;

    // If y2 > y1 negative slope
    int16_t direction_y = (y1 < y2) ? 1 : -1;

    // Fill the array and then update stuff
    SwapBuffer::swap_buffer_t* buff = screen.video_buff.GetBack();
    buff->len = 2;
    buff->dc_level = GPIO_PIN_SET;

    // Colour is stored at these addresses
    buff->data[0] = memory.parameters[14];
    buff->data[1] = memory.parameters[15];

    // Move x position
    if (error * 2 > -diff_y)
    {
        error -= diff_y;
        x1 += direction_x;
    }
    else
    {
        // Move x position
        error += diff_x;
        y1 += direction_y;
    }

    if (x1 != x2 || y1 != y2)
    {
        memory.parameters[0] = x1 >> 8;
        memory.parameters[1] = x1 & 0xFF;
        memory.parameters[2] = (x1 + 1) >> 8;
        memory.parameters[3] = (x1 + 1) & 0xFF;
        memory.parameters[4] = y1 >> 8;
        memory.parameters[5] = y1 & 0xFF;
        memory.parameters[6] = (y1 + 1) >> 8;
        memory.parameters[7] = (y1 + 1) & 0xFF;
        memory.parameters[12] = error >> 8;
        memory.parameters[13] = error;
        memory.callback = SetColumnsCommandAsync;
    }
    else if (thickness > 1)
    {
        x1 = memory.parameters[16] << 8 | memory.parameters[17];
        y1 = memory.parameters[20] << 8 | memory.parameters[21];

        uint16_t angle = memory.parameters[26] << 8 | memory.parameters[27];

        if (angle < 45)
        {
            x1 += 1;
            x2 += 1;
        }
        else
        {
            y1 += 1;
            y2 += 1;
        }

        thickness--;

        memory.parameters[0] = x1 >> 8;
        memory.parameters[1] = x1 & 0xFF;
        memory.parameters[2] = (x1 + 1) >> 8;
        memory.parameters[3] = (x1 + 1) & 0xFF;
        memory.parameters[4] = y1 >> 8;
        memory.parameters[5] = y1 & 0xFF;
        memory.parameters[6] = (y1 + 1) >> 8;
        memory.parameters[7] = (y1 + 1) & 0xFF;
        memory.parameters[12] = error >> 8;
        memory.parameters[13] = error;
        memory.parameters[16] = x1 >> 8;
        memory.parameters[17] = x1 & 0xFF;
        memory.parameters[18] = x2 >> 8;
        memory.parameters[19] = x2 & 0xFF;
        memory.parameters[20] = y1 >> 8;
        memory.parameters[21] = y1 & 0xFF;
        memory.parameters[22] = y2 >> 8;
        memory.parameters[23] = y2 & 0xFF;
        memory.parameters[24] = thickness >> 8;
        memory.parameters[25] = thickness;
        memory.callback = SetColumnsCommandAsync;
    }
    else
    {
        memory.status = MemoryStatus::Complete;
    }
    buff->is_ready = true;
}

void Screen::DrawPixelAsyncProcedure(Screen& screen, ScreenMemory& memory)
{
    SwapBuffer::swap_buffer_t* buff = screen.video_buff.GetBack();
    buff->len = 2;
    buff->data[0] = memory.parameters[8];
    buff->data[1] = memory.parameters[9];
    buff->dc_level = GPIO_PIN_SET;

    memory.status = MemoryStatus::Complete;
    buff->is_ready = true;
}

void Screen::FillRectangleAsyncProcedure(Screen& screen, ScreenMemory& memory)
{
    uint16_t colour = memory.parameters[8] << 8 | memory.parameters[9];

    uint32_t bytes_remaining =
        memory.parameters[10] << 24 |
        memory.parameters[11] << 16 |
        memory.parameters[12] << 8 |
        memory.parameters[13];

    // Get a buff
    SwapBuffer::swap_buffer_t* buff = screen.video_buff.GetBack();
    buff->len = bytes_remaining > screen.video_buff.BufferSize()
        ? screen.video_buff.BufferSize() : bytes_remaining;

    // Fill the buff
    for (size_t i = 0; i < buff->len; i += 2)
    {
        buff->data[i] = colour >> 8;
        buff->data[i + 1] = colour & 0xFF;
    }

    buff->dc_level = GPIO_PIN_SET;
    bytes_remaining -= buff->len;

    // Out of the while loop, if curr_x != x2 and curr_y != y2
    // then save the state, otherwise, finish the memory?
    if (bytes_remaining != 0)
    {
        memory.parameters[10] = bytes_remaining >> 24;
        memory.parameters[11] = bytes_remaining >> 16;
        memory.parameters[12] = bytes_remaining >> 8;
        memory.parameters[13] = bytes_remaining & 0xFF;
    }
    else
    {
        memory.status = MemoryStatus::Complete;
    }
    buff->is_ready = true;
}

/*****************************************************************************/
/*************************** Private helpers *********************************/
/*****************************************************************************/

void Screen::Clip(const uint16_t x_start,
    const uint16_t y_start,
    uint16_t& x_end,
    uint16_t& y_end)
{
    if (x_start + x_end - 1 >= view_width) x_end = view_width - x_start;
    if (y_start + y_end - 1 >= view_height) y_end = view_height - y_start;
}

inline void Screen::WaitUntilSPIFree()
{
    while (spi_handle->State != HAL_SPI_STATE_READY)
    {
        __NOP();
    }
}

void Screen::WaitForFreeMemory(const uint16_t minimum)
{
    const uint16_t min_mem = minimum > 0 ? minimum : 1;

    while (free_memories < min_mem)
    {
        // Since we can't get out of here, lets update the buffer
        // and wait for the timer to handle the issue.

        // TODO If we need to change the buffer so that we set a flag
        // before writing that says "is transferring"
        // and then when the spi dma is done if they are both "done"
        // then we can reset the flag and continue drawing on
        Update(0);
    }
}