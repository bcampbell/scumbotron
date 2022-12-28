#include "sys.h"

#include <SDL.h>

volatile uint16_t inp_joystate = 0;
volatile uint8_t tick = 0;



static void pumpevents();


extern unsigned char export_palette_bin[];
extern unsigned int export_palette_bin_len;
extern unsigned char export_spr16_bin[];
extern unsigned int export_spr16_bin_len;
extern unsigned char export_spr32_bin[];
extern unsigned int export_spr32_bin_len;

// sprite image defs
#define SPR16_BAITER 16
#define SPR16_AMOEBA_MED 20
#define SPR16_AMOEBA_SMALL 24
#define SPR16_TANK 28
#define SPR16_GRUNT 30
#define SPR16_SHOT 4

#define SPR32_AMOEBA_BIG 0

#define NUM_SPR16 64
#define NUM_SPR32 8

#define BYTESIZE_SPR16 ((16*16)/2)   // 16x16 4bpp
#define BYTESIZE_SPR32 ((32*32)/2)   // 32x32 4bpp

static SDL_Window* fenster = NULL;
static SDL_Renderer* renderer = NULL;
static SDL_Texture* screenTexture = NULL;
static SDL_Surface* screen = NULL;
static SDL_Surface* conversionSurface = NULL;
static SDL_Palette *palette;


void sys_init()
{
    Uint32 flags = SDL_WINDOW_RESIZABLE;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        goto bailout;
    }


//    if (fullscreen)
//        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

    fenster = SDL_CreateWindow("ZapZapZappityZapZap",
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              SCREEN_W, SCREEN_H, flags);
    if (!fenster) {
        goto bailout;
    }

    renderer = SDL_CreateRenderer(fenster, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        goto bailout;
    }

    SDL_RenderSetLogicalSize(renderer, SCREEN_W, SCREEN_H);

    screenTexture = SDL_CreateTexture(renderer,
                               SDL_PIXELFORMAT_ARGB8888,
                               SDL_TEXTUREACCESS_STREAMING,
                               SCREEN_W, SCREEN_H);
    if (screenTexture == NULL) {
        goto bailout;
    }


    // Set up palette
    {
        palette = SDL_AllocPalette(256);

        const uint16_t *src = (const uint16_t*)export_palette_bin;
        SDL_Color colors[16];
        for (int i = 0; i < 16; ++i) {
		    // VERA format is:
            // xxxxrrrrggggbbbb
            uint16_t c = *src++;
            uint8_t r = (c>>8) & 0x000f;
            uint8_t g = (c>>4) & 0x000f;
            uint8_t b = (c>>0) & 0x000f;

            // nibble-double to get 8bit components
            colors[i].r = (r<<4) | r;
            colors[i].g = (g<<4) | g;
            colors[i].b = (b<<4) | b;
            colors[i].a = 255;
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

    // Set up screen
    screen = SDL_CreateRGBSurfaceWithFormat(0, SCREEN_W, SCREEN_H, 8,
        SDL_PIXELFORMAT_INDEX8);
    if (screen == NULL) {
        goto bailout;
    }
    SDL_SetSurfacePalette(screen, palette);
    //SDL_SetColorKey(screen, SDL_TRUE, 0);


    // set up intermediate surface to convert to argb to update texture.
    conversionSurface = SDL_CreateRGBSurfaceWithFormat(0, SCREEN_W, SCREEN_H, 32,
                               SDL_PIXELFORMAT_ARGB8888);
    if (conversionSurface == NULL) {
        goto bailout;
    }
    return;

bailout:
    fprintf(stderr, "Init failed: %s\n", SDL_GetError());
    exit(1);
}


#define TICK_INTERVAL (1000/60)

static Uint32 prevtime = 0;
void waitvbl()
{
    pumpevents();

    Uint32 targtime = prevtime + TICK_INTERVAL;
    Uint32 now = SDL_GetTicks();
    if(targtime > now) {
      //  printf("delay for %d\n", targtime-now);
      SDL_Delay(targtime - now);
    }
    prevtime = targtime;
    ++tick;
}

static void pumpevents()
{
    SDL_PumpEvents();
    SDL_Event event;
    while (SDL_PollEvent(&event) == 1) {
      switch (event.type) {
        case SDL_QUIT:
          exit(0);
          break;
        case SDL_KEYDOWN:
          switch (event.key.keysym.sym) {
            case SDLK_F12:
              //g_Display->ScreenShot();
              break;
            default:
              //KeyDown(event.key);
              break;
          }
          break;
      }
    }
}

// hackhackhack
#define JOY_UP_MASK 0x08
#define JOY_DOWN_MASK 0x04
#define JOY_LEFT_MASK 0x02
#define JOY_RIGHT_MASK 0x01
#define JOY_BTN_1_MASK 0x80

void inp_tick()
{
    uint16_t j=0;
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    if (keys[SDL_SCANCODE_UP]) {
        j |= JOY_UP_MASK;
    }
    if (keys[SDL_SCANCODE_DOWN]) {
        j |= JOY_DOWN_MASK;
    }
    if (keys[SDL_SCANCODE_LEFT]) {
        j |= JOY_LEFT_MASK;
    }
    if (keys[SDL_SCANCODE_RIGHT]) {
        j |= JOY_RIGHT_MASK;
    }
    if (keys[SDL_SCANCODE_Z]) {
        j |= JOY_BTN_1_MASK;
    }
    inp_joystate = ~j;
}

void sys_render_start()
{
    // clear the screen
    memset(screen->pixels, 0, SCREEN_H*screen->pitch);

    // cycle colour 15
    uint8_t i = (((tick >> 1)) & 0x7) + 2;
    SDL_SetPaletteColors(palette, &palette->colors[i], 15,1);
}

void sys_render_finish()
{
    // convert screen to argb
    SDL_BlitSurface(screen, NULL, conversionSurface, NULL);

    // load into tetxure
    SDL_UpdateTexture(screenTexture, NULL, conversionSurface->pixels, conversionSurface->pitch);
    // display it!
    SDL_RenderCopy(renderer, screenTexture, NULL, NULL);
    //SDL_RenderDrawRect(renderer, NULL);
    SDL_RenderPresent(renderer);
}

void sys_clr()
{
}

void sys_text(uint8_t cx, uint8_t cy, const char* txt, uint8_t colour)
{
    size_t l = strlen(txt); 
    SDL_Rect rect = {(int)cx*8, (int)cy*8, (int)l*8, 8};
    SDL_SetRenderDrawColor(renderer,128,128,128,255);
    SDL_RenderDrawRect(renderer, &rect);
}

void sys_addeffect(int16_t x, int16_t y, uint8_t kind)
{
}

void sys_hud(uint8_t level, uint8_t lives, uint32_t score)
{
}

void sys_sfx_play(uint8_t effect)
{
}


static void blit_4bpp(SDL_Surface *dest, SDL_Rect const* destrect, const uint8_t *src)
{
    // TODO: clip.
    int y;
    for (y = 0; y < destrect->h; ++y) {
        uint8_t *pix = (uint8_t*)(dest->pixels) + ((destrect->y + y) * dest->pitch) + destrect->x;
        int x;
        for (x = 0; x < destrect->w; x += 2) {
            uint8_t c;
            c = (*src >> 4) & 0x0f;
            if (c) { *pix = c; }
            ++pix;
            c = *src & 0x0f;
            if (c) { *pix = c; }
            ++pix;
            ++src;
        }
    }
}



void sprout16(int16_t x, int16_t y, uint8_t img)
{
    x = x >> FX;
    y = y >> FX;
    SDL_Rect rect = {(int)x, (int)y, 16, 16};
    blit_4bpp(screen, &rect, export_spr16_bin + (img * BYTESIZE_SPR16));
}

void sprout32(int16_t x, int16_t y, uint8_t img)
{
    x = x >> FX;
    y = y >> FX;
    SDL_Rect rect = {(int)x, (int)y, 32, 32};
    blit_4bpp(screen, &rect, export_spr32_bin + (img * BYTESIZE_SPR32));
}

// TODO
void sprout16_highlight(int16_t x, int16_t y, uint8_t img)
{
    x = x >> FX;
    y = y >> FX;
    SDL_Rect rect = {(int)x, (int)y, 16, 16};
    blit_4bpp(screen, &rect, export_spr16_bin + (img * BYTESIZE_SPR16));
}

const uint8_t shot_spr[16] = {
    0,              // 0000
    SPR16_SHOT+2,   // 0001 DIR_RIGHT
    SPR16_SHOT+2,   // 0010 DIR_LEFT
    0,              // 0011
    SPR16_SHOT+0,   // 0100 DIR_DOWN
    SPR16_SHOT+3,   // 0101 down+right           
    SPR16_SHOT+1,   // 0110 down+left           
    0,              // 0111

    SPR16_SHOT+0,   // 1000 up
    SPR16_SHOT+1,   // 1001 up+right
    SPR16_SHOT+3,   // 1010 up+left
    0,              // 1011
    0,              // 1100 up+down
    0,              // 1101 up+down+right           
    0,              // 1110 up+down+left           
    0,              // 1111
};


