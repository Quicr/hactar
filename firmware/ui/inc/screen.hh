#pragma once

#include "stm32.h"
#include "port_pin.hh"
#include "font.hh"
#include "ring_memory_pool.hh"
#include "swap_buffer.hh"

#include <string>

#define SF_RST 0x01U // Software reset
#define PWRC_A 0xCBU // Power control A
#define PWRC_B 0xCFU // Power control B
#define TIMC_A 0xE8U // Timer control A
#define TIMC_B 0xEAU // Timer control B
#define PWR_ON 0xEDU // Power on sequence control
#define PMP_RA 0xF7U // Pump ratio command
#define PC_VRH 0xC0U // Power control VRH[5:0]
#define PC_SAP 0xC1U // Power control SAP[2:0];BT[3:0]
#define VCM_C1 0xC5U // VCM Control 1
#define VCM_C2 0xC7U // VCM Control 2
#define MEM_CR 0x36U // Memory access control
#define PIX_FM 0x3AU // Pixel format
#define FR_CTL 0xB1U // Frame ratio control. RGB Color
#define DIS_CT 0xB6U // Display function control
#define GAMM_3 0xF2U // 3 Gamma function display
#define GAMM_C 0x26U // Gamma curve selected
#define GAM_PC 0xE0U // Positive gamma correction
#define GAM_NC 0xE1U // Negative gamma correction
#define END_SL 0x11U // Exit sleep
#define DIS_ON 0x29U // Display on
#define MAD_CT 0x36U // Rotation control

#define CA_SET 0x2AU // Column address set
#define RA_SET 0x2BU // Row address set
#define WR_RAM 0x2CU // Write to RAM

#define MAD_CTL_MY  0x80U
#define MAD_CTL_MX  0x40U
#define MAD_CTL_MV  0x20U
#define MAD_CTL_ML  0x10U
#define MAD_CTL_RGB 0x00U
#define MAD_CTL_BGR 0x08U
#define MAD_CTL_MH  0x04U

// Some basic colours
#define	C_BLACK         0x0000U
#define	C_BLUE          0x001FU
#define	C_RED           0xF800U
#define C_LIGHT_GREEN   0x3626U
#define	C_GREEN         0x07E0U
#define C_CYAN          0x07FFU
#define C_MAGENTA       0xF81FU
#define C_YELLOW        0xFFE0U
#define C_WHITE         0xFFFFU
#define C_GREY          0xCE59U

// Default orientation
#define WIDTH                240U
#define HEIGHT               320U
#define PORTRAIT_DATA        (MAD_CTL_MY | MAD_CTL_BGR)
#define LEFT_LANDSCAPE_DATA  (MAD_CTL_MV | MAD_CTL_BGR)
#define RIGHT_LANDSCAPE_DATA (MAD_CTL_MX | MAD_CTL_MY | MAD_CTL_MV | MAD_CTL_BGR)

class Screen
{
private:
    // TODO remove
    static constexpr uint32_t Max_Chunk_Size = 16384U;
    static constexpr uint32_t Chunk_Buffer_Size = 2048UL;

    static constexpr uint32_t Num_Memories = 20;
    static constexpr uint32_t Memory_Size = 64;

    enum class MemoryStatus
    {
        Unused = 0,
        In_Progress,
        Complete
    };

    struct ScreenMemory
    {
        void (*callback)(Screen& screen, ScreenMemory& memory) = nullptr;
        void (*post_callback)(Screen& screen, ScreenMemory& memory) = nullptr;
        MemoryStatus status = MemoryStatus::Unused;
        uint8_t parameters[Memory_Size]{0}; // A bunch of data params to run the next command
    };

public:
    enum Orientation {
        portrait,
        left_landscape,
        right_landscape
    };

    enum ArrowDirection {
        Left,
        Up,
        Right,
        Down
    };

    // I will need each of the pins to be input
    Screen(SPI_HandleTypeDef &spi,
           port_pin cs,
           port_pin dc,
           port_pin rst,
           port_pin bl,
           Orientation _orientation);
    ~Screen();

    // General interfacing functions
    void Begin();
    void Update(uint32_t current_tick);
    void Draw();

    // Public command functions
    void DisableBackLight();
    void EnableBackLight();
    void Reset();
    void SetOrientation(Orientation _orientation);
    void Sleep();
    void Wake();

    // Sync functions
    void DrawArrow(const uint16_t tip_x, const uint16_t tip_y,
                   const uint16_t length, const uint16_t width,
                   const ArrowDirection direction, const uint16_t colour);

    void DrawCircle(const uint16_t x, const uint16_t y, const uint16_t r,
                    const uint16_t colour); // TODO

    void DrawHorizontalLine(const uint16_t x1, const uint16_t x2,
                            const uint16_t y, const uint16_t thickness,
                            const uint16_t colour);

    void DrawLine(uint16_t x1, uint16_t y1,
                  uint16_t x2, uint16_t y2,
                  const uint16_t colour);

    void DrawPixel(const uint16_t x, const uint16_t y, const uint16_t colour);

    void DrawPolygon(const size_t count, const uint16_t points[][2], const uint16_t colour);

    void DrawRectangle(const uint16_t x_start, const uint16_t y_start,
                       const uint16_t x_end, const uint16_t y_end,
                       const uint16_t thickness, const uint16_t colour);

    void DrawBlockAnimateString(const uint16_t x, const uint16_t y,
                                const std::string &str, const Font &font,
                                const uint16_t fg, const uint16_t bg,
                                const uint16_t delay);

    void DrawText(const uint16_t x, const uint16_t y, const std::string &str,
                    const Font &font, const uint16_t fg, const uint16_t bg,
                    const bool wordwrap = false,
                    uint32_t max_chunk_size=Max_Chunk_Size);

    void DrawTextbox(uint16_t x_pos,
                     uint16_t y_pos,
                     const uint16_t x_window_start,
                     const uint16_t y_window_start,
                     const uint16_t x_window_end,
                     const uint16_t y_window_end,
                     const std::string &str,
                     const Font &font,
                     const uint16_t fg,
                     const uint16_t bg);

    void DrawTriangle(const uint16_t x1, const uint16_t y1,
                      const uint16_t x2, const uint16_t y2,
                      const uint16_t x3, const uint16_t y3,
                      const uint16_t colour);

    void FillArrow(const uint16_t tip_x, const uint16_t tip_y,
                   const uint16_t length, const uint16_t width,
                   const ArrowDirection direction, const uint16_t colour);

    void FillCircle(const uint16_t x, const uint16_t y, const uint16_t r,
                    const uint16_t fg, const uint16_t bg);

    void FillPolygon(const size_t count,
                     const int16_t points [][2],
                     const uint16_t colour);

    void FillRectangle(const uint16_t x_start,
                       const uint16_t y_start,
                       uint16_t x_end,
                       uint16_t y_end,
                       const uint16_t colour,
                       uint32_t max_chunk_size=Max_Chunk_Size);

    void FillScreen(const uint16_t colour, bool async=true);

    void FillTriangle(const uint16_t x1, const uint16_t y1,
                      const uint16_t x2, const uint16_t y2,
                      const uint16_t x3, const uint16_t y3,
                      const uint16_t colour);

    void DrawCharacter(uint16_t x_start,
                       uint16_t y_start,
                       const uint16_t x_window_begin,
                       const uint16_t y_window_begin,
                       const uint16_t x_window_end,
                       const uint16_t y_window_end,
                       const char ch,
                       const Font &font,
                       const uint16_t fg,
                       const uint16_t bg);

    // Async functions
    void DrawArrowAsync(const uint16_t tip_x, const uint16_t tip_y,
                   const uint16_t length, const uint16_t width,
                   const uint16_t thickness, const ArrowDirection direction,
                   const uint16_t colour);

    void DrawCharacterAsync(uint16_t x, uint16_t y, const char ch,
        const Font& font, const uint16_t fg, const uint16_t bg);

    void DrawCircleAsync(const uint16_t x, const uint16_t y, const uint16_t r,
                    const uint16_t colour); // TODO

    void DrawLineAsync(const uint16_t x1, const uint16_t x2,
                       const uint16_t y1, const uint16_t y2,
                       const  uint16_t thickness,
                       const uint16_t colour);
    void DrawPixelAsync(const uint16_t x, const uint16_t y,
                        const uint16_t colour);

    void DrawPolygonAsync(const size_t count, const uint16_t points[][2],
                          const uint16_t thickness, const uint16_t colour);

    void DrawRectangleAsync(const uint16_t x1, const uint16_t x2,
        const uint16_t y1, const uint16_t y2, const uint16_t thickness,
        const uint16_t colour);

    void DrawString(const uint16_t x, const uint16_t y, const char* str,
        const uint16_t len, const Font& font, const uint16_t fg,
        const uint16_t bg);

    void DrawStringBox(const uint16_t x1, const uint16_t x2,
        const uint16_t y1, const uint16_t y2,
        const char* str, const uint16_t len,
        const Font& font, const uint16_t fg, const uint16_t bg);


    void FillRectangleAsync(uint16_t x1,
                            uint16_t x2,
                            uint16_t y1,
                            uint16_t y2,
                            const uint16_t colour);



    // Helper Functions
    void ReleaseSPI();
    uint16_t ViewWidth() const;
    uint16_t ViewHeight() const;
    uint16_t GetStringWidth(const uint16_t str_len, const Font& font) const;
    uint16_t GetStringCenter(const uint16_t str_len, const Font& font) const;
    uint16_t GetStringCenterMargin(const uint16_t str_len, const Font& font) const;
    uint16_t GetStringLeftDistanceFromRightEdge(const uint16_t str_len, const Font& font) const;
    uint16_t Convert32ColorTo16(const uint32_t colour);


private:
    // Private gpio functions
    inline void Select();
    inline void Deselect();

    // Private sync command functions
    void SetWritablePixels(uint16_t x_start, uint16_t y_start, uint16_t x_end,
                           uint16_t y_end);
    void WriteCommand(uint8_t command);
    void WriteData(uint8_t* data, uint32_t data_size);
    void WriteData(uint8_t data);
    void WriteDataSyncDMA(uint8_t* data, const uint32_t data_size);

    // Private async command functions

    // TODO RENAME SetWritablePixelsAsync
    ScreenMemory* SetWritablePixelsAsync(uint16_t x1, uint16_t x2, uint16_t y1, uint16_t y2);
    static void SetColumnsCommandAsync(Screen& screen, ScreenMemory& memory);
    static void SetColumnsDataAsync(Screen& screen, ScreenMemory& memory);
    static void SetRowsCommandAsync(Screen& screen, ScreenMemory& memory);
    static void SetRowsDataAsync(Screen& screen, ScreenMemory& memory);
    static void WriteToRamCommandAsync(Screen& screen, ScreenMemory& memory);
    bool WriteCommandAsync(uint8_t cmd);
    bool WriteDataAsync(uint8_t* data, uint32_t data_size);
    bool WriteAsync(SwapBuffer::swap_buffer_t* buff);

    ScreenMemory* RetrieveFreeMemory();
    void HandleReadyMemory();
    void HandleVideoBuffer();

    // Async procedure functions
    static void DrawCharacterProcedure(Screen& screen, ScreenMemory& memory);
    static void DrawLineAsyncProcedure(Screen& screen, ScreenMemory& memory);
    static void DrawPixelAsyncProcedure(Screen& screen, ScreenMemory& memory);
    static void FillRectangleAsyncProcedure(Screen& screen, ScreenMemory& memory);


    // Private helpers
    void Clip(const uint16_t x_start, const uint16_t y_start, uint16_t &x_end,
              uint16_t &y_end);
    inline void WaitUntilSPIFree();
    void WaitForFreeMemory(const uint16_t minimum = 1);


    // Variables
    SPI_HandleTypeDef *spi_handle = nullptr;
    port_pin cs;
    port_pin dc;
    port_pin rst;
    port_pin bl;
    Orientation orientation;
    uint16_t view_height;
    uint16_t view_width;
    volatile bool spi_busy;
    volatile bool spi_async;

    SwapBuffer video_buff;
    SwapBuffer::swap_buffer_t* video_front_buff;

    ScreenMemory memories[Num_Memories];
    ScreenMemory* live_memory;
    // TODO put into constructor
    uint32_t memories_write_idx = 0;
    uint32_t memories_read_idx = 0;
    uint32_t free_memories = Num_Memories;

    uint8_t str[64];
    uint16_t len;
};