#pragma once

#include "battery.hh"
#include "battery_icon.hh"
#include "bitmap.hh"
#include "font.hh"
#include "graphics.hh"
#include "ili9341.hh"
#include "jetbrainsmono_10x16.hh"
#include "jetbrainsmono_15x20.hh"
#include "ring_buffer.hh"
#include "screen_buffers.hh"
#include "stm32.h"
#include "tile_map.hh"
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace
{
static constexpr uint16_t swap_u16(const uint16_t val)
{
    return static_cast<uint16_t>(((val << 8) & 0xFF00) | ((val >> 8) & 0x00FF));
}
} // namespace

class Screen
{
public:
    static constexpr uint16_t Title_y1 = 1;
    static constexpr uint32_t Max_Characters = 320 / JetBrainsMono10x16::font.width;
    static constexpr uint16_t Top_Fixed_Height = JetBrainsMono15x20::font.height;
    static constexpr uint16_t Bottom_Fixed_Height = JetBrainsMono10x16::font.height + 2;
    static constexpr uint32_t Max_Texts =
        (320 - Top_Fixed_Height - Bottom_Fixed_Height) / JetBrainsMono10x16::font.height;

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
           const Battery& battery,
           TileMap& tile_map,
           ILI9341& ili9341);

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

    void RotateScreen(ILI9341::Orientation orientation);
    void CalculateDrawAreas();
    void DefineScrollArea();
    void ScrollText();

    // TODO should make a flag helper class, as we have a lot of flag raising
    // through out the program
    void RaiseFlag(uint32_t& flags, const uint32_t flag);
    void LowerFlag(uint32_t& flags, const uint32_t flag);
    uint32_t ReadFlag(const uint32_t flags, const uint32_t flag);
    uint32_t TakeFlag(uint32_t& flags);
    uint32_t TakeFlag(uint32_t& flags, const uint32_t flag);

    void DrawCursor(const bool solid);

    void RasterizeShape(Graphics::Shape& shape, const Graphics::bound_t& bound);

    // Private functions
    static constexpr Font title_font = JetBrainsMono15x20::font;
    static constexpr Font text_font = JetBrainsMono10x16::font;

    // Variables
    ScreenBuffers* const buffers;

    const Battery& battery;
    TileMap& tile_map;

    ILI9341& ili9341;

    Graphics::bound_t draw_bound;
    uint16_t render_height;

    uint16_t title_x1;
    uint16_t title_x2;

    const std::span<char> title_buffer;
    size_t title_len;

    uint32_t text_write;
    uint32_t text_read;
    uint32_t texts_in_use;
    uint32_t scroll_offset;

    char text_buffer[Max_Texts][Max_Characters];
    char text_lens[Max_Texts];
    size_t max_texts;

    char usr_buffer[Max_Characters];
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

namespace
{
template <size_t TITLE_WIDTH, typename TITLE_FONT>
struct TitleStorage
{
    static_assert(TITLE_FONT::font.width > 0);

    static constexpr size_t Capacity = TITLE_WIDTH / TITLE_FONT::font.width;
    static_assert(Capacity > 0);

    std::array<char, Capacity> title{};
};
} // namespace

template <size_t WIDTH,
          size_t HEIGHT,
          size_t QUANTIZATION,
          size_t NUM_SCAN_LINES,
          size_t NUM_SCANLINE_BUFFS>
class StaticScreen
    : private StaticScreenBuffers<WIDTH, HEIGHT, QUANTIZATION, NUM_SCAN_LINES, NUM_SCANLINE_BUFFS>,
      private TitleStorage<WIDTH, JetBrainsMono15x20>,
      public Screen
{

public:
    static_assert(WIDTH > 0 && WIDTH <= UINT16_MAX);
    static_assert(HEIGHT > 0 && HEIGHT <= UINT16_MAX);
    static_assert(QUANTIZATION == 1 || QUANTIZATION == 2 || QUANTIZATION == 4 || QUANTIZATION == 8);
    static_assert(NUM_SCAN_LINES > 0 && NUM_SCAN_LINES <= UINT16_MAX);
    static_assert(NUM_SCANLINE_BUFFS > 0 && NUM_SCANLINE_BUFFS <= UINT16_MAX);

    StaticScreen(const Battery& battery, TileMap& tile_map, ILI9341& ili9341) :
        StaticScreenBuffers<WIDTH, HEIGHT, QUANTIZATION, NUM_SCAN_LINES, NUM_SCANLINE_BUFFS>(),
        TitleStorage<WIDTH, JetBrainsMono15x20>(),
        Screen(this, this->title, battery, tile_map, ili9341)
    {
    }
};
