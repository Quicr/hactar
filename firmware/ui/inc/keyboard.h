#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "stm32.h"

#include "static_ring_buffer.h"

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

typedef struct
{
    GPIO_TypeDef col_ports[Q10_COLS];
    uint16_t col_pins[Q10_COLS];
    GPIO_TypeDef row_ports[Q10_ROWS];
    uint16_t row_pins[Q10_ROWS];
    StaticRingBuffer ring;
} Keyboard;

void KB_Scan(Keyboard* keyboard);
uint8_t KB_Read();



#endif