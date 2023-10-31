#include "effects.h"
#include "plat.h"

#define MAX_EFFECTS 8
static uint8_t ekind[MAX_EFFECTS];
static uint8_t etimer[MAX_EFFECTS];
static uint8_t ex[MAX_EFFECTS];
static uint8_t ey[MAX_EFFECTS];



static void do_spawneffect(uint8_t e) {
    uint8_t t = 16-etimer[e];
    uint8_t cx = ex[e];
    uint8_t cy = ey[e];
    plat_drawbox(cx-t, cy-t, t*2, t*2, 1, t);
    if (++etimer[e] >= 16) {
        ekind[e] = EK_NONE;
    }
}

static void do_kaboomeffect(uint8_t e) {
    uint8_t t = etimer[e];
    uint8_t cx = ex[e];
    uint8_t cy = ey[e];
    plat_drawbox(cx-t, cy-t, t*2, t*2, 1, t);
    if (++etimer[e] >= 16) {
        ekind[e] = EK_NONE;
    }
}

static void do_zombifyeffect(uint8_t e) {
    uint8_t t = etimer[e];
    uint8_t cx = ex[e];
    uint8_t cy = ey[e];
    plat_drawbox(cx-t, cy-1, t*2, 2, 1, t);
    if (++etimer[e] >= 16) {
        ekind[e] = EK_NONE;
    }
}



static void do_warpeffect(uint8_t e) {
    uint8_t t = etimer[e];

    uint8_t cx=0;
    uint8_t cy=0;
    uint8_t cw = SCREEN_TEXT_W;
    uint8_t ch = SCREEN_TEXT_H;


    const uint8_t DUR = 64;
    while(cw >= 2 && ch >= 2) {
        uint8_t c = 0;
        if (t >= 16 && t < (DUR-(16+8))) {
            c = (t-16)&0x0f;
        }
        ++t;
        plat_drawbox(cx, cy, cw, ch, 1, c);
        ++cx;
        ++cy;
        cw -= 2;
        ch -= 2;
    }
    if (++etimer[e] >= DUR) {
        ekind[e] = EK_NONE;
    }
}


void effects_init()
{
    // clear effect table
    uint8_t i;
    for(i=0; i<MAX_EFFECTS; ++i) {
        ekind[i] = EK_NONE;
    }
}


void effects_add(int16_t x, int16_t y, uint8_t kind)
{
    // find free one
    uint8_t e = 0;
    while( e < MAX_EFFECTS && ekind[e]!=EK_NONE) {
        ++e;
    }
    if (e==MAX_EFFECTS) {
        return; // none free
    }

    ex[e] = (((x >> FX) + 4) / 8);
    ey[e] = (((y >> FX) + 4) / 8);
    ekind[e] = kind;
    etimer[e] = 0;
}


void effects_render()
{
    uint8_t e;
    for(e = 0; e < MAX_EFFECTS; ++e) {
        switch (ekind[e]) {
            case EK_NONE: continue;
            case EK_SPAWN: do_spawneffect(e); break;
            case EK_KABOOM: do_kaboomeffect(e); break;
            case EK_ZOMBIFY: do_zombifyeffect(e); break;
            case EK_WARP: do_warpeffect(e); break;
        }
    }
}

