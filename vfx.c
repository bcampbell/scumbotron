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
static uint8_t eduration[MAX_EFFECTS];
static uint8_t ex[MAX_EFFECTS];
static uint8_t ey[MAX_EFFECTS];
static const char* etxt[MAX_EFFECTS];

#define SPAWN_DURATION 16

static void render_spawn(uint8_t e) {
    uint8_t t = SPAWN_DURATION-etimer[e];
    uint8_t cx = ex[e];
    uint8_t cy = ey[e];
    plat_drawbox(cx-t, cy-t, t*2, t*2, DRAWCHR_BLOCK, t);
}

#define KABOOM_DURATION 16

static void render_kaboom(uint8_t e) {
    uint8_t t = etimer[e];
    uint8_t cx = ex[e];
    uint8_t cy = ey[e];
    plat_drawbox(cx-t, cy-t, t*2, t*2, DRAWCHR_BLOCK, t);
}

#define ZOMBIFY_DURATION 16
static void render_zombify(uint8_t e) {
    static const uint8_t greenfade[8] = {1,5,6,6,6,7,7,0};
    uint8_t t = etimer[e];
    uint8_t cx = ex[e];
    uint8_t cy = ey[e];
    uint8_t c = greenfade[t/2];
    plat_drawbox(cx-t, cy-1, t*2, 2, DRAWCHR_BLOCK, c);
    plat_drawbox(cx-1, cy-t, 2, t*2, DRAWCHR_BLOCK, c);
}

#define QUICKTEXT_DURATION 8
static void render_quicktext(uint8_t e) {
    static const uint8_t bluefade[QUICKTEXT_DURATION] = {1,1,1,1,8,9,10,0};

    uint8_t t = etimer[e];
    uint8_t cx = ex[e];
    uint8_t cy = ey[e];
    plat_text(cx,cy,etxt[e],bluefade[t]);
}


#define ALERTTEXT_DURATION 64
static void render_alerttext(uint8_t e) {
    uint8_t cx = ex[e];
    uint8_t cy = ey[e];

    uint8_t c = 15;
    if (etimer[e] == (ALERTTEXT_DURATION-1)) {
        c = 0;
    }
    plat_text(cx, cy, etxt[e], c);
}

#define WARP_DURATION 64
static void render_warp(uint8_t e) {
    uint8_t t = etimer[e];

    uint8_t cx=0;
    uint8_t cy=0;
    uint8_t cw = SCREEN_TEXT_W;
    uint8_t ch = SCREEN_TEXT_H;


    while(cw >= 2 && ch >= 2) {
        uint8_t c = 0;
        if (t >= 16 && t < (WARP_DURATION-(16+8))) {
            c = (t-16)&0x0f;
        }
        ++t;
        plat_drawbox(cx, cy, cw, ch, DRAWCHR_BLOCK, c);
        ++cx;
        ++cy;
        cw -= 2;
        ch -= 2;
    }
}

static void generic_add(int16_t x, int16_t y, uint8_t kind, const char* txt, uint8_t duration)
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
    eduration[e] = duration;
}

void vfx_play_quicktext(int16_t x, int16_t y, const char* txt)
{
    generic_add(x, y, EK_QUICKTEXT, txt, QUICKTEXT_DURATION);
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
    generic_add((cx * 8) << FX, (cy * 8) << FX, EK_ALERTTEXT, txt, ALERTTEXT_DURATION);
}

void vfx_play_kaboom(int16_t x, int16_t y)
{
    generic_add(x, y, EK_KABOOM, 0, KABOOM_DURATION);
}

void vfx_play_spawn(int16_t x, int16_t y)
{
    generic_add(x, y, EK_SPAWN, 0, SPAWN_DURATION);
}

void vfx_play_zombify(int16_t x, int16_t y)
{
    generic_add(x, y, EK_ZOMBIFY, 0, ZOMBIFY_DURATION);
}

void vfx_play_warp()
{
    generic_add(0, 0, EK_WARP, 0, WARP_DURATION);
}


void vfx_init()
{
    // clear effect table
    uint8_t i;
    for(i=0; i<MAX_EFFECTS; ++i) {
        ekind[i] = EK_NONE;
    }
}

void vfx_tick()
{
    uint8_t e;
    for(e = 0; e < MAX_EFFECTS; ++e) {
        if (ekind[e] == EK_NONE) {
            continue;
        }
        ++etimer[e];
        if (etimer[e] >= eduration[e]) {
            ekind[e] = EK_NONE;
        }
    }
}


void vfx_render()
{
    uint8_t e;
    for(e = 0; e < MAX_EFFECTS; ++e) {
        switch (ekind[e]) {
            case EK_NONE: continue;
            case EK_SPAWN: render_spawn(e); break;
            case EK_KABOOM: render_kaboom(e); break;
            case EK_ZOMBIFY: render_zombify(e); break;
            case EK_WARP: render_warp(e); break;
            case EK_QUICKTEXT: render_quicktext(e); break;
            case EK_ALERTTEXT: render_alerttext(e); break;
        }
    }
}

