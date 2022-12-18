#include "sys.h"

#include <SDL.h>

volatile uint16_t inp_joystate = 0;
volatile uint8_t tick = 0;


static SDL_Window* fenster = NULL;
static SDL_Renderer* renderer = NULL;

static void pumpevents();

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


static void sprout(int16_t x, int16_t y, uint8_t img)
{
    x = x >> FX;
    y = y >> FX;
    SDL_Rect rect = {(int)x, (int)y, 16, 16};
    SDL_SetRenderDrawColor(renderer,128,128,128,255);
    SDL_RenderDrawRect(renderer, &rect);
}

static void sys_spr32(int16_t x, int16_t y, uint8_t img)
{
    x = x >> FX;
    y = y >> FX;
    SDL_Rect rect = {(int)x, (int)y, 32, 32};
    SDL_SetRenderDrawColor(renderer,128,128,128,255);
    SDL_RenderDrawRect(renderer, &rect);
}

void sys_player_render(int16_t x, int16_t y)
{
    sprout(x, y, 0);
}

void sys_shot_render(int16_t x, int16_t y, uint8_t direction)
{
    sprout(x, y, 0);
}
void sys_block_render(int16_t x, int16_t y)
{
    sprout(x, y, 0);
}

void sys_grunt_render(int16_t x, int16_t y)
{
    sprout(x, y, 0);
}

void sys_baiter_render(int16_t x, int16_t y)
{
    sprout(x, y, 0);
}

void sys_tank_render(int16_t x, int16_t y, bool highlight)
{
    x = x >> FX;
    y = y >> FX;
    SDL_Rect rect = {(int)x, (int)y, 16, 16};
    if (highlight) {
        SDL_SetRenderDrawColor(renderer,255,255,255,255);
    } else {
        SDL_SetRenderDrawColor(renderer,128,0,0,255);
    }
    SDL_RenderDrawRect(renderer, &rect);
}

void sys_amoeba_big_render(int16_t x, int16_t y)
{
    sys_spr32(x, y, 0);
}

void sys_amoeba_med_render(int16_t x, int16_t y)
{
    sprout(x, y, 0);
}

void sys_amoeba_small_render(int16_t x, int16_t y)
{
    sprout(x, y, 0);
}



