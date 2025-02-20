#include "keyboard.h"


static int8_t KB_FlagPress(Keyboard* kb, const int16_t col, const int16_t row, const uint8_t press)
{
    // Check all of the special flags
    uint32_t mask = press;

    // TODO locks

    if (SYM_Col == col && SYM_Row == row)
    {
        KB_RaiseFlag(kb, Sym_Flag);
        return 1;
    }
    if (ALT_Col == col && ALT_Row == row)
    {
        KB_RaiseFlag(kb, Alt_Flag);
        return 1;
    }

    if (LSH_Col == col && LSH_Row == row)
    {
        KB_RaiseFlag(kb, Left_Shift_Flag);
        return 1;
    }
    if (RSH_Col == col && RSH_Row == row)
    {
        KB_RaiseFlag(kb, Right_Shift_Flag);
        return 1;
    }

    // Mic is unique. Sym flag should be read before the mic flag
    if (!KB_ReadFlag(kb, Sym_Flag) && MIC_Col == col && MIC_Row == row)
    {
        KB_RaiseFlag(kb, Mic_Flag);
        return 1;
    }

    return 0;
}

static void KB_FlagRelease(Keyboard* kb, const int16_t col, const int16_t row)
{
    uint8_t latch_bit = 1 << row;
    uint8_t latch_mask = ~latch_bit;

    // Reset the bit
    kb->latch[col] &= latch_bit;
}

static void KB_KeyPress(Keyboard* kb, const int16_t col, const int16_t row)
{
    uint8_t latch_bit = 1 << row;
    uint8_t latch_mask = ~latch_bit;

    // Reset the bit
    kb->latch[col] &= latch_bit;

    // If the latch was set then flip it on
    kb->latch[col] |= latch_bit;

    if (KB_HandleFlags(kb, col, row))
    {
        return;
    }

    if (KB_ReadFlag(kb, Sym_Flag | Sym_Lock_Flag))
    {
        // Symbol
    }
    else if (KB_ReadFlag(kb, Left_Shift_Flag | Right_Shift_Flag | Caps_Lock_Flag))
    {
        // Uppercase
    }
    else
    {
        // Lowercase
    }
}

static void KB_KeyRelease(Keyboard* kb, const int16_t col, const int16_t row)
{

}

void KB_Scan(Keyboard* kb, const uint32_t ticks)
{
    uint8_t press = 0;
    uint8_t latch;
    uint8_t row_mask = 0;
    int8_t char_press = 0;

    for (int16_t col = 0; col < Q10_COLS; ++col)
    {
        // Raise the col high and poll the rows
        HAL_GPIO_WritePin(kb->col_ports[col], kb->col_pins[col], GPIO_PIN_SET);


        for (int16_t row = 0; row < Q10_ROWS; ++row)
        {
            press = HAL_GPIO_ReadPin(kb->row_ports[row], kb->row_pins[row]);
            row_mask = ~(1 << row);
            latch = (kb->latch[col] >> row) & row_mask;


            if (press == GPIO_PIN_SET
                && press != latch
                && HAL_GetTick() >= kb->debounce[col][row])
            {
                // Press
                KB_KeyPress(kb, col, row);
            }
            else if (press == GPIO_PIN_RESET && press != latch)
            {
                // Release
                KB_KeyRelease(kb, col, row);
            }
            else if (press == GPIO_PIN_SET
                && HAL_GetTick() >= kb->debounce[col][row]
                && latch)
            {
                // Repeat
                KB_KeyPress(kb, col, row);
            }
        }

        HAL_GPIO_WritePin(kb->col_ports[col], kb->col_pins[col], GPIO_PIN_RESET);
    }
}