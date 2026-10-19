#include "screen.hh"
#include "app_main.hh"
#include "font.hh"
#include "graphics.hh"
#include "ili9341.hh"
#include <math.h>
#include <cstring>

Screen::Screen(uint16_t* scanline_buffer,
               const uint16_t num_lines,
               const uint16_t num_cols,
               const uint16_t num_buffs,
               uint8_t* data_buff,
               const uint16_t data_buff_width,
               const uint16_t data_buff_height,
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
    data_buff_width(data_buff_width),
    data_buff_height(data_buff_height),
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
    draw_x1(0),
    draw_y1(0),
    draw_x2(0),
    draw_y2(0),
    title_buffer{0},
    title_len(0),
    text_idx(0),
    texts_in_use(0),
    scroll_offset(0),
    text_buffer{},
    text_lens{},
    max_texts(Max_Texts),
    usr_buffer{0},
    usr_buffer_idx(0),
    title_x1(0),
    title_x2(0),
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

    const uint32_t flag = TakeFlag();

    switch (flag)
    {
    case FlagType::Rendering:
    {
        UI_LOG_INFO("rendering");
        Render();
        return;
    }
    case FlagType::Whole_Screen_Dirty:
    {
        UI_LOG_INFO("Whole screen is dirty");
        draw_x1 = 0;
        draw_y1 = 0;
        draw_x2 = ili9341.GetWidth();
        draw_y2 = ili9341.GetHeight();

        // Drop all flags, as this will cover all areas
        flags = 0;
        break;
    }
    case FlagType::Title_Dirty:
    {
        UI_LOG_INFO("Title dirty");
        draw_x1 = title_x1;
        draw_y1 = Title_y1;
        draw_x2 = title_x2;
        draw_y2 = Title_y1 + font11x16.height;
        break;
    }
    case FlagType::Icons_Dirty:
    {
        break;
    }
    case FlagType::Text_Area_Dirty:
    {
        UI_LOG_INFO("Text area dirty");
        draw_x1 = 0;
        draw_x2 = ili9341.GetWidth();
        draw_y1 = usr_text_y_offset - text_font.height * text_idx;
        draw_y1 = 0;
        draw_y2 = usr_text_y_offset;
        break;
    }
    case FlagType::Usr_Text_Dirty:
    {
        UI_LOG_INFO("User text area dirty");
        draw_x1 = 0;
        draw_x2 = ili9341.GetWidth();
        draw_y1 = usr_text_y_offset;
        draw_y2 = usr_text_y_offset + text_font.height;
        break;
    }
    default:
    {
        return;
    }
    }

    RaiseFlag(Rendering);
    ili9341.SetRenderRegion(draw_x1, draw_y1, draw_x2 - 1, draw_y2 - 1);
    Render();
}

void Screen::Update()
{
}

void Screen::Render()
{
    // TODO Move to a render function
    const uint16_t quantization_divisor = 8 / quantization;
    for (uint16_t y = draw_y1; y < draw_y2; ++y)
    {
        for (uint16_t x = draw_x1; x < draw_x2; ++x)
        {
            const uint16_t data_buff_idx = (x / quantization_divisor) + (y * data_buff_width);
            const uint8_t quant_colour =
                (data_buff[data_buff_idx] >> (4 * ((x ^ 0x0001) & 0x0001))) & 0x0F;
            const uint16_t real_colour = Colour_Map[quant_colour];

            scanline_ptr[x] = real_colour;
        }
        // TODO use dma
        ili9341.WriteData(reinterpret_cast<const uint8_t*>(scanline_ptr + draw_x1),
                          (draw_x2 - draw_x1) * 2);
    }
    // TODO this needs to be only once y1 == y2
    // and everything needs to be put into a dma
    LowerFlag(Rendering);
}

void Screen::EnableBacklight()
{
    ili9341.EnableBacklight();
}

void Screen::DisableBacklight()
{
    ili9341.DisableBacklight();
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

    Graphics::Rasterize(rec, data_buff, data_buff_width, data_buff_height, 0, ili9341.GetHeight());
    RaiseFlag(FlagType::Whole_Screen_Dirty);
}

void Screen::UpdateTitle(const char* title, const uint32_t len)
{
    const uint16_t title_len = len < Max_Title_Length ? len : Max_Title_Length;
    for (uint16_t i = 0; i < title_len; ++i)
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

    Graphics::Rasterize(clear_rec, data_buff, data_buff_width, data_buff_height, 0,
                        Top_Fixed_Height);

    const uint16_t next_x2 = title_x1 + title_len * title_font.width;
    title_x2 = next_x2 > title_x2 ? next_x2 : title_x2;

    Graphics::Shape title_shape = {
        .type = Graphics::ShapeType::String,
        .string =
            {
                .str = title_buffer,
                .len = title_len,
                .x = title_x1,
                .y = Title_y1,
                .font = title_font,
                .foreground = Graphics::Colour::White,
                .background = Graphics::Colour::Black,
                .flags = 0,
            },
    };

    Graphics::Rasterize(title_shape, data_buff, data_buff_width, data_buff_height, Title_y1,
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
    // TODO actual texts need to be moved in memory, or we instead use an array of structs that has
    // the text order in their meta data
    ScrollText();

    Graphics::Shape str = {
        .type = Graphics::ShapeType::String,
        .string =
            {
                .str = text_buffer[text_idx],
                .len = text_lens[text_idx],
                .x = 0,
                .y = static_cast<uint16_t>(usr_text_y_offset - text_font.height),
                .font = text_font,
                .foreground = Graphics::Colour::White,
                .background = Graphics::Colour::Black,
                .flags = 0,
            },
    };

    Graphics::Rasterize(str, data_buff, data_buff_width, data_buff_height, 0, ili9341.GetHeight());
    RaiseFlag(FlagType::Text_Area_Dirty);

    if (text_idx < Max_Texts)
    {
        ++text_idx;
    }
}

void Screen::CommitText(const char* text, const uint32_t len)
{
    AppendText(text, len);
    CommitText();
}

void Screen::AppendUserText(const char* text, const uint32_t len)
{
    if (usr_buffer_idx >= Max_Characters)
    {
        return;
    }

    const uint32_t last_idx = usr_buffer_idx;
    const uint32_t num_char = std::min(len, Max_Characters - usr_buffer_idx);
    for (uint32_t i = 0; i < num_char; ++i)
    {
        usr_buffer[usr_buffer_idx + i] = text[i];
    }

    Graphics::Shape str = {
        .type = Graphics::ShapeType::String,
        .string =
            {
                .str = usr_buffer + last_idx,
                .len = static_cast<uint16_t>(num_char),
                .x = static_cast<uint16_t>(last_idx * text_font.width),
                .y = usr_text_y_offset,
                .font = text_font,
                .foreground = Graphics::Colour::White,
                .background = Graphics::Colour::Black,
                .flags = 0,
            },
    };

    Graphics::Rasterize(str, data_buff, data_buff_width, data_buff_height, usr_text_y_offset,
                        usr_text_y_offset + text_font.height);

    usr_buffer_idx += num_char;
    RaiseFlag(FlagType::Usr_Text_Dirty);
}

void Screen::AppendUserText(const char ch)
{
    if (usr_buffer_idx >= Max_Characters)
    {
        return;
    }

    usr_buffer[usr_buffer_idx] = ch;

    Graphics::Shape str = {
        .type = Graphics::ShapeType::String,
        .string =
            {
                .str = usr_buffer + usr_buffer_idx,
                .len = static_cast<uint16_t>(1),
                .x = static_cast<uint16_t>(usr_buffer_idx * text_font.width),
                .y = usr_text_y_offset,
                .font = text_font,
                .foreground = Graphics::Colour::White,
                .background = Graphics::Colour::Black,
                .flags = 0,
            },
    };

    Graphics::Rasterize(str, data_buff, data_buff_width, data_buff_height, usr_text_y_offset,
                        usr_text_y_offset + text_font.height);

    ++usr_buffer_idx;
    RaiseFlag(FlagType::Usr_Text_Dirty);
}

void Screen::BackspaceUserText()
{
    if (usr_buffer_idx == 0)
    {
        return;
    }

    usr_buffer[--usr_buffer_idx] = '\0';

    const uint16_t x1 = usr_buffer_idx * text_font.width;
    Graphics::Shape rec = {
        .type = Graphics::ShapeType::FillRectangle,
        .fill_rectangle =
            {
                .x1 = x1,
                .y1 = usr_text_y_offset,
                .x2 = static_cast<uint16_t>(x1 + text_font.width),
                .y2 = static_cast<uint16_t>(usr_text_y_offset + text_font.height),
                .colour = Graphics::Colour::Black,
                .flags = 0,
            },
    };

    Graphics::Rasterize(rec, data_buff, data_buff_width, data_buff_height, usr_text_y_offset,
                        usr_text_y_offset + text_font.height);
    RaiseFlag(FlagType::Usr_Text_Dirty);
}

void Screen::ClearUserText()
{
    Graphics::Shape rec = {
        .type = Graphics::ShapeType::FillRectangle,
        .fill_rectangle =
            {
                .x1 = 0,
                .y1 = usr_text_y_offset,
                .x2 = static_cast<uint16_t>(ili9341.GetWidth()),
                .y2 = static_cast<uint16_t>(usr_text_y_offset + text_font.height),
                .colour = Graphics::Colour::Black,
                .flags = 0,
            },
    };

    Graphics::Rasterize(rec, data_buff, data_buff_width, data_buff_height, usr_text_y_offset,
                        usr_text_y_offset + text_font.height);

    std::memset(usr_buffer, 0, usr_buffer_idx);
    usr_buffer_idx = 0;
    RaiseFlag(FlagType::Usr_Text_Dirty);
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
    usr_text_y_offset = ili9341.GetHeight() - text_font.height;
    scroll_area_height = usr_text_y_offset - Top_Fixed_Height;
    max_texts = scroll_area_height / text_font.height;
}

void Screen::RaiseFlag(const FlagType flag)
{
    flags |= flag;
}

void Screen::LowerFlag(const FlagType flag)
{
    flags &= ~flag;
}

uint32_t Screen::TakeFlag()
{
    const uint32_t flag = flags & (~flags + 1);
    LowerFlag(static_cast<FlagType>(flag));
    return flag;
}

void Screen::PushShape(Graphics::Shape shape)
{
    Graphics::Rasterize(shape, data_buff, data_buff_width, data_buff_height, 0,
                        ili9341.GetHeight());
}

void Screen::ScrollText()
{
    const uint16_t dest_idx = text_idx < max_texts ? text_idx + 1 : text_idx;
    const uint16_t src_idx = text_idx < max_texts ? text_idx : text_idx - 1;

    uint16_t dest_y = usr_text_y_offset - dest_idx * text_font.height;
    const uint16_t src_y1 = usr_text_y_offset - src_idx * text_font.height;
    const uint16_t src_y2 = usr_text_y_offset;

    for (uint16_t src_y = src_y1; src_y < src_y2; src_y += text_font.height)
    {
        std::memcpy(data_buff + dest_y * data_buff_width, data_buff + src_y * data_buff_width,
                    text_font.height * data_buff_width);
        dest_y += text_font.height;
    }

    std::memset(data_buff + (usr_text_y_offset - text_font.height) * data_buff_width, 0,
                data_buff_width * text_font.height);
}
