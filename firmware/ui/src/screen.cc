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
    last_ticks_ms(0),
    last_animation_ticks_ms(0),
    animation_flags(0),
    render_flags(0)
{
}

void Screen::Init()
{
    ili9341.Init();
    CalculateDrawAreas();
}

void Screen::Draw(const uint32_t timeout)
{
    UNUSED(timeout);

    const RenderFlags flag = static_cast<RenderFlags>(TakeFlag(render_flags));

    switch (flag)
    {
    case RenderFlags::Rendering:
    {
        UI_LOG_INFO("rendering");
        Render();
        return;
    }
    case RenderFlags::Whole_Screen_Dirty:
    {
        UI_LOG_INFO("Whole screen is dirty");
        draw_x1 = 0;
        draw_y1 = 0;
        draw_x2 = ili9341.GetWidth();
        draw_y2 = ili9341.GetHeight();

        // Drop all flags, as this will cover all areas
        render_flags = 0;
        break;
    }
    case RenderFlags::Title_Dirty:
    {
        UI_LOG_INFO("Title dirty");
        draw_x1 = title_x1;
        draw_y1 = Title_y1;
        draw_x2 = title_x2;
        draw_y2 = Title_y1 + font11x16.height;
        break;
    }
    case RenderFlags::Icons_Dirty:
    {
        break;
    }
    case RenderFlags::Text_Area_Dirty:
    {
        UI_LOG_INFO("Text area dirty");
        draw_x1 = 0;
        draw_x2 = ili9341.GetWidth();
        draw_y1 = usr_text_y_offset - text_font.height * text_idx;
        draw_y1 = 0;
        draw_y2 = usr_text_y_offset;
        break;
    }
    case RenderFlags::Usr_Text_Dirty:
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

    RaiseFlag(render_flags, Rendering);
    ili9341.SetRenderRegion(draw_x1, draw_y1, draw_x2 - 1, draw_y2 - 1);
    Render();
}

// TODO could maybe just use some HAL_GetTick
void Screen::AnimationUpdate(const uint32_t ticks_ms)
{
    last_ticks_ms = ticks_ms;
    // NOTE- this is updated by a timer
    if (last_ticks_ms - last_animation_ticks_ms >= 1000)
    {
        const uint32_t solid = ReadFlag(animation_flags, AnimationFlags::CursorSolid);
        DrawCursor(solid);
        RaiseFlag(render_flags, RenderFlags::Usr_Text_Dirty);
    }
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
    LowerFlag(render_flags, Rendering);
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
    RaiseFlag(render_flags, RenderFlags::Whole_Screen_Dirty);
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

    RaiseFlag(render_flags, RenderFlags::Title_Dirty);
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
    // just switch to a ring buffer, or roll the index
    //
    // another todo, give the user text more space
    ScrollText();

    Graphics::Shape str = {
        .type = Graphics::ShapeType::String,
        .string =
            {
                .str = text_buffer[text_idx],
                .len = text_lens[text_idx],
                .x = 0,
                .y = static_cast<uint16_t>(ili9341.GetHeight() - Bottom_Fixed_Height
                                           - text_font.height),
                .font = text_font,
                .foreground = Graphics::Colour::White,
                .background = Graphics::Colour::Black,
                .flags = 0,
            },
    };

    Graphics::Rasterize(str, data_buff, data_buff_width, data_buff_height, 0, ili9341.GetHeight());
    RaiseFlag(render_flags, RenderFlags::Text_Area_Dirty);

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

    DrawCursor(true);
    RaiseFlag(render_flags, RenderFlags::Usr_Text_Dirty);
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

    DrawCursor(true);
    RaiseFlag(render_flags, RenderFlags::Usr_Text_Dirty);
}

void Screen::BackspaceUserText()
{
    if (usr_buffer_idx == 0)
    {
        return;
    }

    const uint16_t x1 = usr_buffer_idx * text_font.width;
    const uint16_t x2 = std::min<uint16_t>(x1 + text_font.width * 2, ili9341.GetWidth());
    Graphics::Shape rec = {
        .type = Graphics::ShapeType::FillRectangle,
        .fill_rectangle =
            {
                .x1 = x1,
                .y1 = usr_text_y_offset,
                .x2 = x2,
                .y2 = static_cast<uint16_t>(usr_text_y_offset + text_font.height),
                .colour = Graphics::Colour::Black,
                .flags = 0,
            },
    };

    Graphics::Rasterize(rec, data_buff, data_buff_width, data_buff_height, usr_text_y_offset,
                        usr_text_y_offset + text_font.height);

    usr_buffer[--usr_buffer_idx] = '\0';

    DrawCursor(true);
    RaiseFlag(render_flags, RenderFlags::Usr_Text_Dirty);
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

    DrawCursor(true);
    RaiseFlag(render_flags, RenderFlags::Usr_Text_Dirty);
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

void Screen::RaiseFlag(uint32_t& flags, const uint32_t flag)
{
    flags |= flag;
}

void Screen::LowerFlag(uint32_t& flags, const uint32_t flag)
{
    flags &= ~flag;
}

uint32_t Screen::ReadFlag(const uint32_t flags, const uint32_t flag)
{
    return flags & flag;
}

uint32_t Screen::TakeFlag(uint32_t& flags)
{
    const uint32_t flag = flags & (~flags + 1);
    LowerFlag(flags, flag);
    return flag;
}

void Screen::DrawTitleLine()
{
    constexpr uint16_t line_x1 = 0;
    constexpr uint16_t line_y1 = Top_Fixed_Height - 1;
    const uint16_t line_x2 = ili9341.GetWidth();
    constexpr uint16_t line_y2 = Top_Fixed_Height;
    Graphics::Shape rec = {
        .type = Graphics::ShapeType::FillRectangle,
        .fill_rectangle =
            {
                .x1 = line_x1,
                .y1 = line_y1,
                .x2 = line_x2,
                .y2 = line_y2,
                .colour = Graphics::Colour::White,
                .flags = 0,
            },
    };

    Graphics::Rasterize(rec, data_buff, data_buff_width, data_buff_height, line_y1, line_y2);
    RaiseFlag(render_flags, RenderFlags::Title_Dirty);
}

void Screen::DrawUserLine()
{
    constexpr uint16_t line_x1 = 0;
    const uint16_t line_y1 = ili9341.GetHeight() - Bottom_Fixed_Height;
    const uint16_t line_x2 = ili9341.GetWidth();
    const uint16_t line_y2 = ili9341.GetHeight() - Bottom_Fixed_Height + 1;
    Graphics::Shape rec = {
        .type = Graphics::ShapeType::FillRectangle,
        .fill_rectangle =
            {
                .x1 = line_x1,
                .y1 = line_y1,
                .x2 = line_x2,
                .y2 = line_y2,
                .colour = Graphics::Colour::White,
                .flags = 0,
            },
    };

    Graphics::Rasterize(rec, data_buff, data_buff_width, data_buff_height, line_y1, line_y2);
    RaiseFlag(render_flags, RenderFlags::Text_Area_Dirty);
}

void Screen::DrawCursor(const bool solid)
{
    const uint16_t x1 = static_cast<uint16_t>(usr_buffer_idx * text_font.width);
    const uint16_t x2 = std::min<uint16_t>(x1 + text_font.width, ili9341.GetWidth());
    const uint16_t y2 = static_cast<uint16_t>(usr_text_y_offset + text_font.height);

    // Clear or fill in
    Graphics::Shape rec = {
        .type = Graphics::ShapeType::FillRectangle,
        .fill_rectangle =
            {
                .x1 = x1,
                .y1 = usr_text_y_offset,
                .x2 = x2,
                .y2 = y2,
                .colour = solid ? Graphics::Colour::White : Graphics::Colour::Black,
                .flags = 0,
            },
    };
    Graphics::Rasterize(rec, data_buff, data_buff_width, data_buff_height, usr_text_y_offset,
                        usr_text_y_offset + text_font.height);

    if (solid)
    {
        LowerFlag(animation_flags, AnimationFlags::CursorSolid);
    }
    else
    {
        Graphics::Shape rec = {
            .type = Graphics::ShapeType::Rectangle,
            .rectangle =
                {
                    .x1 = x1,
                    .y1 = usr_text_y_offset,
                    .x2 = x2,
                    .y2 = y2,
                    .thickness = 1,
                    .colour = Graphics::Colour::White,
                    .flags = 0,
                },
        };
        Graphics::Rasterize(rec, data_buff, data_buff_width, data_buff_height, usr_text_y_offset,
                            usr_text_y_offset + text_font.height);
        RaiseFlag(animation_flags, AnimationFlags::CursorSolid);
    }

    last_animation_ticks_ms = last_ticks_ms;
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
