#include "plat.h"

// common implementations of plat_drawbox() and plat_text()

static inline int8_t cclip(int8_t v, int8_t low, int8_t high)
{
    if (v < low) {
        return low;
    } else if (v > high) {
        return high;
    } else {
        return v;
    }
}

// draw box in char coords, with clipping
// (note cx,cy can be negative)
void plat_drawbox(int8_t cx, int8_t cy, uint8_t w, uint8_t h, uint8_t ch, uint8_t colour)
{
    int8_t x0,y0,x1,y1;
    x0 = cclip(cx, 0, SCREEN_W / 8);
    x1 = cclip(cx + w, 0, SCREEN_W / 8);
    y0 = cclip(cy, 0, SCREEN_H / 8);
    y1 = cclip(cy + h, 0, SCREEN_H / 8);

    // top
    if (y0 == cy) {
        plat_hline_noclip((uint8_t)x0, (uint8_t)x1, (uint8_t)y0, ch, colour);
    }
    if (h<=1) {
        return;
    }

    // bottom
    if (y1 - 1 == cy + h-1) {
        plat_hline_noclip((uint8_t)x0, (uint8_t)x1, (uint8_t)y1 - 1, ch, colour);
    }
    if (h<=2) {
        return;
    }

    // left (excluding top and bottom)
    if (x0 == cx) {
        plat_vline_noclip((uint8_t)x0, (uint8_t)y0, (uint8_t)y1, ch, colour);
    }
    if (w <= 1) {
        return;
    }

    // right (excluding top and bottom)
    if (x1 - 1 == cx + w - 1) {
        plat_vline_noclip((uint8_t)x1 - 1, (uint8_t)y0, (uint8_t)y1, ch, colour);
    }
}

void plat_text(uint8_t cx, uint8_t cy, const char* txt, uint8_t colour)
{
    uint8_t len = 0;
    while(txt[len] != '\0') {
        ++len;
    }
    plat_textn(cx, cy, txt, len, colour);
}

