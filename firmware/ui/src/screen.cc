#include "screen.hh"
#include "audio_icon.hh"
#include "battery_icon.hh"
#include "font.hh"
#include "graphics.hh"
#include "mic_icon.hh"
#include "moq_icon.hh"
#include "wifi_icon.hh"
#include <math.h>
#include <algorithm>
#include <cstdint>
#include <cstring>

Screen::Screen(ScreenBuffers* buffers,
               std::span<char> title_buffer,
               std::span<ScreenTextLine> texts,
               std::span<char> usr_buffer,
               const Battery& battery,
               TileMap& tile_map,
               ScreenDriver& driver,
               const Font& title_font,
               const Font& text_font) :
    buffers(buffers),
    title_buffer(title_buffer),
    texts(texts),
    usr_buffer(usr_buffer),
    battery(battery),
    tile_map(tile_map),
    driver(driver),
    title_font(title_font),
    text_font(text_font),
    Top_Fixed_Height(title_font.height),
    Bottom_Fixed_Height(text_font.height + 2),
    draw_bound({driver.GetWidth(), driver.GetHeight(), 0, 0}),
    render_height(0),
    title_x1(0),
    title_x2(0),
    title_len(0),
    text_write(0),
    text_read(0),
    texts_in_use(0),
    scroll_offset(0),
    max_texts(texts.size()),
    usr_buffer_idx(0),
    shapes(20),
    icons_x_offset(0),
    usr_text_y_offset(0),
    texts_area_y_offset(0),
    scroll_area_height(0),
    last_ticks_ms(0),
    last_update_ticks_ms(0),
    update_flags(0),
    render_flags(0)
{
    CalculateDrawAreas();
}

void Screen::Init()
{
    driver.Init();
    CalculateDrawAreas();
}

void Screen::Draw(uint32_t)
{
    if (ReadFlag(render_flags, RenderFlags::Rendering))
    {
        return;
    }

    if (shapes.Unread())
    {

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

    StartRender();
}

void Screen::StartRender()
{
    draw_bound = tile_map.GetNext();

    draw_bound.x2 = std::min<uint16_t>(draw_bound.x2, driver.GetWidth());
    draw_bound.y2 = std::min<uint16_t>(draw_bound.y2, driver.GetHeight());
    if (draw_bound.x1 >= draw_bound.x2 || draw_bound.y1 >= draw_bound.y2)
    {
        LowerFlag(render_flags, RenderFlags::Rendering);
        return;
    }

    RaiseFlag(render_flags, Rendering);
    driver.BeginRegion(draw_bound.x1, draw_bound.y1, draw_bound.x2, draw_bound.y2);

    Render();
    Transfer();

    return;
}

void Screen::Update(const uint32_t ticks_ms)
{
    last_ticks_ms = ticks_ms;
    if (last_ticks_ms - last_update_ticks_ms >= 1000)
    {
        const uint32_t solid = ReadFlag(update_flags, UpdateFlags::CursorSolid);
        DrawCursor(solid);
    }
}

void Screen::Render()
{
    if (draw_bound.y1 >= draw_bound.y2)
    {
        return;
    }

    uint16_t* const scanline_ptr = buffers->scanline->Current();
    render_height =
        std::min<const uint16_t>(draw_bound.y2 - draw_bound.y1, buffers->scanline->num_lines);
    const uint16_t render_width = draw_bound.x2 - draw_bound.x1;

    const uint16_t quantization_divisor = 8 / buffers->data->quantization;
    uint16_t y_start = draw_bound.y1;
    for (uint16_t y = 0; y < render_height; ++y)
    {
        uint16_t x_start = draw_bound.x1;
        uint16_t x_end = draw_bound.x2;
        const uint16_t y_offset = y_start * buffers->data->width;

        // Determine if the x1 is a odd value
        if (x_start & 0x01)
        {
            const uint16_t data_buff_idx = (x_start / quantization_divisor) + y_offset;
            const uint8_t quant_colour =
                (buffers->data->buffer[data_buff_idx] >> (4 * ((x_start ^ 0x0001) & 0x0001)))
                & 0x0F;
            const uint16_t real_colour = Colour_Map[quant_colour];

            scanline_ptr[(x_start - draw_bound.x1) + y * render_width] = real_colour;

            ++x_start;
        }

        if (x_end & 0x01)
        {
            --x_end;

            const uint16_t data_buff_idx = (x_end / quantization_divisor) + y_offset;
            const uint8_t quant_colour =
                (buffers->data->buffer[data_buff_idx] >> (4 * ((x_end ^ 0x0001) & 0x0001))) & 0x0F;
            const uint16_t real_colour = Colour_Map[quant_colour];

            scanline_ptr[(x_end - draw_bound.x1) + y * render_width] = real_colour;
        }

        const uint16_t y_render_offset = y * render_width;
        for (uint16_t x = x_start; x < x_end; x += 2)
        {
            const uint16_t data_buff_idx = (x / quantization_divisor) + y_offset;
            const uint8_t quant_colour = buffers->data->buffer[data_buff_idx];
            const uint16_t upper_colour = Colour_Map[quant_colour >> 4];
            const uint16_t lower_colour = Colour_Map[quant_colour & 0x0F];
            const size_t idx = (x - draw_bound.x1) + y_render_offset;

            scanline_ptr[idx] = upper_colour;
            scanline_ptr[idx + 1] = lower_colour;
        }

        ++y_start;
    }
}

void Screen::Transfer()
{
    if (draw_bound.y1 >= draw_bound.y2)
    {
        driver.EndRegion();
        StartRender();
        return;
    }

    const uint16_t render_width = draw_bound.x2 - draw_bound.x1;
    const uint16_t render_height =
        std::min<const uint16_t>(draw_bound.y2 - draw_bound.y1, buffers->scanline->num_lines);

    uint16_t* const scanline_ptr = buffers->scanline->Current();
    const auto* pixels = reinterpret_cast<const uint8_t*>(scanline_ptr);
    const size_t byte_count = size_t{render_width} * render_height * sizeof(uint16_t);
    if (driver.WritePixels({pixels, byte_count}))
    {
        draw_bound.y1 += render_height;

        buffers->scanline->Advance();
    }
    else
    {
        LowerFlag(render_flags, RenderFlags::Rendering);
        driver.EndRegion();
        FullRefresh();
    }
}

void Screen::EnableBacklight()
{
    driver.EnableBacklight();
}

void Screen::DisableBacklight()
{
    driver.DisableBacklight();
}

void Screen::FullRefresh()
{
    tile_map.MarkAll();
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
                .x2 = driver.GetWidth(),
                .y2 = driver.GetHeight(),
                .colour = colour,
            },
    };

    SubmitShape(rec);
}

void Screen::UpdateTitle(const char* title, const uint32_t len)
{
    title_len = std::min<size_t>({len, title_buffer.size(), UINT16_MAX});
    const uint16_t visible_length =
        std::min<size_t>(title_len, driver.GetWidth() / title_font.width);
    for (size_t i = 0; i < title_len; ++i)
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
                .x2 = std::min(title_x2, driver.GetWidth()),
                .y2 = static_cast<const uint16_t>(Title_y1 + title_font.height),
                .colour = Graphics::Colour::Black,
            },
    };

    SubmitShape(clear_rec);

    const uint16_t next_x2 = title_x1 + visible_length * title_font.width;
    title_x2 = next_x2 > title_x2 ? next_x2 : title_x2;

    Graphics::Shape title_shape = {
        .type = Graphics::ShapeType::String,
        .string =
            {
                .str = title_buffer.data(),
                .len = visible_length,
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
    if (max_texts == 0)
    {
        return;
    }

    ScreenTextLine& line = texts[text_write];
    const size_t capacity = std::min<size_t>(
        {line.buffer.size(), size_t(driver.GetWidth() / text_font.width), UINT16_MAX});

    size_t idx = 0;
    while (line.length < capacity && idx < len)
    {
        line.buffer[line.length++] = text[idx++];
    }
}

void Screen::CommitText()
{
    if (max_texts == 0)
    {
        return;
    }

    ScrollText();

    Graphics::Shape str = {
        .type = Graphics::ShapeType::String,
        .string =
            {
                .str = texts[text_read].buffer.data(),
                .len = static_cast<uint16_t>(std::min<size_t>(
                    texts[text_read].length, driver.GetWidth() / text_font.width)),
                .x = 0,
                .y = static_cast<uint16_t>(texts_area_y_offset - text_font.height),
                .font = &text_font,
                .foreground = Graphics::Colour::White,
                .background = Graphics::Colour::Black,
            },
    };

    SubmitShape(str);

    if (texts_in_use < texts.size())
    {
        ++texts_in_use;
    }
    else
    {
    }

    // Keep history indices tied to storage, not the orientation's visible line count.
    ++text_write;
    if (text_write >= texts.size())
    {
        text_write = 0;
    }
    ++text_read;
    if (text_read >= texts.size())
    {
        text_read = 0;
    }

    // Clear the next write
    texts[text_write].length = 0;
}

void Screen::CommitText(const char* text, const uint32_t len)
{
    AppendText(text, len);
    CommitText();
}

void Screen::AppendUserText(const char* text, const uint32_t len)
{
    const size_t capacity =
        std::min<size_t>(usr_buffer.size(), driver.GetWidth() / text_font.width);
    if (usr_buffer_idx >= capacity)
    {
        return;
    }

    const uint32_t num_char = std::min<size_t>(len, capacity - usr_buffer_idx);
    for (uint32_t i = 0; i < num_char; ++i)
    {
        usr_buffer[usr_buffer_idx + i] = text[i];
    }

    const uint16_t x1 = static_cast<uint16_t>(usr_buffer_idx * text_font.width);
    Graphics::Shape str = {
        .type = Graphics::ShapeType::String,
        .string =
            {
                .str = usr_buffer.data() + usr_buffer_idx,
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
    const size_t capacity =
        std::min<size_t>(usr_buffer.size(), driver.GetWidth() / text_font.width);
    if (usr_buffer_idx >= capacity)
    {
        return;
    }

    usr_buffer[usr_buffer_idx] = ch;

    const uint16_t x1 = static_cast<uint16_t>(usr_buffer_idx * text_font.width);
    Graphics::Shape str = {
        .type = Graphics::ShapeType::String,
        .string =
            {
                .str = usr_buffer.data() + usr_buffer_idx,
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

    usr_buffer[--usr_buffer_idx] = '\0';
    if (usr_buffer_idx >= driver.GetWidth() / text_font.width)
    {
        return;
    }

    const uint16_t x1 = usr_buffer_idx * text_font.width;
    const uint16_t x2 = std::min<uint16_t>(x1 + text_font.width * 2, driver.GetWidth());
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

    DrawCursor(true);
}

void Screen::ClearUserText()
{
    const uint16_t x2 = driver.GetWidth();
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

    std::fill_n(usr_buffer.begin(), usr_buffer_idx, '\0');
    usr_buffer_idx = 0;

    DrawCursor(true);
}

const char* Screen::UserText() const noexcept
{
    return usr_buffer.data();
}

uint16_t Screen::UserTextLength() const noexcept
{
    return usr_buffer_idx;
}

void Screen::RotateScreen(ScreenDriver::Orientation orientation)
{
    driver.Rotate(orientation);
    CalculateDrawAreas();
}

void Screen::CalculateDrawAreas()
{
    const uint16_t height = driver.GetHeight();
    usr_text_y_offset = height - std::min<uint16_t>(height, text_font.height);
    texts_area_y_offset = height - std::min(height, Bottom_Fixed_Height);
    scroll_area_height = texts_area_y_offset - std::min(texts_area_y_offset, Top_Fixed_Height);

    max_texts = std::min<size_t>(scroll_area_height / text_font.height, texts.size());
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
    const uint16_t line_y1 = Top_Fixed_Height - 1;
    const uint16_t line_x2 = driver.GetWidth();
    const uint16_t line_y2 = Top_Fixed_Height;
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
    const uint16_t line_y1 = driver.GetHeight() - Bottom_Fixed_Height;
    const uint16_t line_x2 = driver.GetWidth();
    const uint16_t line_y2 = driver.GetHeight() - Bottom_Fixed_Height + 1;
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
    uint16_t x = driver.GetWidth();

    switch (icon_type)
    {
    case IconType::Speaker:
        x -= (Audio_Up_20x18::bitmap.width + margin + Mic_Up_20x18::bitmap.width + margin
              + Moq_Up_20x18::bitmap.width + margin + Wifi_Up_20x18::bitmap.width + margin
              + Battery_Critical_20x18::bitmap.width + margin);
        map = flag ? &Audio_Up_20x18::bitmap : &Audio_Down_20x18::bitmap;
        break;
    case IconType::Mic:
        x -= (Mic_Up_20x18::bitmap.width + margin + Moq_Up_20x18::bitmap.width + margin
              + Wifi_Up_20x18::bitmap.width + margin + Battery_Critical_20x18::bitmap.width
              + margin);
        map = flag ? &Mic_Up_20x18::bitmap : &Mic_Down_20x18::bitmap;
        break;
    case IconType::Moq:
        x -= (Moq_Up_20x18::bitmap.width + margin + Wifi_Up_20x18::bitmap.width + margin
              + Battery_Critical_20x18::bitmap.width + margin);
        map = flag ? &Moq_Up_20x18::bitmap : &Moq_Down_20x18::bitmap;
        break;
    case IconType::Wifi:
        x -= (Wifi_Up_20x18::bitmap.width + margin + Battery_Critical_20x18::bitmap.width + margin);
        map = flag ? &Wifi_Up_20x18::bitmap : &Wifi_Down_20x18::bitmap;
        break;
    case IconType::Battery:
        x -= (Battery_Critical_20x18::bitmap.width + margin);
        map = battery.GetIcon();
        break;
    default:
        return;
    }

    const uint16_t y = Icon_Position_Y1;
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
}

bool Screen::IsRendering()
{
    return ReadFlag(render_flags, RenderFlags::Rendering);
}

uint16_t Screen::PixelWidth()
{
    return driver.GetWidth();
}

uint16_t Screen::PixelHeight()
{
    return driver.GetHeight();
}

void Screen::DrawCursor(const bool solid)
{
    if (usr_buffer_idx >= driver.GetWidth() / text_font.width)
    {
        return;
    }

    const uint16_t x1 = static_cast<uint16_t>(usr_buffer_idx * text_font.width);
    const uint16_t x2 = std::min<uint16_t>(x1 + text_font.width, driver.GetWidth());
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
        LowerFlag(update_flags, UpdateFlags::CursorSolid);
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
        RaiseFlag(update_flags, UpdateFlags::CursorSolid);
    }

    last_update_ticks_ms = last_ticks_ms;
}

bool Screen::SubmitShape(Graphics::Shape& shape)
{
    const Graphics::Graphics::bound_t bound = Graphics::GetBounds(shape);
    if (!ReadFlag(render_flags, RenderFlags::Rendering))
    {
        if (shapes.Unread() > 0)
        {
            shapes.Write(shape);
        }
        else
        {
            RasterizeShape(shape, bound);
            RaiseFlag(render_flags, RenderFlags::Rasterized);
        }
    }
    else
    {
        if (Graphics::Intersecting(bound, draw_bound))
        {
            if (shapes.IsFull())
            {
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
    Graphics::Rasterize(shape, buffers->data->buffer, buffers->data->width, buffers->data->height);

    tile_map.Mark(bound);
}

void Screen::ScrollText()
{
    const size_t visible_texts = std::min<size_t>(texts_in_use, max_texts);
    const uint16_t dest_idx = visible_texts < max_texts ? visible_texts + 1 : visible_texts;
    const uint16_t src_idx = visible_texts < max_texts ? visible_texts : visible_texts - 1;

    uint16_t dest_y = texts_area_y_offset - dest_idx * text_font.height;
    const uint16_t src_y1 = texts_area_y_offset - src_idx * text_font.height;
    const uint16_t src_y2 = texts_area_y_offset;

    Graphics::Shape mem = {
        .type = Graphics::ShapeType::Mem,
        .mem =
            {
                .x1 = 0,
                .x2 = static_cast<uint16_t>(buffers->data->width * 2),
                .dest_y = dest_y,
                .src_y1 = src_y1,
                .src_y2 = src_y2,
                .y_step = text_font.height,
            },
    };

    SubmitShape(mem);
}
