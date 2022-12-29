#ifndef SYS_CX16_H
#define SYS_CX16_H

#include <stdint.h>
#include <stdbool.h>

extern void sys_init();
extern void waitvbl();
extern void inp_tick();

extern void sys_render_start();
extern void sys_render_finish();

extern void sys_clr();
extern void sys_text(uint8_t cx, uint8_t cy, const char* txt, uint8_t colour);
extern void sys_addeffect(int16_t x, int16_t y, uint8_t kind);

extern void sys_hud(uint8_t level, uint8_t lives, uint32_t score);
extern volatile uint16_t inp_joystate;
extern volatile uint8_t tick;

//https://www.nesdev.org/wiki/SNES_controller
/* serial order is:
 0 - B
 1 - Y
 2 - Select
 3 - Start
 4 - Up
 5 - Down
 6 - Left
 7 - Right

 8 - A
 9 - X
10 - L
11 - R
12 - 0
13 - 0
14 - 0
15 - 0
*/



// sfx
#define SFX_NONE 0
#define SFX_LASER 1
#define SFX_KABOOM 2

extern void sys_sfx_play(uint8_t effect);

// (visual) effect kinds
#define EK_NONE 0
#define EK_SPAWN 1
#define EK_KABOOM 2

// core game stuff.
#define SCREEN_W 320
#define SCREEN_H 240

// fixed-point: 6 fractional bits.
#define FX 6
#define FX_ONE (1<<FX)


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

#endif // SYS_CX16_H
