#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>

/* RGB565 color helpers */
#define RGB565(r, g, b) \
    ((uint16_t)(((r) & 0x1F) << 11) | (((g) & 0x3F) << 5) | ((b) & 0x1F))

#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_YELLOW  RGB565(31, 63, 0)
#define COLOR_RED     RGB565(31, 0, 0)
#define COLOR_GREEN   RGB565(0, 63, 0)
#define COLOR_BLUE    RGB565(0, 0, 31)
#define COLOR_CYAN    RGB565(0, 63, 31)
#define COLOR_MAGENTA RGB565(31, 0, 31)
#define COLOR_ORANGE  RGB565(31, 40, 0)

/* Brick row colors */
static inline uint16_t brick_row_color(int row)
{
    switch (row) {
    case 0: return COLOR_RED;
    case 1: return COLOR_ORANGE;
    case 2: return COLOR_GREEN;
    case 3: return COLOR_CYAN;
    default: return COLOR_WHITE;
    }
}

#endif /* UTIL_H */
