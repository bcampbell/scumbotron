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
        }
    }
}

