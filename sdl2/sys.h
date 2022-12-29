#ifndef SYS_SDL2_H
#define SYS_SDL2_H

/*
 * Common interface for all platforms
 */

#include <stdint.h>
#include <stdbool.h>

void sys_init();
void waitvbl();
void inp_tick();

void sys_render_start();
void sys_render_finish();
void sys_sfx_play(uint8_t effect);

void sys_clr();
void sys_text(uint8_t cx, uint8_t cy, const char* txt, uint8_t colour);
void sys_addeffect(int16_t x, int16_t y, uint8_t kind);

void sys_hud(uint8_t level, uint8_t lives, uint32_t score);
extern volatile uint16_t inp_joystate;
extern volatile uint8_t tick;

void sys_player_render(int16_t x, int16_t y);
void sys_shot_render(int16_t x, int16_t y, uint8_t direction);
void sys_block_render(int16_t x, int16_t y);
void sys_grunt_render(int16_t x, int16_t y);
void sys_baiter_render(int16_t x, int16_t y);
void sys_tank_render(int16_t x, int16_t y, bool highlight);
void sys_amoeba_big_render(int16_t x, int16_t y);
void sys_amoeba_med_render(int16_t x, int16_t y);
void sys_amoeba_small_render(int16_t x, int16_t y);
void sys_hzapper_render(int16_t x, int16_t y, uint8_t state);
void sys_vzapper_render(int16_t x, int16_t y, uint8_t state);

// fixed-point: 6 fractional bits.
#define FX 6
#define FX_ONE (1<<FX)

// sfx
#define SFX_NONE 0
#define SFX_LASER 1
#define SFX_KABOOM 2


// (visual) effect kinds
#define EK_NONE 0
#define EK_SPAWN 1
#define EK_KABOOM 2

// core game stuff.
#define SCREEN_W 320
#define SCREEN_H 240


#endif // SYS_SDL2_H
