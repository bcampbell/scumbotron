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

static SDL_Palette *palette;

static uint8_t inp_dualstick_state = 0;
static uint8_t inp_menu_state = 0;
static uint8_t inp_menu_pressed = 0;
static uint8_t inp_cheat_state = 0;
static uint8_t inp_cheat_pressed = 0;
static void update_inp_menu();
static void update_inp_cheat();
static void update_inp_dualstick();

// start PLAT_HAS_MOUSE
int16_t plat_mouse_x = 0;
int16_t plat_mouse_y = 0;
uint8_t plat_mouse_buttons = 0;
static uint8_t mouse_watchdog = 0; // >0 = active
// end PLAT_HAS_MOUSE
static void update_inp_mouse();

#ifdef PLAT_HAS_TEXTENTRY
static void textentry_put(char c);
#endif // PLAT_HAS_TEXTENTRY


static void plat_init();
static void plat_render_start();
static void plat_render_finish();

static bool screen_rethink();
static void pumpevents();
static void blit8(const uint8_t *src, int srcw, int srch,
    SDL_Surface *dest, int destx, int desty);
static void blit8_matte(const uint8_t *src, int srcw, int srch,
    SDL_Surface *dest, int destx, int desty, uint8_t matte);

static void hline_noclip(int x_begin, int x_end, int y, uint8_t colour);
static void vline_noclip(int x, int y_begin, int y_end, uint8_t colour);
static void drawrect(const SDL_Rect *r, uint8_t colour);
static void fillrect(const SDL_Rect *r, uint8_t colour);

static inline void sprout16(int16_t x, int16_t y, uint8_t img);
static inline void sprout16_highlight(int16_t x, int16_t y, uint8_t img);
static inline void sprout32(int16_t x, int16_t y, uint8_t img);
static inline void sprout32_highlight(int16_t x, int16_t y, uint8_t img);
static inline void sprout64x8(int16_t x, int16_t y, uint8_t img);
static void rendereffects();

void plat_gatso(uint8_t t)
{
}

void plat_init()
{
    Uint32 flags = SDL_WINDOW_RESIZABLE;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        goto bailout;
    }


//    if (fullscreen)
    flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

    fenster = SDL_CreateWindow("ZapZapZappityZapZap",
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
    sfx_init();
 
    return;

bailout:
    fprintf(stderr, "Init failed: %s\n", SDL_GetError());
    exit(1);
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
          exit(0);
          break;
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
      }
    }
}

// 
static void update_inp_dualstick()
{
    uint8_t state = 0;
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    uint8_t i;
    static const struct {
        uint8_t scancode;
        uint8_t bitmask;
    } mapping[8] = {
        {SDL_SCANCODE_UP, INP_FIRE_UP},
        {SDL_SCANCODE_DOWN, INP_FIRE_DOWN},
        {SDL_SCANCODE_LEFT, INP_FIRE_LEFT},
        {SDL_SCANCODE_RIGHT, INP_FIRE_RIGHT},
        {SDL_SCANCODE_W, INP_UP},
        {SDL_SCANCODE_S, INP_DOWN},
        {SDL_SCANCODE_A, INP_LEFT},
        {SDL_SCANCODE_D, INP_RIGHT},
    };
   
    for (i = 0; i < 8; ++i) {
        if (keys[mapping[i].scancode]) {
            state |= mapping[i].bitmask;
        }
    }
    inp_dualstick_state = state;
}

static void update_inp_mouse()
{
    int x,y;
    Uint32 b = SDL_GetMouseState(&x, &y);
    uint8_t mb = 0;
    if (b & SDL_BUTTON(SDL_BUTTON_LEFT)) {
        mb |= 0x01;
    }
    if (b & SDL_BUTTON(SDL_BUTTON_RIGHT)) {
        mb |= 0x02;
    }
    if (b & SDL_BUTTON(SDL_BUTTON_MIDDLE)) {
        mb |= 0x04;
    }

    float lx, ly;
    SDL_RenderWindowToLogical(renderer, x,y, &lx, &ly);
    int16_t mx = (int16_t)(lx * (float)FX_ONE);
    int16_t my = (int16_t)(ly * (float)FX_ONE);

    if (mb != 0 || mx != plat_mouse_x || my != plat_mouse_y) {
        plat_mouse_buttons = mb;
        plat_mouse_x = mx;
        plat_mouse_y = my;
        mouse_watchdog = 60;
    } else {
       if (mouse_watchdog > 0) {
           --mouse_watchdog;
       }
    }
}

static void update_inp_menu()
{
    uint8_t state = 0;
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    uint8_t i;
    static const struct {
        uint8_t scancode;
        uint8_t bitmask;
    } mapping[8] = {
        {SDL_SCANCODE_UP, INP_UP},
        {SDL_SCANCODE_DOWN, INP_DOWN},
        {SDL_SCANCODE_LEFT, INP_LEFT},
        {SDL_SCANCODE_RIGHT, INP_RIGHT},
        {SDL_SCANCODE_RETURN, INP_MENU_START},
        //{SDL_SCANCODE_SPACE, INP_MENU_A},
        //{SDL_SCANCODE_RCTRL, INP_MENU_B},
    };
   
    for (i = 0; i < 8; ++i) {
        if (keys[mapping[i].scancode]) {
            state |= mapping[i].bitmask;
        }
    }
    // Which ones were pressed since last check?
    inp_menu_pressed = (~inp_menu_state) & state;
    inp_menu_state = state;
}

static void update_inp_cheat()
{
    uint8_t state = 0;
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    if (keys[SDL_SCANCODE_F1]) {
        state |= INP_CHEAT_POWERUP;
    }
    if (keys[SDL_SCANCODE_F2]) {
        state |= INP_CHEAT_EXTRALIFE;
    }
    if (keys[SDL_SCANCODE_F3]) {
        state |= INP_CHEAT_NEXTLEVEL;
    }
    inp_cheat_pressed = (~inp_cheat_state) & state;
    inp_cheat_state = state;
}



uint8_t plat_inp_dualsticks()
{
    return inp_dualstick_state;
}

uint8_t plat_inp_menu()
{
    return inp_menu_pressed;
}

uint8_t plat_inp_cheat()
{
    return inp_cheat_pressed;
}

void plat_render_start()
{
    uint8_t i;
    // clear the screen
    memset(screen->pixels, 0, SCREEN_H*screen->pitch);
    // cycle colour 15
    i = (((tick >> 1)) & 0x7) + 2;
    SDL_SetPaletteColors(palette, &palette->colors[i], 15,1);

    rendereffects();
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

void plat_text(uint8_t cx, uint8_t cy, const char* txt, uint8_t colour)
{
    uint8_t len = 0;
    while(txt[len] != '\0') {
        ++len;
    }
    plat_textn(cx, cy, txt, len, colour);
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
            c = 128 + (*src >> 4);
            blit8_matte(export_chars_bin + (8 * 8 * c), 8, 8, screen, (cx + x) * 8, (cy + y) * 8, colour);
            // right char
            c = 128 + (*src & 0x0f);
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

static void drawrect(const SDL_Rect *r, uint8_t colour)
{
    SDL_Rect b;
    if (!SDL_IntersectRect(&screen->clip_rect, r, &b)) {
        return;
    }

    // top
    hline_noclip(b.x, b.x + b.w, b.y, colour);

    // bottom
    if (b.h<2) {
        return;
    }
    hline_noclip(b.x, b.x + b.w, b.y + b.h - 1, colour);

    // left (excluding top and bottom)
    vline_noclip(b.x, b.y + 1, b.y + b.h - 1, colour);

    // right (excluding top and bottom)
    if (b.w < 2) {
        return;
    }
    vline_noclip(b.x + b.w-1, b.y + 1, b.y + b.h - 1, colour);
}

static void fillrect(const SDL_Rect *r, uint8_t colour)
{
    SDL_Rect b;
    if (!SDL_IntersectRect(&screen->clip_rect, r, &b)) {
        return;
    }

    for (int y = b.y; y < (b.y + b.h); ++y) {
        hline_noclip(b.x, b.x + b.w, y, colour);
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



static inline void sprout16(int16_t x, int16_t y, uint8_t img)
{
    const uint8_t *pix = export_spr16_bin + (img * BYTESIZE_SPR16);
    x = x >> FX;
    y = y >> FX;
    blit8(pix, 16, 16, screen, x, y);
}

static inline void sprout32(int16_t x, int16_t y, uint8_t img)
{
    const uint8_t *pix = export_spr32_bin + (img * BYTESIZE_SPR32);
    x = x >> FX;
    y = y >> FX;
    blit8(pix, 32, 32, screen, x, y);
}

static inline void sprout16_highlight(int16_t x, int16_t y, uint8_t img)
{
    const uint8_t *pix = export_spr16_bin + (img * BYTESIZE_SPR16);
    x = x >> FX;
    y = y >> FX;
    blit8_matte(pix, 16, 16, screen, x, y, 1);
}

static inline void sprout32_highlight(int16_t x, int16_t y, uint8_t img)
{
    const uint8_t *pix = export_spr32_bin + (img * BYTESIZE_SPR32);
    x = x >> FX;
    y = y >> FX;
    blit8_matte(pix, 32, 32, screen, x, y, 1);
}


static inline void sprout64x8(int16_t x, int16_t y, uint8_t img)
{
    const uint8_t *pix = export_spr64x8_bin + (img * BYTESIZE_SPR64x8);
    x = x >> FX;
    y = y >> FX;
    blit8(pix, 64, 8, screen, x, y);
}


#include "../spr_common_inc.h"

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

/*
 * Visual Effects
 */

#define MAX_EFFECTS 8
static uint8_t ekind[MAX_EFFECTS];
static uint8_t etimer[MAX_EFFECTS];
static int16_t ex[MAX_EFFECTS];
static int16_t ey[MAX_EFFECTS];


void plat_addeffect(int16_t x, int16_t y, uint8_t kind)
{
    // find free one
    uint8_t e = 0;
    while( e < MAX_EFFECTS && ekind[e]!=EK_NONE) {
        ++e;
    }
    if (e==MAX_EFFECTS) {
        return; // none free
    }

    ex[e] = x;
    ey[e] = y;
    ekind[e] = kind;
    etimer[e] = 0;
}

static void do_spawneffect(uint8_t e) {
    int t = (int)etimer[e]++;
    int x = ex[e]>>FX;
    int y = ey[e]>>FX;

    t = 14-t;

    if(t>0) {
        SDL_Rect r = {x-t*8, y-t*8, t*8*2, t*8*2};
        drawrect(&r, t);
    } else {
        ekind[e] = EK_NONE;
    }
}

static void do_kaboomeffect(uint8_t e) {
    int t = (int)etimer[e]++;
    int x = ex[e]>>FX;
    int y = ey[e]>>FX;

    int f = t*t;
    if(t<15) {
        SDL_Rect r = {x-f/2, y-f/2, f, f};
        drawrect(&r, t);
    } else {
        ekind[e] = EK_NONE;
    }
}

static void do_zombifyeffect(uint8_t e) {
    int t = (int)etimer[e]++;
    int x = ex[e]>>FX;
    int y = ey[e]>>FX;

    int w = t*32;
    int h = 4 + t;
    if(t<15) {
        SDL_Rect r = {x-w / 2, y-h / 2, w, h};
        fillrect(&r, t);
    } else {
        ekind[e] = EK_NONE;
    }
}


static void rendereffects()
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

#ifdef PLAT_HAS_TEXTENTRY

static char textentry_buf[16] = {0};
static size_t textentry_n = 0;

static void textentry_put(char c)
{
    if (textentry_n < sizeof(textentry_buf)) {
        textentry_buf[textentry_n++] = c;
    }
}

void plat_textentry_start()
{
    SDL_StartTextInput();
    textentry_n = 0;    // clear buffer
}

void plat_textentry_stop()
{
    SDL_StopTextInput();
}

char plat_textentry_getchar()
{
    char c = '\0';
    if (textentry_n > 0) {
        --textentry_n;
        c = textentry_buf[textentry_n];
    }
    return c;
}

#endif // PLAT_HAS_TEXTENTRY


int main(int argc, char* argv[]) {
    plat_init();
    game_init();

    uint64_t starttime = SDL_GetTicks64();
    int frame = 0;
    while(1) {
        pumpevents();
        update_inp_dualstick();
        update_inp_mouse();
        update_inp_menu();
        update_inp_cheat();
        plat_render_start();
        game_render();
        if (mouse_watchdog > 0) {
            sprout16(plat_mouse_x, plat_mouse_y, 0);
        }
        plat_render_finish();

        game_tick();
        ++tick; // The 8bit byte ticker.

        sfx_tick();
        audio_render();

        ++frame;

        uint64_t now = SDL_GetTicks64();
        uint64_t target = starttime + ((frame*1000) / TARGET_FPS);
        int64_t delta = target - now;
        if (delta > 0) {
            SDL_Delay(delta);
        }

    }
}


