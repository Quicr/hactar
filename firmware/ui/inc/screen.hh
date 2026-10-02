#pragma once

#include "app_main.hh"
#include "font.hh"
#include "stm32.h"
#include <type_traits>

enum class Colour : uint8_t
{
    Black = 0,
    White,
    Blue,
    Red,
    Light_Green,
    Green,
    Cyan,
    Magenta,
    Yellow,
    Grey,
};

class Screen
{
public:
    // Some basic colours (RGB565)
    static constexpr uint16_t C_BLACK = 0x0000U;
    static constexpr uint16_t C_WHITE = 0xFFFFU;
    static constexpr uint16_t C_BLUE = 0x001FU;
    static constexpr uint16_t C_RED = 0xF800U;
    static constexpr uint16_t C_LIGHT_GREEN = 0x3626U;
    static constexpr uint16_t C_GREEN = 0x07E0U;
    static constexpr uint16_t C_CYAN = 0x07FFU;
    static constexpr uint16_t C_MAGENTA = 0xF81FU;
    static constexpr uint16_t C_YELLOW = 0xFFE0U;
    static constexpr uint16_t C_GREY = 0xCE59U;

    // Default orientation
    static constexpr uint8_t WIDTH = 240;
    static constexpr uint16_t HEIGHT = 320;

    static constexpr uint32_t Num_Rows = 10;
    // TODO find a sweet spot for num memories
    static constexpr uint32_t Num_Memories = 50;
    static constexpr uint32_t Memory_Size = 32;
    static constexpr uint32_t Title_Length = 21;
    static constexpr uint32_t Max_Texts = 35;
    static constexpr uint32_t Max_Characters = 48;
    static constexpr uint16_t Top_Fixed_Area = 20;
    static constexpr uint16_t Bottom_Fixed_Area = 20;
    static constexpr uint16_t User_Text_Height_Offset = HEIGHT - Bottom_Fixed_Area;
    static constexpr uint16_t Scroll_Area_Height = HEIGHT - (Top_Fixed_Area + Bottom_Fixed_Area);
    static constexpr uint16_t Scroll_Area_Top = Top_Fixed_Area;
    static constexpr uint16_t Scroll_Area_Bottom = HEIGHT - Bottom_Fixed_Area;
    static constexpr uint32_t Half_Width_Pixel_Size = WIDTH / 2;
    static constexpr uint32_t Width_Pixel_Size = WIDTH * 2;
    static constexpr uint32_t Scan_Window_Dbl_Sz = Width_Pixel_Size * 2;

private:
    struct YBound
    {
        uint16_t y1 = 0;
        uint16_t y2 = 0;
    };

    enum class MemoryStatus : uint8_t
    {
        Free = 0,
        In_Progress,
    };

    static constexpr uint16_t Colour_Map[]{C_BLACK, C_WHITE, C_BLUE,    C_RED,    C_LIGHT_GREEN,
                                           C_GREEN, C_CYAN,  C_MAGENTA, C_YELLOW, C_GREY};

    struct DrawMemory;

    typedef bool (*MemoryCallback)(DrawMemory& memory,
                                   uint8_t matrix[HEIGHT][Half_Width_Pixel_Size],
                                   const int16_t y1,
                                   const int16_t y2);

    struct DrawMemory
    {
        MemoryCallback callback = nullptr;
        MemoryStatus status = MemoryStatus::Free;
        uint16_t x1 = 0;
        uint16_t x2 = 0;
        uint16_t y1 = 0;
        uint16_t y2 = 0;
        Colour colour = Colour::Black;
        uint8_t write_idx = 0;
        uint8_t read_idx = 0;
        // A bunch of data params to run the next command
        uint8_t parameters[Memory_Size]{0};
        // 4+1+2+2+2+2+1+1+1+32 = 48 bytes
    };

public:
    enum Orientation
    {
        portrait,
        flipped_portrait,
        left_landscape,
        right_landscape
    };

    Screen(SPI_HandleTypeDef& hspi,
           GPIO_TypeDef* cs_port,
           const uint16_t cs_pin,
           GPIO_TypeDef* dc_port,
           const uint16_t dc_pin,
           GPIO_TypeDef* rst_port,
           const uint16_t rst_pin,
           GPIO_TypeDef* bl_port,
           const uint16_t bl_pin,
           Orientation orientation);

    void Init();
    void Draw(uint32_t timeout);
    void Reset();
    void Sleep();
    void Wake();
    void EnableBacklight();
    void DisableBacklight();
    inline void Select();
    inline void Deselect();

    void SetOrientation(const Orientation orientation);
    void FillRectangle(uint16_t x1, uint16_t x2, uint16_t y1, uint16_t y2, const Colour colour);
    void FillScreen(const Colour colour);
    void DrawRectangle(uint16_t x1,
                       uint16_t x2,
                       uint16_t y1,
                       uint16_t y2,
                       const uint16_t thickness,
                       const Colour colour);
    void DrawCharacter(
        uint16_t x, uint16_t y, const char ch, const Font& font, const Colour fg, const Colour bg);
    void DrawString(uint16_t x,
                    uint16_t y,
                    const char* str,
                    const uint16_t length,
                    const Font& font,
                    const Colour fg,
                    const Colour bg);
    void DefineScrollArea(const uint16_t tfa_idx, const uint16_t vsa_idx, const uint16_t bfa_idx);
    void ScrollScreen(const uint16_t scroll_idx, bool up);

    void UpdateTitle(const char* title, const uint32_t len);

    // Do I need to do this?
    void AppendText(const char* text, const uint32_t len);
    void CommitText();
    void CommitText(const char* text, const uint32_t len);

    void AppendUserText(const char* text, const uint32_t len);
    void AppendUserText(const char ch);

    void BackspaceUserText();
    void ClearUserText();

    const char* UserText() const noexcept;
    uint16_t UserTextLength() const noexcept;

private:
    inline void WaitForSPIComplete();
    inline void SetPinToCommand();
    inline void SetPinToData();
    void WriteCommand(uint8_t cmd);
    void WriteCommand(uint8_t cmd, uint8_t* data, const uint32_t sz);
    void WriteDataWithSet(uint8_t data);
    void WriteDataWithSet(uint8_t* data, const uint32_t data_size);
    void WriteData(uint8_t data);
    void WriteData(uint8_t* data, const uint32_t data_size);
    void SetWriteablePixels(const int16_t x1, const int16_t x2, const int16_t y1, const int16_t y2);
    DrawMemory& AllocateMemory(const uint16_t x1,
                               const uint16_t x2,
                               const uint16_t y1,
                               const uint16_t y2,
                               const Colour colour,
                               MemoryCallback callback);
    void NormalMode();

    // Private functions
    static bool DrawRectangleProcedure(DrawMemory& memory,
                                       uint8_t matrix[HEIGHT][Half_Width_Pixel_Size],
                                       const int16_t y1,
                                       const int16_t y2);
    void DrawRectangleProcedure(const int16_t x1,
                                const int16_t x2,
                                const int16_t y1,
                                const int16_t y2,
                                const uint16_t thickness,
                                const Colour colour);

    static bool FillRectangleProcedure(DrawMemory& memory,
                                       uint8_t matrix[HEIGHT][Half_Width_Pixel_Size],
                                       const int16_t y1,
                                       const int16_t y2);
    static bool DrawCharacterProcedure(DrawMemory& memory,
                                       uint8_t matrix[HEIGHT][Half_Width_Pixel_Size],
                                       const int16_t y1,
                                       const int16_t y2);
    static bool DrawStringProcedure(DrawMemory& memory,
                                    uint8_t matrix[HEIGHT][Half_Width_Pixel_Size],
                                    const int16_t y1,
                                    const int16_t y2);

    inline void HandleBounds(uint16_t& x1, uint16_t& x2, uint16_t& y1, uint16_t& y2);

    static inline uint8_t* GetCharAddr(uint8_t* font_data,
                                       const uint8_t ch,
                                       const uint16_t font_width,
                                       const uint16_t font_height);
    static inline void
    PushMemoryParameter(DrawMemory& memory, const uint32_t val, const int16_t num_bytes);
    // Non-destructive retrieval
    template <typename T, typename std::enable_if<std::is_integral<T>::value, bool>::type = 0>
    static inline T PullMemoryParameter(DrawMemory& memory)
    {
        const int16_t bytes = sizeof(T);
        T output = 0;
        uint8_t& idx = memory.read_idx;

        for (int16_t i = 0; i < bytes && i < memory.write_idx; ++i)
        {
            output |= memory.parameters[idx] << (8 * i);
            ++idx;
        }

        // Restart where we read from if the index is zero
        if (idx >= memory.write_idx)
        {
            idx = 0;
        }

        return output;
    }
    static inline void FillMatrixAtIdx(uint8_t matrix[HEIGHT][Half_Width_Pixel_Size],
                                       const uint16_t i,
                                       const uint16_t j,
                                       const uint8_t colour_high,
                                       const uint8_t colour_low) __attribute__((always_inline));
    static inline void
    FillLineAtIdx(uint8_t* line, const uint16_t i, const uint16_t j, const uint16_t colour)
        __attribute__((always_inline));
    static inline YBound
    GetYBounds(const uint16_t y1, const uint16_t y2, const uint16_t mem_y1, const uint16_t mem_y2)
        __attribute__((always_inline));

    const Font& title_font = font11x16;
    const Font& text_font = font5x8;

    // Variables
    SPI_HandleTypeDef* spi;

    GPIO_TypeDef* cs_port;
    const uint16_t cs_pin;
    GPIO_TypeDef* dc_port;
    const uint16_t dc_pin;
    GPIO_TypeDef* rst_port;
    const uint16_t rst_pin;
    GPIO_TypeDef* bl_port;
    const uint16_t bl_pin;

    Orientation orientation;
    uint16_t view_height;
    uint16_t view_width;
    uint16_t row_bytes;

    DrawMemory memories[Num_Memories];
    uint32_t memory_read_idx;
    uint32_t memories_in_use;
    uint32_t memory_write_idx;
    uint16_t row;
    uint16_t end_row;

    uint16_t row_start_update;
    uint16_t row_end_update;

    bool updating;
    bool restart_update;

    // Each byte stores two pixels
    // TODO 1d array so that I can dynamically change the orientation
    uint8_t matrix[HEIGHT][WIDTH / 2];

    // Double buff
    uint8_t scan_window[Scan_Window_Dbl_Sz];

    // Window: 0-20px
    // Max 21 characters
    char title_buffer[Title_Length];

    // Window: 20 - 308px
    uint32_t text_idx;
    uint32_t texts_in_use;
    uint32_t scroll_offset;
    char text_buffer[Max_Texts][Max_Characters];
    char text_lens[Max_Texts];

    // Window: 308-320px
    char usr_buffer[Max_Characters];
    char usr_buffer_idx;

    static constexpr uint8_t Software_Reset = 0x01U;
    static constexpr uint8_t Power_Control_A = 0xCBU;
    static constexpr uint8_t Power_Control_B = 0xCFU;
    static constexpr uint8_t Driver_Timing_Control_A = 0xE8U;
    static constexpr uint8_t Driver_Timing_Control_B = 0xEAU;
    static constexpr uint8_t Power_On_Sequence_Control = 0xEDU;
    static constexpr uint8_t Pump_Ratio_Control = 0xF7U;
    static constexpr uint8_t Power_Control_VRH = 0xC0U;
    static constexpr uint8_t Power_Control_SAP_BT = 0xC1U;
    static constexpr uint8_t VCOM_Control_1 = 0xC5U;
    static constexpr uint8_t VCOM_Control_2 = 0xC7U;
    static constexpr uint8_t Memory_Access_Control = 0x36U;
    static constexpr uint8_t Pixel_Format = 0x3AU;
    static constexpr uint8_t Frame_Rate_Control = 0xB1U;
    static constexpr uint8_t Display_Function_Control = 0xB6U;
    static constexpr uint8_t Gamma_Function_3 = 0xF2U;
    static constexpr uint8_t Gamma_Curve_Select = 0x26U;
    static constexpr uint8_t Positive_Gamma_Correction = 0xE0U;
    static constexpr uint8_t Negative_Gamma_Correction = 0xE1U;
    static constexpr uint8_t Exit_Sleep = 0x11U;
    static constexpr uint8_t Display_On = 0x29U;

    static constexpr uint8_t Column_Address_Set = 0x2AU;
    static constexpr uint8_t Row_Address_Set = 0x2BU;
    static constexpr uint8_t Memory_Write = 0x2CU;
    static constexpr uint8_t Normal_Display_Mode_On = 0x13U;
    static constexpr uint8_t Vertical_Scroll_Definition = 0x33U;
    static constexpr uint8_t Vertical_Scroll_Start_Address = 0x37U;

    // Memory access control bits
    static constexpr uint8_t Mirror_Y = 0x80U;
    static constexpr uint8_t Mirror_X = 0x40U;
    static constexpr uint8_t Swap_XY = 0x20U;
    static constexpr uint8_t Vertical_Refresh_Order = 0x10U;
    static constexpr uint8_t RGB_Order = 0x00U;
    static constexpr uint8_t BGR_Order = 0x08U;
    static constexpr uint8_t Horizontal_Refresh_Order = 0x04U;

    static constexpr uint8_t Portrait_Data = Mirror_X | BGR_Order;
    static constexpr uint8_t Flipped_Portrait_Data = Mirror_Y | BGR_Order;
    static constexpr uint8_t Left_Landscape_Data = Swap_XY | BGR_Order;
    static constexpr uint8_t Right_Landscape_Data = Mirror_X | Mirror_Y | Swap_XY | BGR_Order;
};

// 44226 bytes approximately.
