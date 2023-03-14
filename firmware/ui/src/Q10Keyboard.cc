#include "Q10Keyboard.hh"
#include "Helper.h"

Q10Keyboard::Q10Keyboard(port_pin col_pins[Q10_COLS],
                         port_pin row_pins[Q10_ROWS],
                         unsigned int debounce_duration,
                         unsigned int repeat_duration,
                         TIM_HandleTypeDef* htim) :
                         col_pins(col_pins),
                         row_pins(row_pins),
                         htim(htim),
                         lshift_flag(false),
                         rshift_flag(false),
                         alt_flag(false),
                         sym_flag(false),
                         bck_flag(false),
                         mic_flag(false),
                         enter_flag(false),
                         back_flag(false),
                         caps_lock(false),
                         sym_lock(false),
                         special_lock(false),
                         char_pressed(false),
                         internal_buffer_ptr(0),
                         last_read_ptr(0),
                         available_space(Max_Internal_Buffer_Size-1),
                         back_press(false),
                         enter_press(false),
                         debounce_duration(debounce_duration),
                         repeat_duration(repeat_duration)
{
    cols_queue = new LinkedQueue<unsigned char>();
    rows_queue = new LinkedQueue<unsigned char>();
    press_type_queue = new LinkedQueue<key_type>();
}

Q10Keyboard::~Q10Keyboard()
{
    delete cols_queue;
    delete rows_queue;
    delete press_type_queue;

    delete col_pins;
    delete row_pins;
}

// TODO comments
// Assumed the required clocks are enabled
void Q10Keyboard::Begin()
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Set all of the pins to output low and then switch to input

    for (uint8_t i = 0; i < Q10_COLS; i++)
    {
        EnablePortIf(col_pins[i].port);
        HAL_GPIO_WritePin(col_pins[i].port, col_pins[i].pin, GPIO_PIN_RESET);
        GPIO_InitStruct.Pin = col_pins[i].pin;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_PULLDOWN;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(col_pins[i].port, &GPIO_InitStruct);
    }

    for (uint8_t j = 0; j < Q10_ROWS; j++)
    {
        EnablePortIf(row_pins[j].port);
        HAL_GPIO_WritePin(row_pins[j].port, row_pins[j].pin, GPIO_PIN_SET);
        GPIO_InitStruct.Pin = row_pins[j].pin;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_PULLDOWN;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(row_pins[j].port, &GPIO_InitStruct);
    }

    if (htim->Instance)
    {
        HAL_TIM_Base_Start_IT(htim);
    }
}

void Q10Keyboard::ClearInternalBuffer(unsigned long start, unsigned long end)
{
    if (start > end) start = end;
    if (end < start) end = start;
    if (end > internal_buffer.length()) end = internal_buffer.length()-1;

    // Set the internal buffer to be a substring of the previous string
    internal_buffer = internal_buffer.substring(start, end);

    // Set our ptr back to the start of the string
    internal_buffer_ptr = 0;
}

bool Q10Keyboard::EnterPressed()
{
    bool res = enter_press;
    enter_press = false;
    return res;
}

String& Q10Keyboard::GetKeys()
{
    return internal_buffer;
}

void Q10Keyboard::Read()
{
    ReadKeys();
}

bool Q10Keyboard::Read(String &buffer)
{
    ReadKeys();

    bool has_keys = char_pressed || internal_buffer.length();

    if (has_keys)
    {
        buffer += internal_buffer;
        internal_buffer.clear();
    }

    return has_keys;
}

bool Q10Keyboard::Read(char* buffer, size_t &buffer_ptr,
                       const size_t buffer_max_size)
{
    // TODO continue from here and figure out if the logic is correct
    // for copying into the char buffer.
    ReadKeys();
    if (char_pressed)
    {
        while (buffer_ptr < buffer_max_size &&
            internal_buffer_ptr < internal_buffer.length())
        {
            buffer[buffer_ptr++] = internal_buffer[internal_buffer_ptr++];
        }

        ClearInternalBuffer(internal_buffer_ptr, internal_buffer.length()-1);
    }

    return char_pressed;
}

void Q10Keyboard::ReadKeys()
{
    // Disable all pins
    SetAllColumns(GPIO_PIN_RESET);

    bool read_v = 0;

    char_pressed = false;
    for (unsigned char col = 0; col < Q10_COLS; col++)
    {
        if (col > 0)
        {
            // Switch to LOW to power off previous row pins
            HAL_GPIO_WritePin(col_pins[col-1].port, col_pins[col-1].pin, GPIO_PIN_RESET);
        }

        // Raise the column we are reading to HIGH
        HAL_GPIO_WritePin(col_pins[col].port, col_pins[col].pin, GPIO_PIN_SET);

        for (unsigned char row = 0; row < Q10_ROWS; row++)
        {
            // Read the pin
            read_v = HAL_GPIO_ReadPin(row_pins[row].port, row_pins[row].pin);

            // If the key has been pressed and is not already pressed
            if (read_v && read_v != latches[col][row])
            {
                if (!latches[col][row])
                {
                    debounce_timeout[col][row] = HAL_GetTick() + debounce_duration;
                }
                latches[col][row] = read_v;

                HandlePress(col, row, read_v);

                continue;
            }

            // If the key is not longer being pressed and was pressed
            if (!read_v && read_v != latches[col][row])
            {
                latches[col][row] = GPIO_PIN_RESET;
                debounce_timeout[col][row] = 0;

                HandleFlagPress(col, row, read_v);
            }

            // If still holding, start repeating
            if (HAL_GetTick() > debounce_timeout[col][row] && read_v && latches[col][row])
            {
                debounce_timeout[col][row] += repeat_duration;

                HandlePress(col, row, read_v);

                continue;
            }
        }
    }
    // Disable the last column
    HAL_GPIO_WritePin(col_pins[Q10_COLS-1].port,
        col_pins[Q10_COLS-1].pin, GPIO_PIN_RESET);

    // If both left shift and right shift are on then we can toggle shift lock
    // Or if alt and a shift is pressed
    if ((lshift_flag && rshift_flag) ||
        (alt_flag && lshift_flag) ||
        (alt_flag && rshift_flag))
    {
        caps_lock = !caps_lock;
    }

    if (alt_flag && sym_flag)
    {
        sym_lock = !sym_lock;
    }

    // unsigned char ch;
    unsigned char row;
    unsigned char col;
    key_type press_type;

    // To save on cycles we only check one of the queues because they are
    // parallel queues.
    while (!cols_queue->Empty() &&
           internal_buffer_ptr < Max_Internal_Buffer_Size &&
           internal_buffer_ptr <= available_space)
    {
        col = cols_queue->Front();
        row = rows_queue->Front();
        press_type = press_type_queue->Front();

        if (sym_flag || sym_lock)
        {
            // Append a symbol instead of a character.
            internal_buffer.push_back(Symb_Char_Map[col][row]);
        }
        else if (lshift_flag ||
                 rshift_flag ||
                 caps_lock ||
                 press_type == key_type::special)
        {
            // Append a capital letter, space, or $ to the buffer
            internal_buffer.push_back(Base_Char_Map[col][row]);
        }
        else
        {
            // Append a lower case letter to the buffer
            internal_buffer.push_back(static_cast<unsigned char>(
                Base_Char_Map[col][row] + CHR_OFFSET));
        }

        char_pressed = true;
        cols_queue->Pop();
        rows_queue->Pop();
        press_type_queue->Pop();
    }

    if (internal_buffer_ptr < Max_Internal_Buffer_Size &&
        internal_buffer_ptr <= available_space)
    {
        if (back_flag)
        {
            // Set the delete flag for the interpreter to handle it
            char_pressed = true;
            back_press = true;
            back_flag = false;
        }

        if (enter_flag)
        {
            // Set the enter flag for the interpreter can handle it
            char_pressed = true;
            enter_press = true;
            enter_flag = false;
        }
    }
}

bool Q10Keyboard::BackPressed()
{
    bool res = back_press;
    back_press = false;
    return res;
}

void Q10Keyboard::HandlePress(unsigned char col, unsigned char row, unsigned char read_v)
{
    key_type press_type = key_type::ch;

    // If this was a flag press then return
    if (HandleFlagPress(col, row, read_v)) return;

    if (BAK_Col == col && BAK_Row == row)
    {
        press_type = key_type::special;
        back_flag = read_v;
        return;
    }
    if (ENT_Col == col && ENT_Row == row)
    {
        press_type = key_type::special;
        enter_flag = read_v;
        return;
    }

    cols_queue->Enqueue(col);
    rows_queue->Enqueue(row);
    press_type_queue->Enqueue(press_type);
}

bool Q10Keyboard::HandleFlagPress(unsigned char col, unsigned char row, unsigned char read_v)
{
    // Check all of the special flags
    // TODO may not need read_v
    if (SYM_Col == col && SYM_Row == row) { sym_flag = read_v; return true; }
    if (ALT_Col == col && ALT_Row == row) { alt_flag = read_v; return true; }
    if (LSH_Col == col && LSH_Row == row) { lshift_flag = read_v; return true; }
    if (RSH_Col == col && RSH_Row == row) { rshift_flag = read_v; return true; }
    // Mic is unique. Sym flag should be read before the mic flag
    if (!sym_flag && MIC_Col == col && MIC_Row == row) { mic_flag = read_v; return true; }

    return false;
}

void Q10Keyboard::SetAllColumns(GPIO_PinState state)
{
    for (unsigned char col = 0; col < Q10_COLS; col++)
    {
        HAL_GPIO_WritePin(col_pins[col].port, col_pins[col].pin, state);
    }
}