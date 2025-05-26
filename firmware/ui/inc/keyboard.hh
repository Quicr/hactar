#pragma once

#include "ring_buffer.hh"
#include "stm32.h"

// TODO constexpr
#define Q10_COLS 5
#define Q10_ROWS 7

#define SYM_Col 0
#define SYM_Row 2

#define ALT_Col 0
#define ALT_Row 4

#define SPC_Col 0
#define SPC_Row 5

#define MIC_Col 0
#define MIC_Row 6

#define LSH_Col 1
#define LSH_Row 6

#define RSH_Col 2
#define RSH_Row 3

#define ENT_Col 3
#define ENT_Row 3

#define BAK_Col 4
#define BAK_Row 3

#define SPK_Col 0
#define SPK_Row 4

// NOTE- This is corresponding to our fontset
#define CHR_OFFSET 32
// TODO these may need an update
#define ALT 18
#define SYM 17               // CTRL -> remapped to Symbol.
#define SPC 32 - CHR_OFFSET  // This is just 0, but its for readability
#define DLR '$' - CHR_OFFSET // For readability
#define MIC 20               // CAPSLOCK -> remapped to mic
#define SHF 16
#define ENT 13
#define BAK 8
#define SPK 7 // Remapped to Speaker
#define NIL '\0'

class Keyboard
{
public:
    Keyboard(GPIO_TypeDef* col_ports[Q10_COLS],
             uint16_t col_pins[Q10_COLS],
             GPIO_TypeDef* row_ports[Q10_ROWS],
             uint16_t row_pins[Q10_ROWS],
             RingBuffer<uint8_t>& ch_ring,
             uint32_t debounce_duration,
             uint32_t repeat_duration);

    void Scan(const uint32_t ticks);
    uint16_t NumAvailable();
    uint8_t Read();

private:
    typedef enum
    {
        Sym_Flag = 1 << 0,
        Alt_Flag = 1 << 1,
        Left_Shift_Flag = 1 << 2,
        Right_Shift_Flag = 1 << 3,
        Caps_Lock_Flag = 1 << 4,
        Sym_Lock_Flag = 1 << 5,
        Mic_Flag = 1 << 6,
        Enter_Flag = 1 << 7,
        Back_Flag = 1 << 8
    } Flags;

    const uint8_t Base_Char_Map[Q10_COLS][Q10_ROWS] = {
        {'Q', 'W', SYM, 'A', ALT, SPC, MIC}, // col1
        {'E', 'S', 'D', 'P', 'X', 'Z', SHF}, // col2
        {'R', 'G', 'T', SHF, 'V', 'C', 'F'}, // col3
        {'U', 'H', 'Y', ENT, 'B', 'N', 'J'}, // col4
        {'O', 'L', 'I', BAK, DLR, 'M', 'K'}, // col5
    };

    const uint8_t Symb_Char_Map[Q10_COLS][Q10_ROWS] = {
        {'#', '1', SYM, '*', ALT, SPC, '0'},  // col1
        {'2', '4', '5', '@', '8', '7', SHF},  // col2
        {'3', '/', '(', SHF, '?', '9', '6'},  // col3
        {'_', ':', ')', ENT, '!', ',', ';'},  // col4
        {'+', '"', '-', BAK, SPK, '.', '\''}, // col5
    };

    uint8_t ReadFlags(uint32_t flags);
    void RaiseFlags(uint32_t flags);
    void LowerFlags(uint32_t flags);
    int8_t FlagPress(const int16_t col, const int16_t row);

    void FlagRelease(const int16_t col, const int16_t row);
    void KeyPress(const int16_t col, const int16_t row, const uint8_t latch_bit);
    void KeyRelease(const int16_t col, const int16_t row, const uint8_t latch_bit);

    GPIO_TypeDef** col_ports;
    uint16_t* col_pins;
    GPIO_TypeDef** row_ports;
    uint16_t* row_pins;
    RingBuffer<uint8_t>& ch_ring;
    uint32_t debounce_duration;
    uint32_t repeat_duration;
    uint8_t latches[Q10_COLS];
    uint32_t debounce[Q10_COLS][Q10_ROWS];
    uint32_t flags;
};