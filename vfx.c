#include "vfx.h"
#include "plat.h"

// (visual) effect kinds
#define EK_NONE 0
#define EK_SPAWN 1
#define EK_KABOOM 2
#define EK_ZOMBIFY 3
#define EK_WARP 4
#define EK_QUICKTEXT 5
#define EK_ALERTTEXT 6

#define MAX_EFFECTS 8
static uint8_t ekind[MAX_EFFECTS];
static uint8_t etimer[MAX_EFFECTS];
static uint8_t ex[MAX_EFFECTS];
static uint8_t ey[MAX_EFFECTS];
static const char* etxt[MAX_EFFECTS];


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
    static const uint8_t greenfade[8] = {1,5,6,6,6,7,7,0};
    uint8_t t = etimer[e];
    uint8_t cx = ex[e];
    uint8_t cy = ey[e];
    uint8_t c = greenfade[t/2];
    plat_drawbox(cx-t, cy-1, t*2, 2, 1, c);
    plat_drawbox(cx-1, cy-t, 2, t*2, 1, c);
    if (++etimer[e] >= 16) {
        ekind[e] = EK_NONE;
    }
}

static void do_quicktexteffect(uint8_t e) {
    static const uint8_t bluefade[8] = {1,1,1,1,8,9,10,0}; 
    uint8_t t = etimer[e]++;
    uint8_t cx = ex[e];
    uint8_t cy = ey[e];

    plat_text(cx,cy,etxt[e],bluefade[t]);
    if (bluefade[t] == 0) {
        ekind[e] = EK_NONE;
    }
}

static void do_alerttexteffect(uint8_t e) {
    uint8_t cx = ex[e];
    uint8_t cy = ey[e];

    etimer[e]++;
    uint8_t c = 15;
    if (etimer[e] > 64) {
        ekind[e] = EK_NONE;
        c = 0;
    }
    plat_text(cx, cy, etxt[e], c);
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


void vfx_init()
{
    // clear effect table
    uint8_t i;
    for(i=0; i<MAX_EFFECTS; ++i) {
        ekind[i] = EK_NONE;
    }
}


static void generic_add(int16_t x, int16_t y, uint8_t kind, const char* txt)
{
    // find free one
    uint8_t e = 0;
    while( e < MAX_EFFECTS && ekind[e]!=EK_NONE) {
        ++e;
    }
    if (e==MAX_EFFECTS) {
        return; // none free
    }

    ekind[e] = kind;
    ex[e] = (((x >> FX) + 4) / 8);
    ey[e] = (((y >> FX) + 4) / 8);
    etxt[e] = txt;
    etimer[e] = 0;
}

void vfx_play_quicktext(int16_t x, int16_t y, const char* txt)
{
    generic_add(x, y, EK_QUICKTEXT, txt);
}

void vfx_play_alerttext(const char* txt)
{
    uint8_t n = 0;
    const char* p = txt;
    while (*p++) {
        ++n;
    }

    uint8_t cx = (SCREEN_TEXT_W - n)/2;
    uint8_t cy = (SCREEN_TEXT_H / 4);
    generic_add((cx * 8) << FX, (cy * 8) << FX, EK_ALERTTEXT, txt);
}

void vfx_play_kaboom(int16_t x, int16_t y)
{
    generic_add(x, y, EK_KABOOM, 0);
}

void vfx_play_spawn(int16_t x, int16_t y)
{
    generic_add(x, y, EK_SPAWN, 0);
}

void vfx_play_zombify(int16_t x, int16_t y)
{
    generic_add(x, y, EK_ZOMBIFY, 0);
}

void vfx_play_warp()
{
    generic_add(0, 0, EK_WARP, 0);
}


// TODO: kill
void vfx_add(int16_t x, int16_t y, uint8_t kind)
{
    generic_add(x,y, kind, 0);
}


void vfx_render()
{
    uint8_t e;
    for(e = 0; e < MAX_EFFECTS; ++e) {
        switch (ekind[e]) {
            case EK_NONE: continue;
            case EK_SPAWN: do_spawneffect(e); break;
            case EK_KABOOM: do_kaboomeffect(e); break;
            case EK_ZOMBIFY: do_zombifyeffect(e); break;
            case EK_WARP: do_warpeffect(e); break;
            case EK_QUICKTEXT: do_quicktexteffect(e); break;
            case EK_ALERTTEXT: do_alerttexteffect(e); break;
        }
    }
}

