#include "screen.hh"
#include "app_main.hh"
#include "font.hh"
#include "graphics.hh"
#include "ili9341.hh"
#include <math.h>

Screen::Screen(uint16_t* scanline_buffer,
               const uint16_t num_lines,
               const uint16_t num_cols,
               const uint16_t num_buffs,
               uint8_t* data_buff,
               const uint16_t width,
               const uint16_t height,
               const uint16_t quantization,
               const uint16_t screen_width_pixels,
               const uint16_t screen_height_pixels,
               SPI_HandleTypeDef& hspi,
               GPIO_TypeDef* cs_port,
               const uint16_t cs_pin,
               GPIO_TypeDef* dc_port,
               const uint16_t dc_pin,
               GPIO_TypeDef* rst_port,
               const uint16_t rst_pin,
               GPIO_TypeDef* bl_port,
               const uint16_t bl_pin,
               ILI9341::Orientation orientation) :
    scanline_buffer(scanline_buffer),
    num_lines(num_lines),
    num_cols(num_cols),
    num_buffs(num_buffs),
    data_buff(data_buff),
    width(width),
    height(height),
    quantization(quantization),
    ili9341(screen_width_pixels,
            screen_height_pixels,
            hspi,
            cs_port,
            cs_pin,
            dc_port,
            dc_pin,
            rst_port,
            rst_pin,
            bl_port,
            bl_pin,
            orientation),
    scanline_ptr(scanline_buffer),
    row(0),
    end_row(0),
    title_buffer{0},
    text_idx(0),
    texts_in_use(0),
    scroll_offset(0),
    text_buffer{},
    usr_buffer{0},
    usr_buffer_idx(0),
    title_x1(0),
    title_x2(screen_width_pixels),
    icons_x_offset(0),
    scroll_area_height(0),
    usr_text_y_offset(0),
    flags(0)
{
}

void Screen::Init()
{
    ili9341.Init();
    CalculateDrawAreas();
}

void Screen::Draw(uint32_t timeout)
{
    UNUSED(timeout);

    if (flags & FlagType::Title_Dirty)
    {
        UI_LOG_INFO("Title dirty");
        row = Title_y1;
        end_row = Title_y1 + font11x16.height;
        ili9341.SetRenderRegion(title_x1, row, title_x2, end_row);
        LowerFlag(FlagType::Title_Dirty);
        RaiseFlag(FlagType::Title_Rendering);
    }
    else if (flags & FlagType::Title_Rendering)
    {
        UI_LOG_INFO("Title rendering");
        // TODO Move to a render function
        const uint16_t quantization_divisor = 8 / quantization;
        const uint16_t y2 = end_row;
        for (uint16_t y1 = row; y1 < y2; ++y1)
        {
            for (uint16_t x1 = title_x1; x1 < title_x2; ++x1)
            {
                const uint16_t data_buff_idx = (x1 / quantization_divisor) + (y1 * width);
                if (x1 & 0x0001)
                {
                    // Upper
                    const uint8_t quant_colour = data_buff[data_buff_idx] & 0x0F;
                    const uint16_t real_colour = Colour_Map[quant_colour];
                    scanline_ptr[x1] = real_colour;
                }
                else
                {
                    // Lower
                    const uint8_t quant_colour = data_buff[data_buff_idx] >> 4;
                    const uint16_t real_colour = Colour_Map[quant_colour];
                    scanline_ptr[x1] = real_colour;
                }
            }
            // TODO use dma
            ili9341.WriteData(reinterpret_cast<const uint8_t*>(scanline_ptr + title_x1),
                              (title_x2 - title_x1));
        }
    }
    else if (flags & FlagType::Icons_Dirty)
    {
    }
    else if (flags & FlagType::Text_Area_Dirty)
    {
    }
    else if (flags & FlagType::Usr_Text_Dirty)
    {
    }
}

void Screen::EnableBacklight()
{
    ili9341.EnableBacklight();
}

void Screen::DisableBacklight()
{
    ili9341.DisableBacklight();
}

void Screen::ScrollScreen(const uint16_t scroll_idx, bool up)
{
    // TODO, move into ili9341
    // static uint16_t scroll_d = 0;
    //
    // switch (orientation)
    // {
    // case Orientation::portrait:
    // case Orientation::left_landscape:
    // {
    //     if (up)
    //     {
    //         scroll_d = scroll_idx;
    //     }
    //     else
    //     {
    //         scroll_d = ili9341.GetHeignt() - scroll_idx;
    //     }
    //     break;
    // }
    // case Orientation::flipped_portrait:
    // case Orientation::right_landscape:
    // {
    //     if (up)
    //     {
    //         scroll_d = view_height - scroll_idx;
    //     }
    //     else
    //     {
    //         scroll_d = scroll_idx;
    //     }
    //     break;
    // }
    // default:
    // {
    //     scroll_d = 0;
    //     return;
    // }
    // }
    //
    // uint8_t vert_scroll_idx_data[] = {static_cast<uint8_t>(scroll_d >> 8),
    //                                   static_cast<uint8_t>(scroll_d)};
    //
    // WriteCommand(Vertical_Scroll_Start_Address);
    // WriteDataWithSet(vert_scroll_idx_data, 2);
}

void Screen::FillScreen(const Graphics::Colour colour)
{
    Graphics::Shape rec = {
        .type = Graphics::ShapeType::FillRectangle,
        .fill_rectangle =
            {
                .x1 = 0,
                .y1 = 0,
                .x2 = ili9341.GetWidth(),
                .y2 = ili9341.GetHeight(),
                .colour = colour,
                .flags = 0,
            },
    };

    Graphics::Rasterize(rec, data_buff, width, height, 0, ili9341.GetHeight());
    RaiseFlag(FlagType::Whole_Screen_Dirty);
}

void Screen::UpdateTitle(const char* title, const uint32_t len)
{
    // TODO calculate left padding to center the text
    const uint16_t actual_len = len < Title_Length ? len : Title_Length;
    for (uint16_t i = 0; i < actual_len; ++i)
    {
        title_buffer[i] = title[i];
    }

    // Clear the Title section
    Graphics::Shape clear_rec = {
        .type = Graphics::ShapeType::FillRectangle,
        .fill_rectangle =
            {
                .x1 = title_x1,
                .y1 = Title_y1,
                .x2 = title_x2,
                .y2 = Title_y1 + static_cast<const uint16_t>(title_font.height),
                .colour = Graphics::Colour::Black,
                .flags = 0,
            },
    };

    Graphics::Rasterize(clear_rec, data_buff, width, height, 0, Top_Fixed_Height);

    // TODO calculate the x1 based on how long the string is
    Graphics::Shape title_shape = {
        .type = Graphics::ShapeType::String,
        .string =
            {
                .str = title_buffer,
                .len = actual_len,
                .x = title_x1,
                .y = Title_y1,
                .font = title_font,
                .foreground = Graphics::Colour::White,
                .background = Graphics::Colour::Black,
                .flags = 0,
            },
    };

    Graphics::Rasterize(title_shape, data_buff, width, height, Title_y1,
                        Title_y1 + title_font.height);

    RaiseFlag(FlagType::Title_Dirty);
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
        text_buf[uint32_t(text_len++)] = text[idx++];
    }
}

void Screen::CommitText()
{
    // const uint16_t y = Top_Fixed_Area + font5x8.height * text_idx;
    // // Send a scroll text command if we have a full text buff
    // if (texts_in_use >= Max_Texts)
    // {
    //
    //     FillRectangle(0, view_width, y, y + font5x8.height, Colour::Black);
    //     DrawString(0, y, text_buffer[text_idx], text_lens[text_idx], font5x8, Colour::White,
    //                Colour::Black);
    //
    //     scroll_offset += font5x8.height;
    //     if (scroll_offset >= Scroll_Area_Height)
    //     {
    //         scroll_offset = 0;
    //     }
    //     ScrollScreen(Top_Fixed_Area + scroll_offset, true);
    //
    //     // Draw a rectangle to remove the text from the screen.
    //     // TODO replace colour with a class variable
    // }
    // else
    // {
    //     ++texts_in_use;
    //     // Enqueue the string draw
    //     DrawString(0, y, text_buffer[text_idx], text_lens[text_idx], font5x8, Colour::White,
    //                Colour::Black);
    // }
    //
    // if (++text_idx >= Max_Texts)
    // {
    //     text_idx = 0;
    // }
    // text_lens[text_idx] = 0;
}

void Screen::CommitText(const char* text, const uint32_t len)
{
    AppendText(text, len);
    CommitText();
}

void Screen::AppendUserText(const char* text, const uint32_t len)
{
    // if (usr_buffer_idx >= Max_Characters)
    // {
    //     return;
    // }
    //
    // const uint32_t num_char = std::min(len, Max_Characters - usr_buffer_idx);
    // for (uint32_t i = 0; i < num_char; ++i)
    // {
    //     usr_buffer[usr_buffer_idx + i] = text[i];
    // }
    //
    // DrawString(usr_buffer_idx * font7x12.width, User_Text_Height_Offset,
    //            usr_buffer + usr_buffer_idx, num_char, font7x12, Colour::White, Colour::Black);
    //
    // usr_buffer_idx += num_char;
}

void Screen::AppendUserText(const char ch)
{
    // if (usr_buffer_idx >= Max_Characters)
    // {
    //     return;
    // }
    //
    // usr_buffer[usr_buffer_idx] = ch;
    //
    // DrawString(usr_buffer_idx * font7x12.width, User_Text_Height_Offset,
    //            usr_buffer + usr_buffer_idx, 1, font7x12, Colour::White, Colour::Black);
    //
    // ++usr_buffer_idx;
}

void Screen::BackspaceUserText()
{
    // if (usr_buffer_idx == 0)
    // {
    //     return;
    // }
    //
    // --usr_buffer_idx;
    //
    // FillRectangle(usr_buffer_idx * font7x12.width, (usr_buffer_idx + 1) * font7x12.width,
    //               User_Text_Height_Offset, User_Text_Height_Offset + font7x12.height,
    //               Colour::Black);
}

void Screen::ClearUserText()
{
    // FillRectangle(0, usr_buffer_idx * font7x12.width, User_Text_Height_Offset,
    //               User_Text_Height_Offset + font7x12.height, Colour::Black);
    //
    // usr_buffer_idx = 0;
}

const char* Screen::UserText() const noexcept
{
    return usr_buffer;
}

uint16_t Screen::UserTextLength() const noexcept
{
    return usr_buffer_idx;
}

// Private functions
void Screen::RotateScreen(ILI9341::Orientation orientation)
{
    ili9341.Rotate(orientation);
    CalculateDrawAreas();
}

void Screen::CalculateDrawAreas()
{
    usr_text_y_offset = ili9341.GetHeight() - Bottom_Fixed_Height;
    scroll_area_height = usr_text_y_offset - Top_Fixed_Height;
}

void Screen::RaiseFlag(const FlagType flag)
{
    flags |= flag;
}

void Screen::LowerFlag(const FlagType flag)
{
    flags &= ~flag;
}

// inline void Screen::WaitForSPIComplete()
// {
//     while (spi->State != HAL_SPI_STATE_READY)
//     {
//         __NOP();
//     }
// }
//
// inline void Screen::SetPinToCommand()
// {
//     HAL_GPIO_WritePin(dc_port, dc_pin, GPIO_PIN_RESET);
// }
//
// inline void Screen::SetPinToData()
// {
//     HAL_GPIO_WritePin(dc_port, dc_pin, GPIO_PIN_SET);
// }
//
// void Screen::WriteCommand(uint8_t cmd)
// {
//     WaitForSPIComplete();
//     SetPinToCommand();
//     HAL_SPI_Transmit_DMA(spi, &cmd, sizeof(cmd));
// }
//
// void Screen::WriteCommand(uint8_t cmd, uint8_t* data, const uint32_t sz)
// {
//     WriteCommand(cmd);
//     WriteData(data, sz);
// }
//
// // SetPinToData needs to be called before this function
// void Screen::WriteData(uint8_t* data, const uint32_t data_size)
// {
//     WaitForSPIComplete();
//     HAL_SPI_Transmit_DMA(spi, data, data_size);
// }
//
// void Screen::WriteDataWithSet(uint8_t data)
// {
//     WaitForSPIComplete();
//     SetPinToData();
//     HAL_SPI_Transmit_DMA(spi, &data, sizeof(data));
// }
//
// void Screen::WriteDataWithSet(uint8_t* data, const uint32_t data_size)
// {
//     WaitForSPIComplete();
//     SetPinToData();
//     HAL_SPI_Transmit_DMA(spi, data, data_size);
// }
