#include "../plat.h"
#include "../gob.h" // for ZAPPER_*
#include "../misc.h"
#include "plat_sdl2.h"
#include "vera_psg.h"

#include <SDL.h>

volatile uint8_t tick = 0;

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

static SDL_Window* fenster = NULL;
static SDL_Renderer* renderer = NULL;

// screen is our virtual screen (8bit indexed)
static SDL_Surface* screen = NULL;
// conversionSurface and screenTexture are used to get screen to
// the renderer.
static SDL_Surface* conversionSurface = NULL;
static SDL_Texture* screenTexture = NULL;

//
uint16_t screen_w;
uint16_t screen_h;

static bool quit = false;

static SDL_Palette *palette;
static char* scoreFile = NULL;


static void plat_render_start();
static void plat_render_finish();

static bool startup();
static void shutdown();
static bool screen_rethink();
static void pumpevents();
static void blit8(const uint8_t *src, int srcw, int srch,
    SDL_Surface *dest, int destx, int desty);
static void blit8_matte(const uint8_t *src, int srcw, int srch,
    SDL_Surface *dest, int destx, int desty, uint8_t matte);

static void hline_noclip(int x_begin, int x_end, int y, uint8_t colour);
static void vline_noclip(int x, int y_begin, int y_end, uint8_t colour);


// from input_sdl2.c:
void controller_init();
void controller_added(int id);
void controller_removed(SDL_JoystickID joyid);
void controller_shutdown();
void mouse_update(SDL_Renderer* rr);
void mouse_render();
#ifdef PLAT_HAS_TEXTENTRY
void textentry_put(char c);
#endif // PLAT_HAS_TEXTENTRY


void plat_gatso(uint8_t t)
{
}

static bool startup()
{
    Uint32 flags = SDL_WINDOW_RESIZABLE;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0) {
        goto bailout;
    }

    // sort out the name for the high score file
    {
        scoreFile = NULL;
        char* path = SDL_GetPrefPath(NULL, "scumbotron");
        if (path) {
            size_t n = strlen(path);
            path = SDL_realloc(path, n + 5);    // + "score"
            if (path) {
                strcpy(path + n, "score");
                scoreFile = path;
            }
        }
    }

//    if (fullscreen)
    flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

    fenster = SDL_CreateWindow("Scumbotron",
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              640, 480, flags);
    if (!fenster) {
        goto bailout;
    }
    SDL_ShowCursor(SDL_DISABLE);

    // Set up palette
    {
        palette = SDL_AllocPalette(256);

        const uint8_t *src = export_palette_bin;
        SDL_Color colors[16];
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

    //
    renderer = SDL_CreateRenderer(fenster, -1, SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        goto bailout;
    }

    if (!screen_rethink()) {
        goto bailout;
    }
    
    psg_reset();
    if (!audio_init()) {
        goto bailout;
    }

    controller_init(); 
    return true;

bailout:
    fprintf(stderr, "Init failed: %s\n", SDL_GetError());
    return false;
}

static void shutdown()
{
    controller_shutdown(); 
    if (scoreFile) {
        SDL_free(scoreFile);
        scoreFile = NULL;
    }
}


// Sets up screen on startup, or reconfigures screen when window is resized.
static bool screen_rethink()
{
    // Free old stuff if we're already running.
    SDL_FreeSurface(screen);
    screen = NULL;
    SDL_FreeSurface(conversionSurface); 
    conversionSurface = NULL;
    if (screenTexture) {
        SDL_DestroyTexture(screenTexture); 
        screenTexture = NULL;
    }

    // Peek at the window size to get the aspect ratio and use
    // that to decide our virtual screen size.
    int w,h;
    SDL_GetWindowSize(fenster, &w, &h);

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
    if (screen == NULL) {
        goto bailout;
    }
    SDL_SetSurfacePalette(screen, palette);
    //SDL_SetColorKey(screen, SDL_TRUE, 0);


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


static void pumpevents()
{
    SDL_PumpEvents();
    SDL_Event event;
    while (SDL_PollEvent(&event) == 1) {
      switch (event.type) {
        case SDL_QUIT:
          quit = true;
          return;
        case SDL_KEYDOWN:
          switch (event.key.keysym.sym) {
            case SDLK_F11:
              if (SDL_GetWindowFlags(fenster) & SDL_WINDOW_FULLSCREEN_DESKTOP) {
                SDL_SetWindowFullscreen(fenster, 0);
              } else {
                SDL_SetWindowFullscreen(fenster, SDL_WINDOW_FULLSCREEN_DESKTOP);
              }
              break;
#ifdef PLAT_HAS_TEXTENTRY
            case SDLK_RETURN:
              if (SDL_IsTextInputActive()) {
                  textentry_put('\n');
              }
              break;
            case SDLK_BACKSPACE:
              if (SDL_IsTextInputActive()) {
                  textentry_put(0x7f);
              }
              break;
            case SDLK_LEFT:
              if (SDL_IsTextInputActive()) {
                  textentry_put('\b');  // nondestructive backspace
              }
              break;
#endif //PLAT_HAS_TEXTENTRY
            default:
              //KeyDown(event.key);
              break;
          }
          break;
        case SDL_WINDOWEVENT:
          {
            if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
              screen_rethink();
            }
          }
          break;
#ifdef PLAT_HAS_TEXTENTRY
        case SDL_TEXTINPUT:
          {
            for (char *p = event.text.text; *p; ++p) {
                // Strip non-ascii chars from the utf-8.
                if ((*p & 0x80) == 0) {
                    textentry_put(*p);
                }
            }
          }
          break;
#endif // PLAT_HAS_TEXTENTRY
        case SDL_CONTROLLERDEVICEADDED:
          controller_added((int)event.cdevice.which);
          break;
        case SDL_CONTROLLERDEVICEREMOVED:
          controller_removed((SDL_JoystickID)event.cdevice.which);
          break;
      }
    }
}




void plat_render_start()
{
    uint8_t i;
    // clear the screen
    memset(screen->pixels, 0, SCREEN_H*screen->pitch);
    // cycle colour 15
    i = (((tick >> 1)) & 0x7) + 2;
    SDL_SetPaletteColors(palette, &palette->colors[i], 15,1);
}

void plat_render_finish()
{

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

bool plat_savescores(const void* begin, int nbytes)
{
    if (!scoreFile) {
        return false;
    }
    SDL_RWops* f = SDL_RWFromFile(scoreFile, "wb");
    if (!f) {
        return false;
    }
    // TODO: should write to temp file then rename.
    size_t n = SDL_RWwrite(f, begin, 1, nbytes);
    SDL_RWclose(f);
    return (n == nbytes);
}

bool plat_loadscores(void* begin, int nbytes)
{
    if (!scoreFile) {
        return false;
    }
    SDL_RWops* f = SDL_RWFromFile(scoreFile, "rb");
    if (!f) {
        return false;
    }
    // TODO: should read via a temp buffer
    size_t n = SDL_RWread(f, begin, 1, nbytes);
    SDL_RWclose(f);
    return (n == nbytes);
}

void plat_quit()
{
    quit = true;
}

int main(int argc, char* argv[]) {
    if (!startup()) {
        shutdown();
        return 1;
    }
    game_init();

    uint64_t starttime = SDL_GetTicks64();
    int frame = 0;
    quit = false;
    while(1) {
        pumpevents();
        if (quit) {
            break;
        }
        // Renderer needed for coord mapping.
        mouse_update(renderer);

        plat_render_start();
        game_render();
        mouse_render();
        plat_render_finish();

        game_tick();
        ++tick; // The 8bit byte ticker.

        audio_render();

        ++frame;

        uint64_t now = SDL_GetTicks64();
        uint64_t target = starttime + ((frame*1000) / TARGET_FPS);
        int64_t delta = target - now;
        if (delta > 0) {
            SDL_Delay(delta);
        }

    }
    shutdown();
    return 0;
}


