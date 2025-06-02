#pragma once

#include "ring_buffer.hh"
#include "stm32.h"
class Keyboard
{
public:
    static constexpr uint8_t Q10_Cols = 5;
    static constexpr uint8_t Q10_Rows = 7;

    static constexpr uint8_t Sym_Col = 0;
    static constexpr uint8_t Sym_Row = 2;

    static constexpr uint8_t Alt_Col = 0;
    static constexpr uint8_t Alt_Row = 4;

    static constexpr uint8_t Spc_Col = 0;
    static constexpr uint8_t Spc_Row = 5;

    static constexpr uint8_t Mic_Col = 0;
    static constexpr uint8_t Mic_Row = 6;

    static constexpr uint8_t Lsh_Col = 1;
    static constexpr uint8_t Lsh_Row = 6;

    static constexpr uint8_t Rsh_Col = 2;
    static constexpr uint8_t Rsh_Row = 3;

    static constexpr uint8_t Ent_Col = 3;
    static constexpr uint8_t Ent_Row = 3;

    static constexpr uint8_t Bak_Col = 4;
    static constexpr uint8_t Bak_Row = 3;

    static constexpr uint8_t Spk_Col = 0;
    static constexpr uint8_t Spk_Row = 4;

    // NOTE- This is corresponding to our fontset
    static constexpr uint8_t CHR_OFFSET = 32;
    // TODO these may need an update
    static constexpr uint8_t Alt = 18;
    static constexpr uint8_t Sym = 17;               // CTRL -> remapped to Symbol.
    static constexpr uint8_t Spc = 32 - CHR_OFFSET;  // This is just 0, but its for readability
    static constexpr uint8_t Dlr = '$' - CHR_OFFSET; // For readability
    static constexpr uint8_t Mic = 20;               // CAPSLOCK -> remapped to mic
    static constexpr uint8_t Shf = 16;
    static constexpr uint8_t Ent = 13;
    static constexpr uint8_t Bak = 8;
    static constexpr uint8_t Spk = 7; // Remapped to Speaker
    static constexpr uint8_t Nil = '\0';

    Keyboard(GPIO_TypeDef* col_ports[Q10_Cols],
             uint16_t col_pins[Q10_Cols],
             GPIO_TypeDef* row_ports[Q10_Rows],
             uint16_t row_pins[Q10_Rows],
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

    const uint8_t Base_Char_Map[Q10_Cols][Q10_Rows] = {
        {'Q', 'W', Sym, 'A', Alt, Spc, Mic}, // col1
        {'E', 'S', 'D', 'P', 'X', 'Z', Shf}, // col2
        {'R', 'G', 'T', Shf, 'V', 'C', 'F'}, // col3
        {'U', 'H', 'Y', Ent, 'B', 'N', 'J'}, // col4
        {'O', 'L', 'I', Bak, Dlr, 'M', 'K'}, // col5
    };

    const uint8_t Symb_Char_Map[Q10_Cols][Q10_Rows] = {
        {'#', '1', Sym, '*', Alt, Spc, '0'},  // col1
        {'2', '4', '5', '@', '8', '7', Shf},  // col2
        {'3', '/', '(', Shf, '?', '9', '6'},  // col3
        {'_', ':', ')', Ent, '!', ',', ';'},  // col4
        {'+', '"', '-', Bak, Spk, '.', '\''}, // col5
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
    uint8_t latches[Q10_Cols];
    uint32_t debounce[Q10_Cols][Q10_Rows];
    uint32_t flags;
};