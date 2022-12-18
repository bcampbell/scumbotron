#include "sys.h"

#include <SDL.h>

volatile uint16_t inp_joystate = 0;
volatile uint8_t tick = 0;


static SDL_Window* fenster = NULL;
static SDL_Renderer* renderer = NULL;

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
static SDL_Texture* spr16[NUM_SPR16] = {0};
static SDL_Texture* spr32[NUM_SPR32] = {0};
static SDL_Palette *palette;

static SDL_Texture* raw_to_texture(const uint8_t* pixels, int w, int h);

void sys_init()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
      fprintf(stderr, "SDL_Init() failed: %s\n", SDL_GetError());
      exit(1);
    }

    Uint32 flags = SDL_WINDOW_RESIZABLE;

//    if (fullscreen)
//        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

    fenster = SDL_CreateWindow("laser zappy zap zap zap",
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              640, 480, flags);

    renderer = SDL_CreateRenderer(fenster, -1, SDL_RENDERER_ACCELERATED);

    if (!fenster) {
        fprintf(stderr, "SDL_CreateWindow() failed: %s\n", SDL_GetError());
        exit(1);
    }
    SDL_RenderSetLogicalSize(renderer, SCREEN_W, SCREEN_H);

    // Set up palette
    {
        palette = SDL_AllocPalette(16);

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
            colors[i].a = 255;  //i==0 ? 0 : 255;   // colour 0 transparent
        }
        SDL_SetPaletteColors(palette, colors, 0, 16);
        /*
        for (int i=0; i<palette->ncolors; ++i) {
            printf("%d: %d,%d,%d,%d\n", i, palette->colors[i].r, palette->colors[i].g, palette->colors[i].b, palette->colors[i].a);
        }
        */
    }

    // Set up sprites
    {
        const uint8_t *src = export_spr16_bin;
        for( int i = 0; i < NUM_SPR16; ++i) {
            spr16[i] = raw_to_texture(src, 16, 16);
            if (spr16[i] == NULL) {
                exit(1);
            }
            src += 8*16;        // 16x16 4bpp
        }
    }
    {
        const uint8_t *src = export_spr32_bin;
        for( int i = 0; i < NUM_SPR32; ++i) {
            spr32[i] = raw_to_texture(src, 32, 32);
            if (spr32[i] == NULL) {
                exit(1);
            }
            src += 16*32;        // 32x32 4bpp
        }
    }
}


static SDL_Texture* raw_to_texture(const uint8_t* pixels, int w, int h)
{
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, w, h, 4,
        SDL_PIXELFORMAT_INDEX4MSB);
    if (surface == NULL) {
        fprintf(stderr, "SDL_CreateRGBSurfaceWithFormat() failed: %s\n", SDL_GetError());
        return NULL;
    }
    SDL_SetSurfacePalette(surface, palette);
    SDL_SetColorKey(surface, SDL_TRUE, 0);
    // copy in the image
    memcpy(surface->pixels, pixels, h*surface->pitch);

    SDL_Texture *t = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (t == NULL) {
        fprintf(stderr, "SDL_CreateTextureFromSurface() failed: %s\n", SDL_GetError());
        return NULL;
    }

    return t;
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
    SDL_SetRenderDrawColor(renderer,0,0,0,255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 128,128,128,255);
    SDL_RenderDrawRect(renderer, NULL);
}

void sys_render_finish()
{
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


void sprout16(int16_t x, int16_t y, uint8_t img)
{
    x = x >> FX;
    y = y >> FX;
    SDL_Rect rect = {(int)x, (int)y, 16, 16};
    SDL_RenderCopy(renderer, spr16[img], NULL, &rect);
//    SDL_SetRenderDrawColor(renderer, 128,128,128,255);
//    SDL_RenderDrawRect(renderer, &rect);
}

void sprout32(int16_t x, int16_t y, uint8_t img)
{
    x = x >> FX;
    y = y >> FX;
    SDL_Rect rect = {(int)x, (int)y, 32, 32};
    SDL_RenderCopy(renderer, spr32[img], NULL, &rect);
}

// TODO
void sprout16_highlight(int16_t x, int16_t y, uint8_t img)
{
    x = x >> FX;
    y = y >> FX;
    SDL_Rect rect = {(int)x, (int)y, 16, 16};
    SDL_RenderCopy(renderer, spr16[img], NULL, &rect);
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


