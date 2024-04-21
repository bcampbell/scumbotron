#include "../plat.h"
#include "../gob.h" // for ZAPPER_*
#include "../misc.h"
#include "plat_sdl2.h"

#include <SDL.h>

SDL_Renderer* renderer = NULL;
uint16_t screen_w;
uint16_t screen_h;

// screen is our virtual screen (8bit indexed)
static SDL_Surface* screen = NULL;

// separate layer for text, as it's persistant across frames
static SDL_Surface* textLayer = NULL;

// conversionSurface and screenTexture are used to get screen to
// the renderer.
static SDL_Surface* conversionSurface = NULL;
static SDL_Texture* screenTexture = NULL;

static SDL_Palette *palette;


// Exported gfx data and defs.
extern unsigned char export_palette_bin[];
extern unsigned int export_palette_bin_len;
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
static void blit8_mono(const uint8_t *src, int srcw, int srch,
    SDL_Surface *dest, int destx, int desty, uint8_t matte);
static void blit8_solid_mono(const uint8_t *src, int srcw, int srch, SDL_Surface *dest, int destx, int desty, uint8_t colour);

static void hline_noclip(int x_begin, int x_end, int y, uint8_t colour);
static void vline_noclip(int x, int y_begin, int y_end, uint8_t colour);

bool gfx_screen_rethink(int w, int h);


bool gfx_init(SDL_Window* fenster)
{
    renderer = SDL_CreateRenderer(fenster, -1, SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        return false;
    }

    // Set up palette
    {
        palette = SDL_AllocPalette(256);

        const uint8_t *src = export_palette_bin;
        SDL_Color colors[256];
        for (int i = 0; i < 16; ++i) {
            colors[i].r = *src++;
            colors[i].g = *src++;
            colors[i].b = *src++;
            colors[i].a = *src++;
        }
        // fill out unused colours.
        for (int i = 16; i < 256; ++i) {
            colors[i].r = 255;
            colors[i].g = 255;
            colors[i].b = 255;
            colors[i].a = 255;
        }
        SDL_SetPaletteColors(palette, colors, 0, 256);
    }

    // set up drawing
    int w,h;
    SDL_GetWindowSize(fenster, &w, &h);
    if (!gfx_screen_rethink(w, h)) {
        return false;
    }
    return true;
}


// Sets up screen on startup, or reconfigures screen when window is resized.
bool gfx_screen_rethink(int w, int h)
{
    // Free old stuff if we're already running.
    if (screen) {
        SDL_FreeSurface(screen);
        screen = NULL;
    }
    if (textLayer) {
        SDL_FreeSurface(textLayer);
        textLayer = NULL;
    }
    if (conversionSurface) {
        SDL_FreeSurface(conversionSurface); 
        conversionSurface = NULL;
    }
    if (screenTexture) {
        SDL_DestroyTexture(screenTexture); 
        screenTexture = NULL;
    }


    // could use floating point, but why start now?
    int aspect = (1024 * w) / h;

    // We'll clip aspect ratio to reasonable bounds.
    const int min_aspect = (1024 * 4)/3;    // 4:3 eg 640x480
    const int max_aspect = (1024 * 16)/9;   // 16:9 eg 1280x768
    if (aspect < min_aspect) {
        aspect = min_aspect;
    } else if (aspect > max_aspect) {
        aspect = max_aspect;
    }

    // We'll allow wider playfield, but always 240 high.
    screen_h = 240;
    screen_w = (512 + (screen_h * aspect)) / 1024;

    //printf("screen_rethink: window: %dx%d screen: %dx%d (aspect=%f)\n",w,h,screen_w, screen_h, (float)aspect/1024.0f);

    SDL_RenderSetLogicalSize(renderer, screen_w, screen_h);

    screenTexture = SDL_CreateTexture(renderer,
                               SDL_PIXELFORMAT_ARGB8888,
                               SDL_TEXTUREACCESS_STREAMING,
                               screen_w, screen_h);
    if (screenTexture == NULL) {
        goto bailout;
    }


    // Set up screen
    screen = SDL_CreateRGBSurfaceWithFormat(0, screen_w, screen_h, 8,
        SDL_PIXELFORMAT_INDEX8);
    if (!screen) {
        goto bailout;
    }
    SDL_SetSurfacePalette(screen, palette);

    // Set up text layer (overlaid on screen)
    textLayer = SDL_CreateRGBSurfaceWithFormat(0, screen_w, screen_h, 8,
        SDL_PIXELFORMAT_INDEX8);
    if (!textLayer) {
        goto bailout;
    }
    SDL_SetSurfacePalette(textLayer, palette);
    // SDL docs unclear, but I'm assuming key param is a palette index for
    // INDEX8 surfaces. It seems to work, anyway.
    SDL_SetColorKey(textLayer, SDL_TRUE, 0);

    // set up intermediate surface to convert to argb to update texture.
    conversionSurface = SDL_CreateRGBSurfaceWithFormat(0, screen_w, screen_h, 32,
                               SDL_PIXELFORMAT_ARGB8888);
    if (conversionSurface == NULL) {
        goto bailout;
    }
    return true;

bailout:
    return false;
}

void gfx_shutdown()
{
    // should really free stuff here...
}


// Start of frame.
void gfx_render_start()
{
    uint8_t i;
    // clear the screen
    memset(screen->pixels, 0, SCREEN_H*screen->pitch);
    // cycle colour 15
    i = (((tick >> 1)) & 0x7) + 2;
    SDL_SetPaletteColors(palette, &palette->colors[i], 15,1);
}

// Finalise frame and display it.
void gfx_render_finish()
{
    // Overlay text on top of screen
    SDL_BlitSurface(textLayer, NULL, screen, NULL);
    // Convert screen to argb.
    SDL_BlitSurface(screen, NULL, conversionSurface, NULL);
    // Load into texture.
    SDL_UpdateTexture(screenTexture, NULL, conversionSurface->pixels, conversionSurface->pitch);
    SDL_SetRenderDrawColor(renderer, 32,32,32, 255);
    SDL_RenderClear(renderer);
    // Display screen.
    SDL_RenderCopy(renderer, screenTexture, NULL, NULL);
    SDL_RenderPresent(renderer);
}


void plat_clr()
{
    SDL_FillRect(textLayer, NULL, 0);
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
        blit8_solid_mono(export_chars_bin + (8 * 8 * i), 8, 8, textLayer, x, y, colour);
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
            blit8_mono(export_chars_bin + (8 * 8 * c), 8, 8, textLayer, (cx + x) * 8, (cy + y) * 8, colour);
            // right char
            c = DRAWCHR_2x2 + (*src & 0x0f);
            blit8_mono(export_chars_bin + (8 * 8 * c), 8, 8, textLayer, (cx + x + 1) * 8, (cy + y) * 8, colour);
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

// Draw vertical line of chars, range [cy_begin, cy_end).
void plat_vline_noclip(uint8_t cx, uint8_t cy_begin, uint8_t cy_end, uint8_t chr, uint8_t colour)
{
    for (uint8_t cy = cy_begin; cy < cy_end; ++cy) {
    blit8_mono(export_chars_bin + (8 * 8 * chr), 8, 8, screen, cx*8, cy*8, colour);
    }
}

// Draw horizontal line of chars, range [cx_begin, cx_end).
void plat_hline_noclip(uint8_t cx_begin, uint8_t cx_end, uint8_t cy, uint8_t chr, uint8_t colour)
{
    //chr=127;
    for (uint8_t cx = cx_begin; cx < cx_end; ++cx) {
        blit8_mono(export_chars_bin + (8 * 8 * chr), 8, 8, screen, cx*8, cy*8, colour);
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
static void blit8_mono(const uint8_t *src, int srcw, int srch, SDL_Surface *dest, int destx, int desty, uint8_t matte)
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

// blit without matte, converting non-0 colours to colour param
static void blit8_solid_mono(const uint8_t *src, int srcw, int srch, SDL_Surface *dest, int destx, int desty, uint8_t colour)
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
            c = c ? colour : 0;
            *out = c;
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
    blit8_mono(pix, 16, 16, screen, x, y, 1);
}

void sprout32_highlight(int16_t x, int16_t y, uint8_t img)
{
    const uint8_t *pix = export_spr32_bin + (img * BYTESIZE_SPR32);
    x = x >> FX;
    y = y >> FX;
    blit8_mono(pix, 32, 32, screen, x, y, 1);
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


