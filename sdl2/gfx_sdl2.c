#include "../plat.h"
#include "../gob.h" // for ZAPPER_*
#include "../misc.h"
#include "plat_sdl2.h"

#include <SDL.h>


extern SDL_Surface* screen;
extern uint16_t screen_w;
extern uint16_t screen_h;


// Exported gfx data and defs.
extern unsigned char export_spr16_bin[];
extern unsigned int export_spr16_bin_len;
extern unsigned char export_spr32_bin[];
extern unsigned int export_spr32_bin_len;
extern unsigned char export_spr64x8_bin[];
extern unsigned int export_spr64x8_bin_len;
extern unsigned char export_chars_bin[];
extern unsigned int export_chars_bin_len;
#define NUM_SPR16 128
#define NUM_SPR32 3
#define BYTESIZE_SPR16 (16*16)   // 16x16 8bpp
#define BYTESIZE_SPR32 (32*32)   // 32x32 8bpp
#define BYTESIZE_SPR64x8 (64*8)   // 64x8 8bpp



static void blit8(const uint8_t *src, int srcw, int srch,
    SDL_Surface *dest, int destx, int desty);
static void blit8_matte(const uint8_t *src, int srcw, int srch,
    SDL_Surface *dest, int destx, int desty, uint8_t matte);

static void hline_noclip(int x_begin, int x_end, int y, uint8_t colour);
static void vline_noclip(int x, int y_begin, int y_end, uint8_t colour);


void plat_clr()
{
}


static uint8_t glyph(char ascii) {
    // 32-126 directly printable
    if (ascii >= 32 && ascii <= 126) {
        return (uint8_t)ascii;
    }
    return 0;
}

void plat_textn(uint8_t cx, uint8_t cy, const char* txt, uint8_t len, uint8_t colour)
{
    int x = cx*8;
    int y = cy*8;

    while(len--) {
        uint8_t i = glyph(*txt++);
        blit8_matte(export_chars_bin + (8 * 8 * i), 8, 8, screen, x, y, colour);
        x += 8;
    }
}

// cw must be even!
void plat_mono4x2(uint8_t cx, int8_t cy, const uint8_t* src, uint8_t cw, uint8_t ch, uint8_t basecol)
{
    int x = cx*8;
    int y = cy*8;

    for (y=0; y < ch; ++y) {
        uint8_t c = (basecol + y) & 0x0f;
        uint8_t colour = (basecol + y) & 0x0f;
        for (x=0; x < cw; x+=2) {
            // left char
            c = DRAWCHR_2x2 + (*src >> 4);
            blit8_matte(export_chars_bin + (8 * 8 * c), 8, 8, screen, (cx + x) * 8, (cy + y) * 8, colour);
            // right char
            c = DRAWCHR_2x2 + (*src & 0x0f);
            blit8_matte(export_chars_bin + (8 * 8 * c), 8, 8, screen, (cx + x + 1) * 8, (cy + y) * 8, colour);
            ++src;
        }
    }
}


void plat_hud(uint8_t level, uint8_t lives, uint32_t score)
{
    char buf[40];
    uint8_t i;
    sprintf(buf, "LV %" PRIu8 "", level);
    plat_text(0, 0, buf, 1);

    sprintf(buf,"%08" PRIu32 "", score);
    plat_text(8, 0, buf, 1);

    for (i = 0; i < 7 && i < lives; ++i) {
        buf[i] = '*';
    }
    if (lives > 7) {
        buf[i++] = '+';
    }
    buf[i] = '\0';
    plat_text(18, 0, buf, 3);
}

static inline uint8_t* pixptr(SDL_Surface* s, int x, int y)
{
    return (uint8_t*)s->pixels + (y*s->pitch) + x;
}

// Covers range [x_begin, x_end).
static void hline_noclip(int x_begin, int x_end, int y, uint8_t colour)
{
    uint8_t *p = pixptr(screen, x_begin, y);
    int n = x_end - x_begin;
    while(n--) {
        *p++ = colour;
    }
}

// Covers range [y_begin, y_end).
static void vline_noclip(int x, int y_begin, int y_end, uint8_t colour)
{
    uint8_t *p = pixptr(screen, x, y_begin);
    int n = y_end - y_begin;
    while(n--) {
        *p = colour;
        p += screen->pitch;
    }
}

static void plonkchar(uint8_t cx, uint8_t cy, uint8_t ch, uint8_t colour)
{
    blit8_matte(export_chars_bin + (8 * 8 * ch), 8, 8, screen, cx*8, cy*8, colour);
}

// Draw vertical line of chars, range [cy_begin, cy_end).
void plat_vline_noclip(uint8_t cx, uint8_t cy_begin, uint8_t cy_end, uint8_t chr, uint8_t colour)
{
    for (uint8_t cy = cy_begin; cy < cy_end; ++cy) {
        plonkchar(cx, cy, chr, colour);
    }
}

// Draw horizontal line of chars, range [cx_begin, cx_end).
void plat_hline_noclip(uint8_t cx_begin, uint8_t cx_end, uint8_t cy, uint8_t chr, uint8_t colour)
{
    //chr=127;
    for (uint8_t cx = cx_begin; cx < cx_end; ++cx) {
        plonkchar(cx, cy, chr, colour);
    }
}



// blit with colour 0 transparent.
static void blit8(const uint8_t *src, int srcw, int srch, SDL_Surface *dest, int destx, int desty)
{
    int y;
    SDL_Rect unclipped = {destx, desty, srcw, srch};
    SDL_Rect r;
    if (!SDL_IntersectRect(&dest->clip_rect, &unclipped, &r)) {
        return;
    }
    for (y = 0; y < r.h; ++y) {
        const uint8_t *in = src + (((r.y-desty) + y) * srcw) + (r.x-destx);
        uint8_t *out = (uint8_t*)(dest->pixels) + ((r.y + y) * dest->pitch) + r.x;
        int x;
        for (x = 0; x < r.w; ++x) {
            uint8_t c = *in++;
            if (c) { *out = c; }    // key==0
            ++out;
        }
    }
}

// blit with colour 0 transparent, replace all other pixels with matte.
static void blit8_matte(const uint8_t *src, int srcw, int srch, SDL_Surface *dest, int destx, int desty, uint8_t matte)
{
    int y;
    SDL_Rect unclipped = {destx, desty, srcw, srch};
    SDL_Rect r;
    if (!SDL_IntersectRect(&dest->clip_rect, &unclipped, &r)) {
        return;
    }
    for (y = 0; y < r.h; ++y) {
        const uint8_t *in = src + (((r.y-desty) + y) * srcw) + (r.x-destx);
        uint8_t *out = (uint8_t*)(dest->pixels) + ((r.y + y) * dest->pitch) + r.x;
        int x;
        for (x = 0; x < r.w; ++x) {
            uint8_t c = *in++;
            if (c) { *out = matte; }
            ++out;
        }
    }
}



void sprout16(int16_t x, int16_t y, uint8_t img)
{
    const uint8_t *pix = export_spr16_bin + (img * BYTESIZE_SPR16);
    x = x >> FX;
    y = y >> FX;
    blit8(pix, 16, 16, screen, x, y);
}

void sprout32(int16_t x, int16_t y, uint8_t img)
{
    const uint8_t *pix = export_spr32_bin + (img * BYTESIZE_SPR32);
    x = x >> FX;
    y = y >> FX;
    blit8(pix, 32, 32, screen, x, y);
}

void sprout16_highlight(int16_t x, int16_t y, uint8_t img)
{
    const uint8_t *pix = export_spr16_bin + (img * BYTESIZE_SPR16);
    x = x >> FX;
    y = y >> FX;
    blit8_matte(pix, 16, 16, screen, x, y, 1);
}

void sprout32_highlight(int16_t x, int16_t y, uint8_t img)
{
    const uint8_t *pix = export_spr32_bin + (img * BYTESIZE_SPR32);
    x = x >> FX;
    y = y >> FX;
    blit8_matte(pix, 32, 32, screen, x, y, 1);
}


void sprout64x8(int16_t x, int16_t y, uint8_t img)
{
    const uint8_t *pix = export_spr64x8_bin + (img * BYTESIZE_SPR64x8);
    x = x >> FX;
    y = y >> FX;
    blit8(pix, 64, 8, screen, x, y);
}



void plat_hzapper_render(int16_t x, int16_t y, uint8_t state) {
    switch(state) {
        case ZAPPER_OFF:
            sprout16(x, y, SPR16_HZAPPER);
            break;
        case ZAPPER_WARMING_UP:
            sprout16(x, y, SPR16_HZAPPER_ON);
            //if (tick & 0x01) {
            //    hline_noclip(0, screen_w, (y >> FX) + 8, 3);
            //}
            break;
        case ZAPPER_ON:
            hline_noclip(0, screen_w, (y >> FX) + 8, 15);
            sprout16(x, y, SPR16_HZAPPER_ON);
            break;
    }
}

void plat_vzapper_render(int16_t x, int16_t y, uint8_t state) {
    switch(state) {
        case ZAPPER_OFF:
            sprout16(x, y, SPR16_VZAPPER);
            break;
        case ZAPPER_WARMING_UP:
            sprout16(x, y, SPR16_VZAPPER_ON);
            //if (tick & 0x01) {
            //    vline_noclip((x>>FX)+8, 0, SCREEN_H, 3);
            //}
            break;
        case ZAPPER_ON:
            vline_noclip((x>>FX)+8, 0, SCREEN_H, 15);
            sprout16(x, y, SPR16_VZAPPER_ON);
            break;
    }
}


