#pragma once

#include "battery.hh"
#include "battery_icon.hh"
#include "bitmap.hh"
#include "display_driver.hh"
#include "font.hh"
#include "graphics.hh"
#include "jetbrainsmono_10x16.hh"
#include "jetbrainsmono_15x20.hh"
#include "ring_buffer.hh"
#include "screen_buffers.hh"
#include "tile_map.hh"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <tuple>

namespace
{
static constexpr uint16_t swap_u16(const uint16_t val)
{
    return static_cast<uint16_t>(((val << 8) & 0xFF00) | ((val >> 8) & 0x00FF));
}
} // namespace

struct ScreenTextLine
{
    const std::span<char> buffer;
    uint16_t length = 0;
};

class Screen
{
public:
    static constexpr size_t Num_Icons = 5;
    static constexpr uint16_t Title_y1 = 1;

    static constexpr uint16_t Icon_Position_Y1 = Title_y1;
    static constexpr uint16_t Icon_Position_Y2 = Title_y1 + Battery_Critical_20x18::bitmap.height;

    static constexpr uint16_t Battery_Level_X1_Margin = 7;
    static constexpr uint16_t Battery_Level_X2_Margin = 4;
    static constexpr uint16_t Battery_Charge_Position_Y1 = Icon_Position_Y1 + 4;
    static constexpr uint16_t Battery_Charge_Position_Y2 = Icon_Position_Y2 - 4;

private:
    enum class Colour : uint16_t
    {
        Black = swap_u16(0x0000U),
        White = swap_u16(0xFFFFU),
        Blue = swap_u16(0x001FU),
        Red = swap_u16(0xF800U),
        Light_Green = swap_u16(0x3626U),
        Green = swap_u16(0x07E0U),
        Cyan = swap_u16(0x07FFU),
        Magenta = swap_u16(0xF81FU),
        Yellow = swap_u16(0xFFE0U),
        Grey = swap_u16(0xCE59U),
    };

    static constexpr uint16_t Colour_Map[]{
        static_cast<uint16_t>(Colour::Black),       static_cast<uint16_t>(Colour::White),
        static_cast<uint16_t>(Colour::Blue),        static_cast<uint16_t>(Colour::Red),
        static_cast<uint16_t>(Colour::Light_Green), static_cast<uint16_t>(Colour::Green),
        static_cast<uint16_t>(Colour::Cyan),        static_cast<uint16_t>(Colour::Magenta),
        static_cast<uint16_t>(Colour::Yellow),      static_cast<uint16_t>(Colour::Grey)};

public:
    Screen(ScreenBuffers* buffers,
           std::span<char> title_buffer,
           std::span<ScreenTextLine> texts,
           std::span<char> usr_buffer,
           const Battery& battery,
           TileMap& tile_map,
           DisplayDriver& display,
           const Font& title_font = JetBrainsMono15x20::font,
           const Font& text_font = JetBrainsMono10x16::font);

    enum class IconType
    {
        Mic,
        Speaker,
        Moq,
        Wifi,
        Battery,
    };

    void Init();

    void Update(const uint32_t ticks_ms);
    void Draw(const uint32_t ticks_ms);
    void StartRender();
    void Render();
    void Transfer();

    void Reset();
    void Sleep();
    void Wake();
    void EnableBacklight();
    void DisableBacklight();
    void FullRefresh();

    void FillScreen(const Graphics::Colour colour);
    void UpdateTitle(const char* title, const uint32_t len);

    void AppendText(const char* text, const uint32_t len);
    void CommitText();
    void CommitText(const char* text, const uint32_t len);

    void AppendUserText(const char* text, const uint32_t len);
    void AppendUserText(const char ch);

    void BackspaceUserText();
    void ClearUserText();

    const char* UserText() const noexcept;
    uint16_t UserTextLength() const noexcept;

    // False means the deferred queue was full; the shape was not submitted.
    bool SubmitShape(Graphics::Shape& shape);

    void DrawTitleLine();
    void DrawUserLine();

    void DrawIcon(const IconType icon_type, const bool flag);
    void BatteryLevel(const uint8_t percentage);

    bool IsRendering();

    uint16_t PixelWidth();
    uint16_t PixelHeight();

private:
    enum RenderFlags : uint32_t
    {
        Rendering = 1 << 0,
        Rasterized = 1 << 1,
    };

    enum UpdateFlags : uint32_t
    {
        CursorSolid = 1 << 0
    };

    void RotateScreen(DisplayDriver::Orientation orientation);
    void CalculateDrawAreas();
    void DefineScrollArea();
    void ScrollText();

    void RaiseFlag(uint32_t& flags, const uint32_t flag);
    void LowerFlag(uint32_t& flags, const uint32_t flag);
    uint32_t ReadFlag(const uint32_t flags, const uint32_t flag);
    uint32_t TakeFlag(uint32_t& flags);
    uint32_t TakeFlag(uint32_t& flags, const uint32_t flag);

    void DrawCursor(const bool solid);

    void RasterizeShape(Graphics::Shape& shape, const Graphics::bound_t& bound);

    ScreenBuffers* const buffers;
    const std::span<char> title_buffer;
    const std::span<ScreenTextLine> texts;
    const std::span<char> usr_buffer;
    const Battery& battery;
    TileMap& tile_map;
    DisplayDriver& display;
    const Font& title_font;
    const Font& text_font;

    const uint16_t Top_Fixed_Height;
    const uint16_t Bottom_Fixed_Height;

    Graphics::bound_t draw_bound;
    uint16_t render_height;

    uint16_t title_x1;
    uint16_t title_x2;
    size_t title_len;

    uint32_t text_write;
    uint32_t text_read;
    uint32_t texts_in_use;
    uint32_t scroll_offset;

    size_t max_texts;

    size_t usr_buffer_idx;

    RingBuffer<Graphics::Shape> shapes;

    uint16_t icons_x_offset;
    uint16_t usr_text_y_offset;
    uint16_t texts_area_y_offset;
    uint16_t scroll_area_height;

    uint32_t last_ticks_ms;

    uint32_t last_update_ticks_ms;
    uint32_t update_flags;

    uint32_t render_flags;
};

namespace screen_detail
{
template <size_t TITLE_WIDTH, typename TITLE_FONT>
struct TitleStorage
{
    static_assert(TITLE_FONT::font.width > 0);

    static constexpr size_t Capacity = TITLE_WIDTH / TITLE_FONT::font.width;
    static_assert(Capacity > 0);

    std::array<char, Capacity> title{};
};

constexpr size_t TextAreaHeight(size_t height,
                                const Font& title_font = JetBrainsMono15x20::font,
                                const Font& text_font = JetBrainsMono10x16::font) noexcept
{
    const size_t Fixed_Height = title_font.height + text_font.height + 2;
    return height >= Fixed_Height ? height - Fixed_Height : 0;
}

template <size_t TEXT_WIDTH, size_t TEXT_HEIGHT, typename TEXT_FONT>
struct TextStorage
{
    static_assert(TEXT_FONT::font.width > 0 && TEXT_FONT::font.height > 0);

    static constexpr size_t Characters_Per_Line = TEXT_WIDTH / TEXT_FONT::font.width;
    static constexpr size_t Line_Count = TEXT_HEIGHT / TEXT_FONT::font.height;

    static_assert(Characters_Per_Line > 0 && Characters_Per_Line <= UINT16_MAX);
    static_assert(Line_Count > 0);

    std::array<std::array<char, Characters_Per_Line>, Line_Count> characters{};
    std::array<ScreenTextLine, Line_Count> text_lines{};

    constexpr TextStorage() noexcept :

        text_lines(std::apply([](auto&... args) { return std::array{ScreenTextLine{args, 0}...}; },
                              characters))
    {
    }

    TextStorage(const TextStorage&) = delete;
    TextStorage& operator=(const TextStorage&) = delete;
    TextStorage(TextStorage&&) = delete;
    TextStorage& operator=(TextStorage&&) = delete;
};
template <size_t WIDTH, typename TEXT_FONT>
struct UserTextStorage
{
    static_assert(TEXT_FONT::font.width > 0);

    static constexpr size_t Capacity = WIDTH / TEXT_FONT::font.width;
    static_assert(Capacity > 0 && Capacity <= UINT16_MAX);

    std::array<char, Capacity> user{};
};
} // namespace screen_detail

template <size_t WIDTH,
          size_t HEIGHT,
          size_t QUANTIZATION,
          size_t NUM_SCAN_LINES,
          size_t NUM_SCANLINE_BUFFS,
          typename TITLE_FONT,
          typename TEXT_FONT>
class StaticScreen
    : private StaticScreenBuffers<WIDTH, HEIGHT, QUANTIZATION, NUM_SCAN_LINES, NUM_SCANLINE_BUFFS>,
      private screen_detail::TitleStorage<
          std::max(WIDTH - (Screen::Num_Icons * Battery_Critical_20x18::bitmap.width),
                   WIDTH - (Screen::Num_Icons * Battery_Critical_20x18::bitmap.width)),
          TITLE_FONT>,
      private screen_detail::TextStorage<
          std::max(WIDTH, HEIGHT),
          screen_detail::TextAreaHeight(std::max(WIDTH, HEIGHT), TITLE_FONT::font, TEXT_FONT::font),
          TEXT_FONT>,
      private screen_detail::UserTextStorage<std::max(WIDTH, HEIGHT), TEXT_FONT>,
      public Screen
{

public:
    static_assert(WIDTH > 0 && WIDTH <= UINT16_MAX);
    static_assert(HEIGHT > 0 && HEIGHT <= UINT16_MAX);
    static_assert(QUANTIZATION == 1 || QUANTIZATION == 2 || QUANTIZATION == 4 || QUANTIZATION == 8);
    static_assert(NUM_SCAN_LINES > 0 && NUM_SCAN_LINES <= UINT16_MAX);
    static_assert(NUM_SCANLINE_BUFFS > 0 && NUM_SCANLINE_BUFFS <= UINT16_MAX);
    static_assert(TITLE_FONT::font.height > 0 && TEXT_FONT::font.height > 0);
    static_assert(screen_detail::TextAreaHeight(std::min(WIDTH, HEIGHT),
                                                TITLE_FONT::font,
                                                TEXT_FONT::font)
                  >= TEXT_FONT::font.height);

    StaticScreen(const Battery& battery, TileMap& tile_map, DisplayDriver& display) :
        StaticScreenBuffers<WIDTH, HEIGHT, QUANTIZATION, NUM_SCAN_LINES, NUM_SCANLINE_BUFFS>(),
        screen_detail::TitleStorage<
            std::max(WIDTH - (Screen::Num_Icons * Battery_Critical_20x18::bitmap.width),
                     WIDTH - (Screen::Num_Icons * Battery_Critical_20x18::bitmap.width)),
            TITLE_FONT>(),
        screen_detail::TextStorage<std::max(WIDTH, HEIGHT),
                                   screen_detail::TextAreaHeight(
                                       std::max(WIDTH, HEIGHT), TITLE_FONT::font, TEXT_FONT::font),
                                   TEXT_FONT>(),
        screen_detail::UserTextStorage<std::max(WIDTH, HEIGHT), TEXT_FONT>(),
        Screen(this,
               this->title,
               this->text_lines,
               this->user,
               battery,
               tile_map,
               display,
               TITLE_FONT::font,
               TEXT_FONT::font)
    {
    }
};
