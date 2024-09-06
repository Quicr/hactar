#include <algorithm>
#include "Helper.h"
#include "Screen.hh"
#include "RingBuffer.hh"
#include "app_main.hh"
#include <vector>

#define DELAY HAL_MAX_DELAY

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
    chunk_buffer{ 0 },
    spi_busy(0),
    draw_async(0),
    draw_async_stop(0),
    buffer_overwritten_by_sync(0),
    drawing_func_read(0),
    drawing_func_write(0),
    async_draw_ready(0),
    draw_matrix(32),
    Drawing_Func_Ring(new void* [32])
{
}

Screen::~Screen()
{
    delete [] Drawing_Func_Ring;
}

void Screen::Begin()
{
    // Ensure the spi is initialize at this point
    if (spi_handle == nullptr) return;

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
    buffer_overwritten_by_sync = 1;
    HAL_SPI_Transmit_DMA(spi_handle, data, data_size);

    // Wait for the SPI IT call complete to invoked
    WaitUntilSPIFree();
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
    uint8_t col_data [] = { static_cast<uint8_t>(x_start >> 8), static_cast<uint8_t>(x_start),
                           static_cast<uint8_t>(x_end >> 8), static_cast<uint8_t>(x_end) };
    uint8_t row_data [] = { static_cast<uint8_t>(y_start >> 8), static_cast<uint8_t>(y_start),
                           static_cast<uint8_t>(y_end >> 8), static_cast<uint8_t>(y_end) };

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

void Screen::EnableBackLight()
{
    HAL_GPIO_WritePin(bl.port, bl.pin, GPIO_PIN_SET);
}

void Screen::DisableBackLight()
{
    HAL_GPIO_WritePin(bl.port, bl.pin, GPIO_PIN_RESET);
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

void Screen::DrawArrow(const uint16_t tip_x, const uint16_t tip_y,
    const uint16_t length, const uint16_t width,
    const Screen::ArrowDirection direction, const uint16_t colour)
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
        DrawPolygon(7, points, colour);

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
        DrawPolygon(7, points, colour);
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
        DrawPolygon(7, points, colour);
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

        DrawPolygon(7, points, colour);
    }
}

void Screen::DrawHorizontalLine(const uint16_t x1, const uint16_t x2,
    const uint16_t y, const uint16_t thickness,
    const uint16_t colour)
{
    if (x2 <= x1) return;
    if (thickness == 0) return;

    FillRectangle(x1, y, x2, y + thickness, colour);
}


void Screen::DrawLine(uint16_t x1, uint16_t y1,
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
        DrawPixel(x1, y1, colour);

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

void Screen::DrawPixel(const uint16_t x, const uint16_t y, const uint16_t colour)
{
    if (x >= view_width || y >= view_height) return;
    Select();

    SetWritablePixels(x, y, x, y);

    uint8_t data[2] = { static_cast<uint8_t>(colour >> 8),
                        static_cast<uint8_t>(colour) };

    // Draw pixel
    WriteDataDMA(data, 2);

    Deselect();
}

void Screen::DrawPolygon(const size_t count,
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
        DrawLine(points[i][x], points[i][y],
            points[i + 1][x], points[i + 1][y], colour);
    }
    // Connect the final line
    DrawLine(points[count - 1][x], points[count - 1][y],
        points[0][x], points[0][y], colour);
}

void Screen::FillPolygon(const size_t count,
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

        DrawLine(points[i][x], points[i][y],
            points[i + 1][x], points[i + 1][y], colour);
    }
    // Connect the final line
    DrawLine(points[count - 1][x], points[count - 1][y],
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
            DrawHorizontalLine(intersections[i],
                intersections[i + 1], pix_y, 1, colour);
        }

    }
    delete [] intersections;
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

// TODO consider adding x2 and y2 so it has a bounding box.
void Screen::DrawBlockAnimateString(const uint16_t x, const uint16_t y,
    const std::string& str, const Font& font,
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

        std::string letter;
        letter.push_back(str[i]);
        // Draw the string finally
        DrawText(x_pos, y, letter, font, fg, bg);

        x_pos += font.width;
        x_end += font.width;
    }
}

void Screen::DrawText(const uint16_t x, const uint16_t y, const std::string& str,
    const Font& font, const uint16_t fg, const uint16_t bg,
    const bool wordwrap, uint32_t max_chunk_size)
{
    // TODO
    UNUSED(wordwrap);

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
            DrawCharacter(x_pos, y_pos, x_window_start, y_window_start,
                x_window_end, y_window_end, ch, font, fg, bg);
            x_pos += font.width;
        }
    }

    Deselect();
}

void Screen::DrawTriangle(const uint16_t x1, const uint16_t y1,
    const uint16_t x2, const uint16_t y2,
    const uint16_t x3, const uint16_t y3,
    const uint16_t colour)
{
    uint16_t points[3][2] = { { x1, y1 }, {x2, y2}, {x3, y3} };
    DrawPolygon(3, points, colour);
}

void Screen::FillArrow(const uint16_t tip_x, const uint16_t tip_y,
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
        FillPolygon(7, points, colour);

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
        FillPolygon(7, points, colour);
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
        FillPolygon(7, points, colour);
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
        FillPolygon(7, points, colour);
    }
}

void Screen::FillCircle(const uint16_t x, const uint16_t y, const uint16_t r,
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
                DrawPixel(left + col, top+row, fg);
                // Point falls in the circle
                // bytes[idx++] = static_cast<uint8_t>(fg >> 8);
                // bytes[idx++] = static_cast<uint8_t>(fg);
            }
            else
            {
                DrawPixel(left + col, top+row, bg);
                // bytes[idx++] = static_cast<uint8_t>(bg >> 8);
                // bytes[idx++] = static_cast<uint8_t>(bg);
            }
        }
    }
    // WaitUntilSPIFree();
    // Select();

    // SetWritablePixels(left, top, right - 1, bottom - 1);

    // HAL_GPIO_WritePin(dc.port, dc.pin, GPIO_PIN_SET);
    // WriteDataDMA(bytes, num_bytes);

    // Deselect();
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

    WaitUntilSPIFree();

    Select();
    SetWritablePixels(x_start, y_start, x_end - 1, y_end - 1);
    HAL_GPIO_WritePin(dc.port, dc.pin, GPIO_PIN_SET);

    uint16_t y_pixels = y_end - y_start;
    uint16_t x_pixels = x_end - x_start;

    uint32_t total_pixels = y_pixels * x_pixels;

    uint32_t chunk = std::min<uint32_t>(total_pixels, 128);

    // Copy colour to data
    for (uint32_t i = 0; i < chunk * 2; i += 2)
    {
        chunk_buffer[i] = static_cast<uint8_t>(colour >> 8);
        chunk_buffer[i + 1] = static_cast<uint8_t>(colour);
    }

    while (total_pixels > 0)
    {
        WriteDataDMA(chunk_buffer, chunk * 2);

        total_pixels -= chunk;

        // Get the size of data we are sending that is remaining
        chunk = std::min<uint32_t>(total_pixels, 128);
    }

    Deselect();
}

void Screen::FillTriangle(const uint16_t x1, const uint16_t y1,
    const uint16_t x2, const uint16_t y2,
    const uint16_t x3, const uint16_t y3,
    const uint16_t colour)
{
    int16_t points[3][2] = { { int16_t(x1), int16_t(y1) },
                             { int16_t(x2), int16_t(y2) },
                             { int16_t(x3), int16_t(y3) }
    };
    FillPolygon(3, points, colour);
}

void Screen::FillScreen(const uint16_t colour, bool async)
{
    if (async)
    {
        FillRectangleAsync(0, 0, view_width, view_height, colour);
    }
    else
    {
        FillRectangle(0, 0, view_width, view_height, colour);
    }
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

void Screen::ReleaseSPI()
{
    spi_busy = 0;

    if (draw_async)
    {
        draw_async = 0;
        async_draw_ready = 1;
        Deselect();
    }

    if (draw_async_stop)
    {
        // Give time for the ili9341 to finish drawing
        draw_matrix.Swap();
        draw_async_stop = 0;
        if (++drawing_func_read >= 8)
        {
            drawing_func_read = 0;
        }

        if (Drawing_Func_Ring[drawing_func_read] != nullptr)
        {
            async_draw_ready = 1;
        }
        else
        {
            draw_async = 0;
            async_draw_ready = 0;
            Deselect();
        }
    }
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

// Note - The select() and deselect() should be called before
//        and after invoking this function, respectively.
void Screen::DrawCharacter(uint16_t x_start,
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


/***** PRIVATE FUNCTIONS *****/

void Screen::Clip(const uint16_t x_start,
    const uint16_t y_start,
    uint16_t& x_end,
    uint16_t& y_end)
{
    if (x_start + x_end - 1 >= view_width) x_end = view_width - x_start;
    if (y_start + y_end - 1 >= view_height) y_end = view_height - y_start;
}

uint16_t Screen::ViewWidth() const
{
    return view_width;
}

uint16_t Screen::ViewHeight() const
{
    return view_height;
}


void Screen::FillRectangleAsync(const uint16_t x_start,
    const uint16_t y_start,
    uint16_t x_end,
    uint16_t y_end,
    const uint16_t colour)
{
    // Clip to the size of the screen
    if (x_start >= view_width || y_start >= view_height) return;

    if (x_end >= view_width) x_end = view_width;
    if (y_end >= view_height) y_end = view_height;

    uint32_t num_pixels = (x_end - x_start) * (y_end - y_start);

    draw_matrix.Allocate(19);

    draw_matrix.Push(colour);
    draw_matrix.Push(x_start);
    draw_matrix.Push(y_start);
    draw_matrix.Push(x_end);
    draw_matrix.Push(y_end);
    draw_matrix.Push(x_start);
    draw_matrix.Push(y_start);
    draw_matrix.Push(num_pixels);
    draw_matrix.Push((uint8_t)0); // initialized = 0;

    PushDrawingFunction((void*)&FillRectangleAsyncProcedure);
    draw_async = 1;
    async_draw_ready = 1;
}

void Screen::PushDrawingFunction(void* func)
{
    Drawing_Func_Ring[drawing_func_write++] = func;

    if (drawing_func_write >= 8)
    {
        drawing_func_write = 0;
    }
}

void Screen::UpdateDrawingFunction(void* func)
{
    Drawing_Func_Ring[drawing_func_read] = func;
}

inline void Screen::DrawNext()
{
    if (Drawing_Func_Ring[drawing_func_read] != nullptr)
        ((void(*)(Screen*))Drawing_Func_Ring[drawing_func_read])(this);
}

void Screen::WriteDataDMAFree(uint8_t* data, const uint32_t data_size)
{
    HAL_SPI_Transmit_DMA(spi_handle, data, data_size);
}

void Screen::FillRectangleAsyncProcedure(Screen* screen)
{
    screen->draw_async = 1;
    uint16_t x_end = screen->draw_matrix.Read<uint16_t>(6);
    uint16_t y_end = screen->draw_matrix.Read<uint16_t>(8);

    if (!screen->draw_matrix.Read<uint8_t>(18) ||
        screen->buffer_overwritten_by_sync)
    {
        screen->buffer_overwritten_by_sync = 0;
        // uint16_t colour = *(uint16_t*)screen->PopVariable(0, 2);
        uint16_t colour = screen->draw_matrix.Read<uint16_t>(0);

        uint16_t x_start = screen->draw_matrix.Read<uint16_t>(2);
        uint16_t y_start = screen->draw_matrix.Read<uint16_t>(4);

        // Filled rectangles always use the same colour, so we can
        // just fill the buffer now and reuse it.
        uint32_t total_pixel_bytes = (((x_end - x_start) * (y_end - y_start)) * 2);

        if (total_pixel_bytes > Chunk_Buffer_Size * 2)
            total_pixel_bytes = Chunk_Buffer_Size * 2;

        // This is big enough for a 64x64 rectangle
        for (uint32_t i = 0; i < total_pixel_bytes; i += 2)
        {
            screen->chunk_buffer[i] = static_cast<uint8_t>(colour >> 8);
            screen->chunk_buffer[i + 1] = static_cast<uint8_t>(colour);
        }

        screen->draw_matrix.Write<uint8_t>((uint8_t)1, 18);
    }

    uint16_t x_current = screen->draw_matrix.Read<uint16_t>(10);
    uint16_t y_current = screen->draw_matrix.Read<uint16_t>(12);

    const uint16_t Pixel_Distance = 32;
    uint16_t x_width = x_current + Pixel_Distance;
    if (x_width > x_end)
        x_width = x_end;

    uint16_t y_height = y_current + Pixel_Distance;
    if (y_height > y_end)
        y_height = y_end;

    screen->Select();
    screen->SetWritablePixels(x_current, y_current, x_width - 1, y_height - 1);

    uint32_t remaining_pixels = screen->draw_matrix.Read<uint32_t>(14);

    uint32_t num_pixels = (x_width - x_current) * (y_height - y_current);
    if (num_pixels > remaining_pixels)
        num_pixels = remaining_pixels;

    remaining_pixels -= num_pixels;
    if (remaining_pixels == 0)
    {
        screen->draw_async_stop = 1;
    }
    screen->WriteDataDMAFree(screen->chunk_buffer, num_pixels * 2);

    x_current += x_width - x_current;
    if (x_current >= x_end)
    {
        // Get the start x
        x_current = screen->draw_matrix.Read<uint16_t>(2);
        y_current += y_height - y_current;

        if (y_current >= y_end)
        {
            // Get the start y
            y_current = screen->draw_matrix.Read<uint16_t>(4);
        }
        screen->draw_matrix.Write(y_current, 12);
    }

    screen->draw_matrix.Write(x_current, 10);
    screen->draw_matrix.Write(remaining_pixels, 14);
}

void Screen::Loop()
{
    if (async_draw_ready && !spi_busy)
    {
        spi_busy = 1;
        async_draw_ready = 0;

        DrawNext();
    }
}

inline void Screen::WaitUntilSPIFree()
{
    while (spi_busy)
    {
        __NOP();
    }
}
