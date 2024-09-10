#pragma once

#include "linked_queue.hh"
#include <string>
#include "port_pin.hh"

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

#define CHR_OFFSET 32
// TODO these may need an update
#define ALT   18
#define SYM   17                // CTRL -> remapped to Symbol.
#define SPC   32 - CHR_OFFSET   // This is just 0, but its for readability
#define DLR  '$' - CHR_OFFSET   // For readability
#define MIC   20                // CAPSLOCK -> remapped to mic
#define SHF   16
#define ENT   13
#define BAK    8
#define SPK    7                // Remapped to Speaker
#define NIL '\0'

class Q10Keyboard
{
public:
    Q10Keyboard(port_pin col_pins[Q10_COLS],
                port_pin row_pins[Q10_ROWS],
                unsigned int debounce_duration,
                unsigned int repeat_duration,
                TIM_HandleTypeDef* htim=nullptr);
    ~Q10Keyboard();

    void Begin();
    bool BackPressed();
    void ClearInternalBuffer(unsigned long start = 0, unsigned long end = 0);
    bool EnterPressed();
    bool MicPressed();
    void Read();
    bool Read(std::string &buffer);
    bool Read(char* buffer, size_t &buffer_ptr, const size_t buffer_max_size);
    std::string& GetKeys();
    std::string GetKeysCopy();

    static const unsigned char lshift_flag_b = 0b00000001;
    static const unsigned char rshift_flag_b = 0b00000010;
    static const unsigned char alt_flag_b = 0b00000100;
    static const unsigned char sym_flag_b = 0b00001000;
    static const unsigned char bck_flag_b = 0b00010000;
    static const unsigned char mic_flag_b = 0b00100000;
    static const unsigned char enter_flag_b = 0b01000000;
    static const unsigned char back_flag_b = 0b10000000;

private:
    enum class key_type
    {
        ch = 0,
        special = 1
    };

    void ReadKeys();
    void HandlePress(unsigned char col,
                     unsigned char row,
                     unsigned char read_v);
    bool HandleFlagPress(unsigned char col,
                         unsigned char row,
                         unsigned char read_v);
    void SetAllColumns(GPIO_PinState state);

    const unsigned char Max_Internal_Buffer_Size = 32;

    const char Base_Char_Map[Q10_COLS][Q10_ROWS] =
    {
        { 'Q', 'W', SYM, 'A', ALT, SPC, MIC }, // col1
        { 'E', 'S', 'D', 'P', 'X', 'Z', SHF }, // col2
        { 'R', 'G', 'T', SHF, 'V', 'C', 'F' }, // col3
        { 'U', 'H', 'Y', ENT, 'B', 'N', 'J' }, // col4
        { 'O', 'L', 'I', BAK, DLR, 'M', 'K' }, // col5
    };

    const char Symb_Char_Map[Q10_COLS][Q10_ROWS] =
    {
        { '#', '1', SYM, '*', ALT, SPC, '0' }, // col1
        { '2', '4', '5', '@', '8', '7', SHF }, // col2
        { '3', '/', '(', SHF, '?', '9', '6' }, // col3
        { '_', ':', ')', ENT, '!', ',', ';' }, // col4
        { '+', '"', '-', BAK, SPK, '.', '\''}, // col5
    };

    unsigned char latches[Q10_COLS][Q10_ROWS] =
    {
        { 0, 0, 0, 0, 0, 0, 0 }, // col1
        { 0, 0, 0, 0, 0, 0, 0 }, // col2
        { 0, 0, 0, 0, 0, 0, 0 }, // col3
        { 0, 0, 0, 0, 0, 0, 0 }, // col4
        { 0, 0, 0, 0, 0, 0, 0 }, // col5
    };

    unsigned long debounce_timeout[Q10_COLS][Q10_ROWS] =
    {
        { 0, 0, 0, 0, 0, 0, 0 }, // col1
        { 0, 0, 0, 0, 0, 0, 0 }, // col2
        { 0, 0, 0, 0, 0, 0, 0 }, // col3
        { 0, 0, 0, 0, 0, 0, 0 }, // col4
        { 0, 0, 0, 0, 0, 0, 0 }, // col5
    };

    unsigned char flags = 0b00000000;


    // Pins array
    port_pin* col_pins;
    port_pin* row_pins;
    TIM_HandleTypeDef* htim;

    // Queues for holding the indices we will reference for characters
    // TODO make a queue that utilizes the vector concept
    LinkedQueue<unsigned char> *cols_queue;
    LinkedQueue<unsigned char> *rows_queue;
    LinkedQueue<key_type> *press_type_queue;

    // Flags -- consider removing these and consider adding a queue of flags
    // That will then use binary math to set and unset.
    bool lshift_flag;
    bool rshift_flag;
    bool alt_flag;
    bool sym_flag;
    bool bck_flag;
    bool mic_flag;
    bool enter_flag;
    bool back_flag;

    // Locks
    bool caps_lock;
    bool sym_lock;
    bool special_lock;

    // Character pressed flag
    bool char_pressed;

    std::string internal_buffer;
    size_t internal_buffer_ptr;

    unsigned char last_read_ptr;
    unsigned char available_space;

    bool back_press;
    bool enter_press;

    // The amount of time until the already read key gets read again
    unsigned int debounce_duration;
    // The amount of time until the same can be registered
    unsigned int repeat_duration;
};
