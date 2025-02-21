#include "keyboard.h"

static uint8_t KB_ReadFlags(Keyboard* kb, uint32_t flags)
{
    return kb->flags & flags;
}

static void KB_RaiseFlags(Keyboard* kb, uint32_t flags)
{
    kb->flags |= flags;
}

static void KB_LowerFlags(Keyboard* kb, uint32_t flags)
{
    kb->flags &= ~flags;
}

static int8_t KB_FlagPress(Keyboard* kb, const int16_t col, const int16_t row)
{
    // TODO locks

    if (SYM_Col == col && SYM_Row == row)
    {
        KB_RaiseFlags(kb, Sym_Flag);
        return 1;
    }
    if (ALT_Col == col && ALT_Row == row)
    {
        KB_RaiseFlags(kb, Alt_Flag);
        return 1;
    }

    if (LSH_Col == col && LSH_Row == row)
    {
        KB_RaiseFlags(kb, Left_Shift_Flag);
        return 1;
    }
    if (RSH_Col == col && RSH_Row == row)
    {
        KB_RaiseFlags(kb, Right_Shift_Flag);
        return 1;
    }

    // Mic is unique. Sym flag should be read before the mic flag
    if (!KB_ReadFlags(kb, Sym_Flag) && MIC_Col == col && MIC_Row == row)
    {
        KB_RaiseFlags(kb, Mic_Flag);
        return 1;
    }

    return 0;
}

static void KB_FlagRelease(Keyboard* kb, const int16_t col, const int16_t row)
{
    if (SYM_Col == col && SYM_Row == row)
    {
        KB_LowerFlags(kb, Sym_Flag);
        return;
    }
    if (ALT_Col == col && ALT_Row == row)
    {
        KB_LowerFlags(kb, Alt_Flag);
        return;
    }

    if (LSH_Col == col && LSH_Row == row)
    {
        KB_LowerFlags(kb, Left_Shift_Flag);
        return;
    }
    if (RSH_Col == col && RSH_Row == row)
    {
        KB_LowerFlags(kb, Right_Shift_Flag);
        return;
    }
    if (MIC_Col == col && MIC_Row == row)
    {
        KB_LowerFlags(kb, Mic_Flag);
        return;
    }
}

static void KB_KeyPress(Keyboard* kb, const int16_t col, const int16_t row, const uint8_t latch_bit)
{
    // Reset the bit
    kb->latch[col] |= latch_bit;

    if (KB_FlagPress(kb, col, row))
    {
        return;
    }

    if (KB_ReadFlags(kb, Sym_Flag | Sym_Lock_Flag))
    {
        // Symbol
        StaticRingBuffer_Commit(kb->ch_ring, Symb_Char_Map[col][row]);
    }
    else if (KB_ReadFlags(kb, Left_Shift_Flag | Right_Shift_Flag | Caps_Lock_Flag))
    {
        // Uppercase
        StaticRingBuffer_Commit(kb->ch_ring, Base_Char_Map[col][row]);
    }
    else
    {
        // Lowercase
        StaticRingBuffer_Commit(kb->ch_ring, Base_Char_Map[col][row] + CHR_OFFSET);
    }
}

static void KB_KeyRelease(Keyboard* kb, const int16_t col, const int16_t row, const uint8_t latch_bit)
{
    kb->latch[col] &= ~latch_bit;

    KB_FlagRelease(kb, col, row);
}



void KB_Init(Keyboard* kb,
    GPIO_TypeDef* col_ports[Q10_COLS],
    uint16_t col_pins[Q10_COLS],
    GPIO_TypeDef* row_ports[Q10_ROWS],
    uint16_t row_pins[Q10_ROWS],
    StaticRingBuffer* ch_ring,
    uint32_t debounce_duration,
    uint32_t repeat_duration
)
{
    for (int i = 0; i < Q10_COLS; ++i)
    {
        kb->col_ports[i] = col_ports[i];
        kb->col_pins[i] = col_pins[i];
        kb->latch[i] = 0;

        for (int j = 0; j < Q10_ROWS; ++j)
        {
            kb->debounce[i][j] = 0;
        }
    }

    for (int i = 0; i < Q10_ROWS; ++i)
    {
        kb->row_ports[i] = row_ports[i];
        kb->row_pins[i] = row_pins[i];
    }

    kb->ch_ring = ch_ring;
    kb->debounce_duration = debounce_duration;
    kb->repeat_duration = repeat_duration;
    kb->flags = 0;
}


void KB_Scan(Keyboard* kb, const uint32_t ticks)
{
    uint8_t press = 0;
    uint8_t latch = 0;
    uint8_t row_bit = 0;
    uint8_t row_mask = 0;

    for (int16_t col = 0; col < Q10_COLS; ++col)
    {
        // Raise the col high and poll the rows
        HAL_GPIO_WritePin(kb->col_ports[col], kb->col_pins[col], GPIO_PIN_SET);


        for (int16_t row = 0; row < Q10_ROWS; ++row)
        {
            press = HAL_GPIO_ReadPin(kb->row_ports[row], kb->row_pins[row]);
            row_bit = 1 << row;
            latch = (kb->latch[col] >> row) & 0x1;


            if (press == GPIO_PIN_SET
                && press != latch
                && ticks >= kb->debounce[col][row])
            {
                // Press
                kb->debounce[col][row] = ticks + kb->debounce_duration;
                KB_KeyPress(kb, col, row, row_bit);
            }
            else if (press == GPIO_PIN_RESET && press != latch)
            {
                // Release
                kb->debounce[col][row] = ticks + kb->debounce_duration;
                KB_KeyRelease(kb, col, row, row_bit);
            }
            else if (press == GPIO_PIN_SET
                && ticks >= kb->debounce[col][row]
                && latch)
            {
                // Repeat
                kb->debounce[col][row] = ticks + kb->repeat_duration;
                KB_KeyPress(kb, col, row, row_bit);
            }
        }

        HAL_GPIO_WritePin(kb->col_ports[col], kb->col_pins[col], GPIO_PIN_RESET);
    }
}

uint16_t KB_NumAvailable(Keyboard* kb)
{
    return StaticRingBuffer_Available(kb->ch_ring);
}

uint8_t KB_Read(Keyboard* kb)
{
    return StaticRingBuffer_Read(kb->ch_ring);
}