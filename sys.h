#ifndef SYS_H
#define SYS_H

#include "plat_details.h"    // platform-specific details (screen size etc...)

// provided by game.c:
void game_init();
void game_tick();
void game_render();

/*
 * Common interface for all platforms
 */

#include <stdint.h>
#include <stdbool.h>

// fixed-point: 6 fractional bits.
#define FX 6
#define FX_ONE (1<<FX)

void sys_clr();
void sys_text(uint8_t cx, uint8_t cy, const char* txt, uint8_t colour);

void sys_hud(uint8_t level, uint8_t lives, uint32_t score);
extern volatile uint8_t tick;

void sys_player_render(int16_t x, int16_t y);
void sys_shot_render(int16_t x, int16_t y, uint8_t direction);
void sys_powerup_render(int16_t x, int16_t y, uint8_t kind);
void sys_block_render(int16_t x, int16_t y);
void sys_grunt_render(int16_t x, int16_t y);
void sys_baiter_render(int16_t x, int16_t y);
void sys_tank_render(int16_t x, int16_t y, bool highlight);
void sys_amoeba_big_render(int16_t x, int16_t y);
void sys_amoeba_med_render(int16_t x, int16_t y);
void sys_amoeba_small_render(int16_t x, int16_t y);
void sys_hzapper_render(int16_t x, int16_t y, uint8_t state);
void sys_vzapper_render(int16_t x, int16_t y, uint8_t state);
void sys_fragger_render(int16_t x, int16_t y);
void sys_frag_render(int16_t x, int16_t y, uint8_t dir);
void sys_vulgon_render(int16_t x, int16_t y, bool highlight);

// noddy profiling
#define GATSO_OFF 0
#define GATSO_ON 1
void sys_gatso(uint8_t t);

// sfx
#define SFX_NONE 0
#define SFX_LASER 1
#define SFX_KABOOM 2
void sys_sfx_play(uint8_t effect);

// (visual) effect kinds
#define EK_NONE 0
#define EK_SPAWN 1
#define EK_KABOOM 2
void sys_addeffect(int16_t x, int16_t y, uint8_t kind);


#define DIR_UP 0x08
#define DIR_DOWN 0x04
#define DIR_LEFT 0x02
#define DIR_RIGHT 0x01

// input
#define INP_UP DIR_UP
#define INP_DOWN DIR_DOWN
#define INP_LEFT DIR_LEFT
#define INP_RIGHT DIR_RIGHT
#define INP_FIRE_UP 0x80
#define INP_FIRE_DOWN 0x40
#define INP_FIRE_LEFT 0x20
#define INP_FIRE_RIGHT 0x10

uint8_t sys_inp_dualsticks();


#endif // SYS_H
