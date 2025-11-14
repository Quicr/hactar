// #pragma once

// #include "app_main.hh"
// #include "font.hh"
// #include "stm32.h"
// #include <type_traits>

// // TODO move into screen as constexpr
// #define SF_RST 0x01U // Software reset
// #define PWRC_A 0xCBU // Power control A
// #define PWRC_B 0xCFU // Power control B
// #define TIMC_A 0xE8U // Timer control A
// #define TIMC_B 0xEAU // Timer control B
// #define PWR_ON 0xEDU // Power on sequence control
// #define PMP_RA 0xF7U // Pump ratio command
// #define PC_VRH 0xC0U // Power control VRH[5:0]
// #define PC_SAP 0xC1U // Power control SAP[2:0];BT[3:0]
// #define VCM_C1 0xC5U // VCM Control 1
// #define VCM_C2 0xC7U // VCM Control 2
// #define MEM_CR 0x36U // Memory access control
// #define PIX_FM 0x3AU // Pixel format
// #define FR_CTL 0xB1U // Frame ratio control. RGB Color
// #define DIS_CT 0xB6U // Display function control
// #define GAMM_3 0xF2U // 3 Gamma function display
// #define GAMM_C 0x26U // Gamma curve selected
// #define GAM_PC 0xE0U // Positive gamma correction
// #define GAM_NC 0xE1U // Negative gamma correction
// #define END_SL 0x11U // Exit sleep
// #define DIS_ON 0x29U // Display on
// #define MAD_CT 0x36U // Rotation control

// #define CA_SET 0x2AU // Column address set
// #define RA_SET 0x2BU // Row address set
// #define WR_RAM 0x2CU // Write to RAM

// #define NORON 0x13U

// #define MAD_CTL_MY 0x80U
// #define MAD_CTL_MX 0x40U
// #define MAD_CTL_MV 0x20U
// #define MAD_CTL_ML 0x10U
// #define MAD_CTL_RGB 0x00U
// #define MAD_CTL_BGR 0x08U
// #define MAD_CTL_MH 0x04U

// // Vertical scroll definition
// #define VSCRDEF 0x33U

// // Vertical scroll address
// #define VSCRSADD 0x37U

// // Some basic colours
// #define C_BLACK 0x0000U
// #define C_WHITE 0xFFFFU
// #define C_BLUE 0x001FU
// #define C_RED 0xF800U
// #define C_LIGHT_GREEN 0x3626U
// #define C_GREEN 0x07E0U
// #define C_CYAN 0x07FFU
// #define C_MAGENTA 0xF81FU
// #define C_YELLOW 0xFFE0U
// #define C_GREY 0xCE59U

// // Default orientation
// #define WIDTH uint16_t(240)
// #define HEIGHT uint16_t(320)
// #define PORTRAIT_DATA (MAD_CTL_MX | MAD_CTL_BGR)
// #define FLIPPED_PORTRAIT_DATA (MAD_CTL_MY | MAD_CTL_BGR)
// #define LEFT_LANDSCAPE_DATA (MAD_CTL_MV | MAD_CTL_BGR)
// #define RIGHT_LANDSCAPE_DATA (MAD_CTL_MX | MAD_CTL_MY | MAD_CTL_MV | MAD_CTL_BGR)

// // enum Colour: uint16_t
// // {
// //     NOP = 0x0000U,
// //     Black = 0x0001U,
// //     White = 0xFFFFU,
// //     Red = 0xF800U,
// //     Blue = 0x001FU,
// //     Light_Green = 0x3626U,
// //     Green = 0x07E0U,
// //     Cyan = 0x07FFU,
// //     Magenta = 0xF81FU,
// //     Yellow = 0xFFE0U,
// //     Grey = 0xCE59U,
// // };

// enum class Colour : uint8_t
// {
//     Black = 0,
//     White,
//     Blue,
//     Red,
//     Light_Green,
//     Green,
//     Cyan,
//     Magenta,
//     Yellow,
//     Grey,
// };

// class Screen
// {
// public:
//     static constexpr uint32_t Num_Rows = 10;
//     // TODO find a sweet spot for num memories
//     static constexpr uint32_t Num_Memories = 50;
//     static constexpr uint32_t Memory_Size = 32;
//     static constexpr uint32_t Title_Length = 21;
//     static constexpr uint32_t Max_Texts = 35;
//     static constexpr uint32_t Max_Characters = 48;
//     static constexpr uint16_t Top_Fixed_Area = 20;
//     static constexpr uint16_t Bottom_Fixed_Area = 20;
//     static constexpr uint16_t User_Text_Height_Offset = HEIGHT - Bottom_Fixed_Area;
//     static constexpr uint16_t Scroll_Area_Height = HEIGHT - (Top_Fixed_Area + Bottom_Fixed_Area);
//     static constexpr uint16_t Scroll_Area_Top = Top_Fixed_Area;
//     static constexpr uint16_t Scroll_Area_Bottom = HEIGHT - Bottom_Fixed_Area;
//     static constexpr uint32_t Half_Width_Pixel_Size = WIDTH / 2;
//     static constexpr uint32_t Width_Pixel_Size = WIDTH * 2;
//     static constexpr uint32_t Scan_Window_Dbl_Sz = Width_Pixel_Size * 2;

// private:
//     struct YBound
//     {
//         uint16_t y1 = 0;
//         uint16_t y2 = 0;
//     };

//     enum class MemoryStatus : uint8_t
//     {
//         Free = 0,
//         In_Progress,
//     };

//     static constexpr uint16_t Colour_Map[]{C_BLACK, C_WHITE, C_BLUE,    C_RED,    C_LIGHT_GREEN,
//                                            C_GREEN, C_CYAN,  C_MAGENTA, C_YELLOW, C_GREY};

//     struct DrawMemory;

//     typedef bool (*MemoryCallback)(DrawMemory& memory,
//                                    uint8_t matrix[HEIGHT][Half_Width_Pixel_Size],
//                                    const int16_t y1,
//                                    const int16_t y2);

//     struct DrawMemory
//     {
//         MemoryCallback callback = nullptr;
//         MemoryStatus status = MemoryStatus::Free;
//         uint16_t x1 = 0;
//         uint16_t x2 = 0;
//         uint16_t y1 = 0;
//         uint16_t y2 = 0;
//         Colour colour = Colour::Black;
//         uint8_t write_idx = 0;
//         uint8_t read_idx = 0;
//         // A bunch of data params to run the next command
//         uint8_t parameters[Memory_Size]{0};
//         // 4+1+2+2+2+2+1+1+1+32 = 48 bytes
//     };

// public:
//     enum Orientation
//     {
//         portrait,
//         flipped_portrait,
//         left_landscape,
//         right_landscape
//     };

//     Screen(SPI_HandleTypeDef& hspi,
//            GPIO_TypeDef* cs_port,
//            const uint16_t cs_pin,
//            GPIO_TypeDef* dc_port,
//            const uint16_t dc_pin,
//            GPIO_TypeDef* rst_port,
//            const uint16_t rst_pin,
//            GPIO_TypeDef* bl_port,
//            const uint16_t bl_pin,
//            Orientation orientation);

//     void Init();
//     void Draw(uint32_t timeout);
//     void Reset();
//     void Sleep();
//     void Wake();
//     void EnableBacklight();
//     void DisableBacklight();
//     inline void Select();
//     inline void Deselect();

//     void SetOrientation(const Orientation orientation);
//     void FillRectangle(uint16_t x1, uint16_t x2, uint16_t y1, uint16_t y2, const Colour colour);
//     void FillScreen(const Colour colour);
//     void DrawRectangle(uint16_t x1,
//                        uint16_t x2,
//                        uint16_t y1,
//                        uint16_t y2,
//                        const uint16_t thickness,
//                        const Colour colour);
//     void DrawCharacter(
//         uint16_t x, uint16_t y, const char ch, const Font& font, const Colour fg, const Colour
//         bg);
//     void DrawString(uint16_t x,
//                     uint16_t y,
//                     const char* str,
//                     const uint16_t length,
//                     const Font& font,
//                     const Colour fg,
//                     const Colour bg);
//     void DefineScrollArea(const uint16_t tfa_idx, const uint16_t vsa_idx, const uint16_t
//     bfa_idx); void ScrollScreen(const uint16_t scroll_idx, bool up);

//     void UpdateTitle(const char* title, const uint32_t len);

//     // Do I need to do this?
//     void AppendText(const char* text, const uint32_t len);
//     void CommitText();
//     void CommitText(const char* text, const uint32_t len);

//     void AppendUserText(const char* text, const uint32_t len);
//     void AppendUserText(const char ch);

//     void BackspaceUserText();
//     void ClearUserText();

//     const char* UserText() const noexcept;
//     uint16_t UserTextLength() const noexcept;

// private:
//     inline void WaitForSPIComplete();
//     inline void SetPinToCommand();
//     inline void SetPinToData();
//     void WriteCommand(uint8_t cmd);
//     void WriteCommand(uint8_t cmd, uint8_t* data, const uint32_t sz);
//     void WriteDataWithSet(uint8_t data);
//     void WriteDataWithSet(uint8_t* data, const uint32_t data_size);
//     void WriteData(uint8_t data);
//     void WriteData(uint8_t* data, const uint32_t data_size);
//     void SetWriteablePixels(const int16_t x1, const int16_t x2, const int16_t y1, const int16_t
//     y2); DrawMemory& AllocateMemory(const uint16_t x1,
//                                const uint16_t x2,
//                                const uint16_t y1,
//                                const uint16_t y2,
//                                const Colour colour,
//                                MemoryCallback callback);
//     void NormalMode();

//     // Private functions
//     static bool DrawRectangleProcedure(DrawMemory& memory,
//                                        uint8_t matrix[HEIGHT][Half_Width_Pixel_Size],
//                                        const int16_t y1,
//                                        const int16_t y2);
//     void DrawRectangleProcedure(const int16_t x1,
//                                 const int16_t x2,
//                                 const int16_t y1,
//                                 const int16_t y2,
//                                 const uint16_t thickness,
//                                 const Colour colour);

//     static bool FillRectangleProcedure(DrawMemory& memory,
//                                        uint8_t matrix[HEIGHT][Half_Width_Pixel_Size],
//                                        const int16_t y1,
//                                        const int16_t y2);
//     static bool DrawCharacterProcedure(DrawMemory& memory,
//                                        uint8_t matrix[HEIGHT][Half_Width_Pixel_Size],
//                                        const int16_t y1,
//                                        const int16_t y2);
//     static bool DrawStringProcedure(DrawMemory& memory,
//                                     uint8_t matrix[HEIGHT][Half_Width_Pixel_Size],
//                                     const int16_t y1,
//                                     const int16_t y2);

//     inline void HandleBounds(uint16_t& x1, uint16_t& x2, uint16_t& y1, uint16_t& y2);

//     static inline uint8_t* GetCharAddr(uint8_t* font_data,
//                                        const uint8_t ch,
//                                        const uint16_t font_width,
//                                        const uint16_t font_height);
//     static inline void
//     PushMemoryParameter(DrawMemory& memory, const uint32_t val, const int16_t num_bytes);
//     // Non-destructive retrieval
//     template <typename T, typename std::enable_if<std::is_integral<T>::value, bool>::type = 0>
//     static inline T PullMemoryParameter(DrawMemory& memory)
//     {
//         const int16_t bytes = sizeof(T);
//         T output = 0;
//         uint8_t& idx = memory.read_idx;

//         for (int16_t i = 0; i < bytes && i < memory.write_idx; ++i)
//         {
//             output |= memory.parameters[idx] << (8 * i);
//             ++idx;
//         }

//         // Restart where we read from if the index is zero
//         if (idx >= memory.write_idx)
//         {
//             idx = 0;
//         }

//         return output;
//     }
//     static inline void FillMatrixAtIdx(uint8_t matrix[HEIGHT][Half_Width_Pixel_Size],
//                                        const uint16_t i,
//                                        const uint16_t j,
//                                        const uint8_t colour_high,
//                                        const uint8_t colour_low) __attribute__((always_inline));
//     static inline void
//     FillLineAtIdx(uint8_t* line, const uint16_t i, const uint16_t j, const uint16_t colour)
//         __attribute__((always_inline));
//     static inline YBound
//     GetYBounds(const uint16_t y1, const uint16_t y2, const uint16_t mem_y1, const uint16_t
//     mem_y2)
//         __attribute__((always_inline));

//     const Font& title_font = font11x16;
//     const Font& text_font = font5x8;

//     // Variables
//     SPI_HandleTypeDef* spi;

//     GPIO_TypeDef* cs_port;
//     const uint16_t cs_pin;
//     GPIO_TypeDef* dc_port;
//     const uint16_t dc_pin;
//     GPIO_TypeDef* rst_port;
//     const uint16_t rst_pin;
//     GPIO_TypeDef* bl_port;
//     const uint16_t bl_pin;

//     Orientation orientation;
//     uint16_t view_height;
//     uint16_t view_width;
//     uint16_t row_bytes;

//     DrawMemory memories[Num_Memories];
//     uint32_t memory_read_idx;
//     uint32_t memories_in_use;
//     uint32_t memory_write_idx;
//     uint16_t row;
//     uint16_t end_row;

//     uint16_t row_start_update;
//     uint16_t row_end_update;

//     bool updating;
//     bool restart_update;

//     // Each byte stores two pixels
//     // TODO 1d array so that I can dynamically change the orientation
//     uint8_t matrix[HEIGHT][WIDTH / 2];

//     // Double buff
//     uint8_t scan_window[Scan_Window_Dbl_Sz];

//     // Window: 0-20px
//     // Max 21 characters
//     char title_buffer[Title_Length];

//     // Window: 20 - 308px
//     uint32_t text_idx;
//     uint32_t texts_in_use;
//     uint32_t scroll_offset;
//     char text_buffer[Max_Texts][Max_Characters];
//     char text_lens[Max_Texts];

//     // Window: 308-320px
//     char usr_buffer[Max_Characters];
//     char usr_buffer_idx;
// };

// // 44226 bytes approximately.