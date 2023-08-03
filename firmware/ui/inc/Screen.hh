#pragma once

#include "stm32.h"
#include "PortPin.hh"
#include "Font.hh"
#include "String.hh"
#include "RingMatrix.hh"

#define SF_RST 0x01 // Software reset
#define PWRC_A 0xCB // Power control A
#define PWRC_B 0xCF // Power control B
#define TIMC_A 0xE8 // Timer control A
#define TIMC_B 0xEA // Timer control B
#define PWR_ON 0xED // Power on sequence control
#define PMP_RA 0xF7 // Pump ratio command
#define PC_VRH 0xC0 // Power control VRH[5:0]
#define PC_SAP 0xC1 // Power control SAP[2:0];BT[3:0]
#define VCM_C1 0xC5 // VCM Control 1
#define VCM_C2 0xC7 // VCM Control 2
#define MEM_CR 0x36 // Memory access control
#define PIX_FM 0x3A // Pixel format
#define FR_CTL 0xB1 // Frame ratio control. RGB Color
#define DIS_CT 0xB6 // Display function control
#define GAMM_3 0xF2 // 3 Gamma function display
#define GAMM_C 0x26 // Gamma curve selected
#define GAM_PC 0xE0 // Positive gamma correction
#define GAM_NC 0xE1 // Negative gamma correction
#define END_SL 0x11 // Exit sleep
#define DIS_ON 0x29 // Display on
#define MAD_CT 0x36 // Rotation control

#define CA_SET 0x2A // Column address set
#define RA_SET 0x2B // Row address set
#define WR_RAM 0x2C // Write to RAM

#define MAD_CTL_MY  0x80
#define MAD_CTL_MX  0x40
#define MAD_CTL_MV  0x20
#define MAD_CTL_ML  0x10
#define MAD_CTL_RGB 0x00
#define MAD_CTL_BGR 0x08
#define MAD_CTL_MH  0x04

// Some basic colours
#define	C_BLACK         0x0000
#define	C_BLUE          0x001F
#define	C_RED           0xF800
#define C_LIGHT_GREEN   0x3626
#define	C_GREEN         0x07E0
#define C_CYAN          0x07FF
#define C_MAGENTA       0xF81F
#define C_YELLOW        0xFFE0
#define C_WHITE         0xFFFF
#define C_GREY          0xCE59

// Default orientation
#define WIDTH                240
#define HEIGHT               320
#define PORTRAIT_DATA        (MAD_CTL_MY | MAD_CTL_BGR)
#define LEFT_LANDSCAPE_DATA  (MAD_CTL_MV | MAD_CTL_BGR)
#define RIGHT_LANDSCAPE_DATA (MAD_CTL_MX | MAD_CTL_MY | MAD_CTL_MV | MAD_CTL_BGR)

class Screen
{
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

    // TODO pg 129 add vertical scroll
    //https://www.waveshare.com/w/upload/e/e3/ILI9341_DS.pdf

    void Begin();
    inline void Select();
    inline void Deselect();
    void Reset();
    void Loop();

    // TODO move these to private?
    void WriteCommand(uint8_t command);
    void WriteData(uint8_t* data, uint32_t data_size);
    void WriteData(uint8_t data);
    void WriteDataDMA(uint8_t* data, const uint32_t data_size);
    void WriteDataDMAFree(uint8_t* data, const uint32_t data_size);
    void SetWritablePixels(uint16_t x_start, uint16_t y_start, uint16_t x_end,
                           uint16_t y_end);
    void SetOrientation(Orientation _orientation);
    void EnableBackLight();
    void DisableBackLight();
    void Sleep();
    void Wake();


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
                                const String &str, const Font &font,
                                const uint16_t fg, const uint16_t bg,
                                const uint16_t delay);

    void DrawText(const uint16_t x, const uint16_t y, const String &str,
                    const Font &font, const uint16_t fg, const uint16_t bg,
                    const bool wordwrap = false,
                    uint32_t max_chunk_size=Max_Chunk_Size);

    void DrawTextbox(uint16_t x_pos,
                     uint16_t y_pos,
                     const uint16_t x_window_start,
                     const uint16_t y_window_start,
                     const uint16_t x_window_end,
                     const uint16_t y_window_end,
                     const String &str,
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
                    const uint16_t colour); // TODO

    void FillPolygon(const size_t count,
                     const int16_t points [][2],
                     const uint16_t colour);

    void FillRectangle(const uint16_t x_start,
                       const uint16_t y_start,
                       uint16_t x_end,
                       uint16_t y_end,
                       const uint16_t colour,
                       uint32_t max_chunk_size=Max_Chunk_Size);

    void FillRectangleFree(const uint16_t x_start,
                       const uint16_t y_start,
                       uint16_t x_end,
                       uint16_t y_end,
                       const uint16_t colour);

    void FillScreen(const uint16_t colour);

    void FillTriangle(const uint16_t x1, const uint16_t y1,
                      const uint16_t x2, const uint16_t y2,
                      const uint16_t x3, const uint16_t y3,
                      const uint16_t colour);

    void ReleaseSPI();
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

    uint16_t ViewWidth() const;
    uint16_t ViewHeight() const;
    uint16_t GetStringWidth(const uint16_t str_len, const Font& font) const;
    uint16_t GetStringCenter(const uint16_t str_len, const Font& font) const;
    uint16_t GetStringCenterMargin(const uint16_t str_len, const Font& font) const;
    uint16_t GetStringLeftDistanceFromRightEdge(const uint16_t str_len, const Font& font) const;

    inline void DrawNext();
private:
    void PushDrawingFunction(void* func);
    void UpdateDrawingFunction(void* func);
    void PopDrawingFunction();

    static void FillRectangleProcedure(Screen* screen);


    static constexpr uint32_t Max_Chunk_Size = 16384U;
    static constexpr uint32_t Chunk_Buffer_Size = 1024UL;

    void Clip(const uint16_t x_start, const uint16_t y_start, uint16_t &x_end,
              uint16_t &y_end);

    inline void WaitUntilSPIFree();

    // Variables
    SPI_HandleTypeDef *spi_handle = nullptr;
    port_pin cs;
    port_pin dc;
    port_pin rst;
    port_pin bl;
    Orientation orientation;
    uint16_t view_height;
    uint16_t view_width;
    uint8_t chunk_buffer[Chunk_Buffer_Size * 2]; // TODO use this more
    volatile bool spi_busy;
    volatile bool draw_async;
    volatile bool draw_async_stop;
    bool buffer_overwritten_by_sync;
    uint32_t drawing_func_read;
    uint32_t drawing_func_write;
    bool async_draw_ready;
    RingMatrix draw_matrix;
    void** Drawing_Func_Ring;
};