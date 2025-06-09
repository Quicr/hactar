#include "keyboard.hh"

Keyboard::Keyboard(GPIO_TypeDef* col_ports[Q10_Cols],
                   uint16_t col_pins[Q10_Cols],
                   GPIO_TypeDef* row_ports[Q10_Rows],
                   uint16_t row_pins[Q10_Rows],
                   RingBuffer<uint8_t>& ch_ring,
                   uint32_t debounce_duration,
                   uint32_t repeat_duration) :
    col_ports(col_ports),
    col_pins(col_pins),
    row_ports(row_ports),
    row_pins(row_pins),
    ch_ring(ch_ring),
    debounce_duration(debounce_duration),
    repeat_duration(repeat_duration),
    latches{0},
    debounce{0},
    flags(0)
{
}

void Keyboard::Scan(const uint32_t ticks)
{
    uint8_t press = 0;
    uint8_t latch = 0;
    uint8_t row_bit = 0;

    for (size_t col = 0; col < Q10_Cols; ++col)
    {
        // Raise the col high and poll the rows
        HAL_GPIO_WritePin(col_ports[col], col_pins[col], GPIO_PIN_SET);

        for (size_t row = 0; row < Q10_Rows; ++row)
        {
            press = HAL_GPIO_ReadPin(row_ports[row], row_pins[row]);
            row_bit = 1 << row;
            latch = (latches[col] >> row) & 0x1;

            if (press == GPIO_PIN_SET && press != latch && ticks >= debounce[col][row])
            {
                // Press
                debounce[col][row] = ticks + debounce_duration;
                KeyPress(col, row, row_bit);
            }
            else if (press == GPIO_PIN_RESET && press != latch)
            {
                // Release
                debounce[col][row] = ticks + debounce_duration;
                KeyRelease(col, row, row_bit);
            }
            else if (press == GPIO_PIN_SET && ticks >= debounce[col][row] && latch)
            {
                // Repeat
                debounce[col][row] = ticks + repeat_duration;
                KeyPress(col, row, row_bit);
            }
        }

        HAL_GPIO_WritePin(col_ports[col], col_pins[col], GPIO_PIN_RESET);
    }
}

uint16_t Keyboard::NumAvailable()
{
    return ch_ring.Unread();
}

uint8_t Keyboard::Read()
{
    return ch_ring.Read();
}

uint8_t Keyboard::ReadFlags(uint32_t flags)
{
    volatile uint8_t test = this->flags & flags;
    return test;
}

void Keyboard::RaiseFlags(uint32_t flags)
{
    this->flags |= flags;
}

void Keyboard::LowerFlags(uint32_t flags)
{
    this->flags &= ~flags;
}

int8_t Keyboard::FlagPress(const int16_t col, const int16_t row)
{
    // TODO locks

    if (Sym_Col == col && Sym_Row == row)
    {
        RaiseFlags(Sym_Flag);
        return 1;
    }
    if (Alt_Col == col && Alt_Row == row)
    {
        RaiseFlags(Alt_Flag);
        return 1;
    }
    if (Lsh_Col == col && Lsh_Row == row)
    {
        RaiseFlags(Left_Shift_Flag);
        return 1;
    }
    if (Rsh_Col == col && Rsh_Row == row)
    {
        RaiseFlags(Right_Shift_Flag);
        return 1;
    }

    // Mic is unique. Sym flag should be read before the mic flag
    if (!ReadFlags(Sym_Flag) && Mic_Col == col && Mic_Row == row)
    {
        RaiseFlags(Mic_Flag);
        return 1;
    }

    return 0;
}

void Keyboard::FlagRelease(const int16_t col, const int16_t row)
{
    if (Sym_Col == col && Sym_Row == row)
    {
        LowerFlags(Sym_Flag);
        return;
    }
    if (Alt_Col == col && Alt_Row == row)
    {
        LowerFlags(Alt_Flag);
        return;
    }
    if (Lsh_Col == col && Lsh_Row == row)
    {
        LowerFlags(Left_Shift_Flag);
        return;
    }
    if (Rsh_Col == col && Rsh_Row == row)
    {
        LowerFlags(Right_Shift_Flag);
        return;
    }
    if (Mic_Col == col && Mic_Row == row)
    {
        LowerFlags(Mic_Flag);
        return;
    }
}

void Keyboard::KeyPress(const int16_t col, const int16_t row, const uint8_t latch_bit)
{
    // Reset the bit
    latches[col] |= latch_bit;

    if (FlagPress(col, row))
    {
        return;
    }

    if (ReadFlags(Sym_Flag | Sym_Lock_Flag))
    {
        // Symbol
        ch_ring.Write(Symb_Char_Map[col][row]);
    }
    else if (ReadFlags(Left_Shift_Flag | Right_Shift_Flag | Caps_Lock_Flag | Enter_Flag | Back_Flag)
             || (Bak_Col == col && Bak_Row == row) || (Ent_Col == col && Ent_Row == row))
    {
        // Uppercase & certain base characters
        ch_ring.Write(Base_Char_Map[col][row]);
    }
    else
    {
        // Lowercase
        ch_ring.Write(Base_Char_Map[col][row] + CHR_OFFSET);
    }
}

void Keyboard::KeyRelease(const int16_t col, const int16_t row, const uint8_t latch_bit)
{
    latches[col] &= ~latch_bit;

    FlagRelease(col, row);
}