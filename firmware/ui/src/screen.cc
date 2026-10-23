#include "screen.hh"
#include "audio_icon.hh"
#include "battery_icon.hh"
#include "font.hh"
#include "graphics.hh"
#include "ili9341.hh"
#include "moq_icon.hh"
#include "wifi_icon.hh"
#include <math.h>
#include <cstdint>
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
    rasterize_bound({screen_width_pixels, screen_height_pixels, 0, 0}),
    draw_bound({screen_width_pixels, screen_height_pixels, 0, 0}),
    title_x1(0),
    title_x2(0),
    title_buffer{0},
    title_len(0),
    text_write(0),
    text_read(0),
    texts_in_use(0),
    scroll_offset(0),
    text_buffer{},
    text_lens{},
    max_texts(Max_Texts),
    usr_buffer{0},
    usr_buffer_idx(0),
    shapes(20),
    icons_x_offset(0),
    usr_text_y_offset(0),
    texts_area_y_offset(0),
    scroll_area_height(0),
    last_ticks_ms(0),
    last_animation_ticks_ms(0),
    animation_flags(0),
    battery_level(0),
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

    if (ReadFlag(render_flags, RenderFlags::Rendering))
    {
        Render();
        return;
    }

    if (shapes.Unread())
    {

        UI_LOG_INFO("Shapes to rasterize! %d", shapes.Unread());
        // Empty the queue
        while (shapes.Unread())
        {
            Graphics::Shape& shape = shapes.Read();
            if (shape.done)
            {
                continue;
            }

            Graphics::Graphics::bound_t bound = Graphics::GetBounds(shape);
            RasterizeShape(shape, bound);
        }

        RaiseFlag(render_flags, RenderFlags::Rasterized);
    }

    if (!TakeFlag(render_flags, RenderFlags::Rasterized))
    {
        return;
    }

    draw_bound = rasterize_bound;
    ResetBound(rasterize_bound);

    draw_bound.x2 = std::min<uint16_t>(draw_bound.x2, ili9341.GetWidth());
    draw_bound.y2 = std::min<uint16_t>(draw_bound.y2, ili9341.GetHeight());
    if (draw_bound.x1 >= draw_bound.x2 || draw_bound.y1 >= draw_bound.y2)
    {
        return;
    }

    RaiseFlag(render_flags, Rendering);
    ili9341.SetRenderRegion(draw_bound.x1, draw_bound.y1, draw_bound.x2 - 1, draw_bound.y2 - 1);

    Render();
}

void Screen::AnimationUpdate(const uint32_t ticks_ms)
{
    last_ticks_ms = ticks_ms;
    if (last_ticks_ms - last_animation_ticks_ms >= 1000)
    {
        const uint32_t solid = ReadFlag(animation_flags, AnimationFlags::CursorSolid);
        DrawCursor(solid);
    }
}

void Screen::Render()
{
    const uint16_t quantization_divisor = 8 / quantization;
    for (uint16_t y = draw_bound.y1; y < draw_bound.y2; ++y)
    {
        for (uint16_t x = draw_bound.x1; x < draw_bound.x2; ++x)
        {
            const uint16_t data_buff_idx = (x / quantization_divisor) + (y * data_buff_width);
            const uint8_t quant_colour =
                (data_buff[data_buff_idx] >> (4 * ((x ^ 0x0001) & 0x0001))) & 0x0F;
            const uint16_t real_colour = Colour_Map[quant_colour];

            scanline_ptr[x] = real_colour;
        }
        // TODO use dma
        ili9341.WriteData(reinterpret_cast<const uint8_t*>(scanline_ptr + draw_bound.x1),
                          (draw_bound.x2 - draw_bound.x1) * 2);
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

void Screen::FullRefresh()
{
    Graphics::ExpandBound(rasterize_bound, {
                                               .x1 = 0,
                                               .y1 = 0,
                                               .x2 = ili9341.GetWidth(),
                                               .y2 = ili9341.GetHeight(),
                                           });
    RaiseFlag(render_flags, RenderFlags::Rasterized);
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
            },
    };

    SubmitShape(rec);
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
                .y2 = static_cast<const uint16_t>(Title_y1 + title_font.height),
                .colour = Graphics::Colour::Black,
            },
    };

    SubmitShape(clear_rec);

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
                .font = &title_font,
                .foreground = Graphics::Colour::White,
                .background = Graphics::Colour::Black,
            },
    };

    SubmitShape(title_shape);
}

void Screen::AppendText(const char* text, const uint32_t len)
{
    // Work our way down
    const uint32_t num_bytes = Max_Characters < len ? Max_Characters : len;
    char* text_buf = text_buffer[text_write];
    char& text_len = text_lens[text_write];

    size_t idx = 0;
    while (text_len < Max_Characters && idx < num_bytes)
    {
        text_buf[uint32_t(text_len++)] = text[idx++];
    }
}

void Screen::CommitText()
{
    ScrollText();

    Graphics::Shape str = {
        .type = Graphics::ShapeType::String,
        .string =
            {
                .str = text_buffer[text_read],
                .len = text_lens[text_read],
                .x = 0,
                .y = static_cast<uint16_t>(texts_area_y_offset - text_font.height),
                .font = &text_font,
                .foreground = Graphics::Colour::White,
                .background = Graphics::Colour::Black,
            },
    };

    SubmitShape(str);

    if (texts_in_use < max_texts)
    {
        ++texts_in_use;
    }
    else
    {
    }

    ++text_write;
    if (text_write >= max_texts)
    {
        text_write = 0;
    }
    ++text_read;
    if (text_read >= max_texts)
    {
        text_read = 0;
    }

    // Clear the next write
    text_lens[text_write] = '\0';
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

    const uint32_t num_char = std::min(len, Max_Characters - usr_buffer_idx);
    for (uint32_t i = 0; i < num_char; ++i)
    {
        usr_buffer[usr_buffer_idx + i] = text[i];
    }

    const uint16_t x1 = static_cast<uint16_t>(usr_buffer_idx * text_font.width);
    Graphics::Shape str = {
        .type = Graphics::ShapeType::String,
        .string =
            {
                .str = usr_buffer + usr_buffer_idx,
                .len = static_cast<uint16_t>(num_char),
                .x = x1,
                .y = usr_text_y_offset,
                .font = &text_font,
                .foreground = Graphics::Colour::White,
                .background = Graphics::Colour::Black,
            },
    };

    SubmitShape(str);

    usr_buffer_idx += num_char;

    DrawCursor(true);
}

void Screen::AppendUserText(const char ch)
{
    if (usr_buffer_idx >= Max_Characters)
    {
        return;
    }

    usr_buffer[usr_buffer_idx] = ch;

    const uint16_t x1 = static_cast<uint16_t>(usr_buffer_idx * text_font.width);
    Graphics::Shape str = {
        .type = Graphics::ShapeType::String,
        .string =
            {
                .str = usr_buffer + usr_buffer_idx,
                .len = static_cast<uint16_t>(1),
                .x = x1,
                .y = usr_text_y_offset,
                .font = &text_font,
                .foreground = Graphics::Colour::White,
                .background = Graphics::Colour::Black,
            },
    };

    SubmitShape(str);

    ++usr_buffer_idx;

    DrawCursor(true);
}

void Screen::BackspaceUserText()
{
    if (usr_buffer_idx == 0)
    {
        return;
    }

    const uint16_t x1 = usr_buffer_idx * text_font.width;
    const uint16_t x2 = std::min<uint16_t>(x1 + text_font.width * 2, ili9341.GetWidth());
    const uint16_t y2 = static_cast<uint16_t>(usr_text_y_offset + text_font.height);
    Graphics::Shape rec = {
        .type = Graphics::ShapeType::FillRectangle,
        .fill_rectangle =
            {
                .x1 = x1,
                .y1 = usr_text_y_offset,
                .x2 = x2,
                .y2 = y2,
                .colour = Graphics::Colour::Black,
            },
    };

    SubmitShape(rec);

    usr_buffer[--usr_buffer_idx] = '\0';

    DrawCursor(true);
}

void Screen::ClearUserText()
{
    const uint16_t x2 = static_cast<uint16_t>(ili9341.GetWidth());
    const uint16_t y2 = static_cast<uint16_t>(usr_text_y_offset + text_font.height);
    Graphics::Shape rec = {
        .type = Graphics::ShapeType::FillRectangle,
        .fill_rectangle =
            {
                .x1 = 0,
                .y1 = usr_text_y_offset,
                .x2 = x2,
                .y2 = y2,
                .colour = Graphics::Colour::Black,
            },
    };

    SubmitShape(rec);

    std::memset(usr_buffer, 0, usr_buffer_idx);
    usr_buffer_idx = 0;

    DrawCursor(true);
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
    texts_area_y_offset = ili9341.GetHeight() - Bottom_Fixed_Height;
    scroll_area_height = texts_area_y_offset - Top_Fixed_Height;

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

uint32_t Screen::TakeFlag(uint32_t& flags, const uint32_t flag)
{
    const uint32_t f = ReadFlag(flags, flag);
    LowerFlag(flags, flag);
    return f;
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
            },
    };

    SubmitShape(rec);
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
            },
    };

    SubmitShape(rec);
}

void Screen::DrawIcon(const IconType icon_type, const bool flag)
{
    const Bitmap* map = nullptr;
    constexpr uint16_t margin = 2;
    uint16_t x = ili9341.GetWidth();

    switch (icon_type)
    {
    case IconType::Speaker:
        x -= (Audio_Up_20x18::bitmap.width + margin + Mic_Up_20x18::bitmap.width + margin
              + Moq_Up_20x18::bitmap.width + margin + Wifi_Up_20x18::bitmap.width + margin
              + Battery_20x18::bitmap.width + margin);
        map = flag ? &Audio_Up_20x18::bitmap : &Audio_Down_20x18::bitmap;
        break;
    case IconType::Mic:
        x -= (Mic_Up_20x18::bitmap.width + margin + Moq_Up_20x18::bitmap.width + margin
              + Wifi_Up_20x18::bitmap.width + margin + Battery_20x18::bitmap.width + margin);
        map = flag ? &Mic_Up_20x18::bitmap : &Mic_Down_20x18::bitmap;
        break;
    case IconType::Moq:
        x -= (Moq_Up_20x18::bitmap.width + margin + Wifi_Up_20x18::bitmap.width + margin
              + Battery_20x18::bitmap.width + margin);
        map = flag ? &Moq_Up_20x18::bitmap : &Moq_Down_20x18::bitmap;
        break;
    case IconType::Wifi:
        x -= (Wifi_Up_20x18::bitmap.width + margin + Battery_20x18::bitmap.width + margin);
        map = flag ? &Wifi_Up_20x18::bitmap : &Wifi_Down_20x18::bitmap;
        break;
    case IconType::Battery:
        x -= (Battery_20x18::bitmap.width + margin);
        if (flag)
        {
            battery_status = false;
            map = &Battery_Charging_20x18::bitmap;
        }
        else
        {
            battery_status = true;
            map = &Battery_20x18::bitmap;
        }
        break;
    default:
        return;
    }

    const uint16_t y = Battery_Position_Y1;
    Graphics::Shape shape = {
        .type = Graphics::ShapeType::Bitmap,
        .bitmap =
            {
                .x = x,
                .y = y,
                .map = map,
                .foreground = Graphics::Colour::White,
                .background = Graphics::Colour::Black,
            },
    };

    SubmitShape(shape);

    if (icon_type == IconType::Battery)
    {
        BatteryLevel(battery_level);
    }
}

void Screen::BatteryLevel(const uint8_t percentage)
{
    battery_level = std::min<uint8_t>(percentage, 100);

    // Clear
    const uint16_t x2 = ili9341.GetWidth() - Battery_Level_X2_Margin;
    const uint16_t x1 =
        static_cast<uint16_t>((x2 - Battery_20x18::bitmap.width) + Battery_Level_X1_Margin);
    Graphics::Shape clear = {
        .type = Graphics::ShapeType::FillRectangle,
        .fill_rectangle =
            {
                .x1 = x1,
                .y1 = Battery_Charge_Position_Y1,
                .x2 = x2,
                .y2 = Battery_Charge_Position_Y2,
                .colour = Graphics::Colour::Black,
            },
    };

    SubmitShape(clear);

    const uint16_t width = x2 - x1;
    const uint16_t level_x1 = x2 - width * battery_level / 100;
    Graphics::Shape level = {
        .type = Graphics::ShapeType::FillRectangle,
        .fill_rectangle =
            {
                .x1 = level_x1,
                .y1 = Battery_Charge_Position_Y1,
                .x2 = x2,
                .y2 = Battery_Charge_Position_Y2,
                .colour = Graphics::Colour::White,
            },
    };

    SubmitShape(level);
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
            },
    };

    SubmitShape(rec);

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
                },
        };
        SubmitShape(rec);
        RaiseFlag(animation_flags, AnimationFlags::CursorSolid);
    }

    last_animation_ticks_ms = last_ticks_ms;
}

void Screen::ResetBound(Graphics::bound_t& bound)
{
    bound.x1 = ili9341.GetWidth();
    bound.y1 = ili9341.GetHeight();
    bound.x2 = 0;
    bound.y2 = 0;
}

bool Screen::SubmitShape(Graphics::Shape& shape)
{
    const Graphics::Graphics::bound_t bound = Graphics::GetBounds(shape);
    if (!ReadFlag(render_flags, RenderFlags::Rendering))
    {
        RasterizeShape(shape, bound);
        RaiseFlag(render_flags, RenderFlags::Rasterized);
    }
    else
    {
        if (Graphics::Intersecting(bound, draw_bound))
        {
            if (shapes.IsFull())
            {
                UI_LOG_ERROR("Screen::SubmitShape: shape queue full; submission rejected");
                return false;
            }
            shapes.Write(shape);
        }
        else
        {
            RasterizeShape(shape, bound);
            RaiseFlag(render_flags, RenderFlags::Rasterized);
        }
    }
    return true;
}

void Screen::RasterizeShape(Graphics::Shape& shape, const Graphics::bound_t& bound)
{
    Graphics::Rasterize(shape, data_buff, data_buff_width, data_buff_height);
    Graphics::ExpandBound(rasterize_bound, bound);
}

void Screen::ScrollText()
{
    const uint16_t dest_idx = texts_in_use < max_texts ? texts_in_use + 1 : texts_in_use;
    const uint16_t src_idx = texts_in_use < max_texts ? texts_in_use : texts_in_use - 1;

    uint16_t dest_y = texts_area_y_offset - dest_idx * text_font.height;
    const uint16_t src_y1 = texts_area_y_offset - src_idx * text_font.height;
    const uint16_t src_y2 = texts_area_y_offset;

    Graphics::Shape mem = {
        .type = Graphics::ShapeType::Mem,
        .mem =
            {
                .x1 = 0,
                .x2 = data_buff_width * 2,
                .dest_y = dest_y,
                .src_y1 = src_y1,
                .src_y2 = src_y2,
                .y_step = text_font.height,
            },
    };

    SubmitShape(mem);
}
