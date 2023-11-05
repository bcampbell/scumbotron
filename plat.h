#ifndef SYS_H
#define SYS_H


// platform capabilities:
// PLAT_HAS_MOUSE
// PLAT_HAS_TEXTENTRY

#include "plat_details.h"    // platform-specific details (screen size etc...)

#define SCREEN_TEXT_W (SCREEN_W/8)
#define SCREEN_TEXT_H (SCREEN_H/8)

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

// text layer. This is persistent - if you draw text it'll stay onscreen until
// overwritten, or plat_clr() is called.
void plat_clr();
// draw len number of chars
void plat_textn(uint8_t cx, uint8_t cy, const char* txt, uint8_t len, uint8_t colour);
// draw null-terminated string
void plat_text(uint8_t cx, uint8_t cy, const char* txt, uint8_t colour);

// TODO: should just pass in level and score as bcd
void plat_hud(uint8_t level, uint8_t lives, uint32_t score);

// Render bonkers encoding where each byte encodes a 4x2 block of pixels,
// to be rendered via charset.
void plat_mono4x2(uint8_t cx, int8_t cy, const uint8_t* src, uint8_t cw, uint8_t ch, uint8_t basecol);

// Incremented every frame.
extern volatile uint8_t tick;

void plat_player_render(int16_t x, int16_t y, uint8_t facing, bool moving);
void plat_shot_render(int16_t x, int16_t y, uint8_t direction, uint8_t power);
void plat_powerup_render(int16_t x, int16_t y, uint8_t kind);
void plat_block_render(int16_t x, int16_t y);
void plat_grunt_render(int16_t x, int16_t y);
void plat_baiter_render(int16_t x, int16_t y);
void plat_tank_render(int16_t x, int16_t y, bool highlight);
void plat_amoeba_big_render(int16_t x, int16_t y);
void plat_amoeba_med_render(int16_t x, int16_t y);
void plat_amoeba_small_render(int16_t x, int16_t y);
void plat_hzapper_render(int16_t x, int16_t y, uint8_t state);
void plat_vzapper_render(int16_t x, int16_t y, uint8_t state);
void plat_fragger_render(int16_t x, int16_t y);
void plat_frag_render(int16_t x, int16_t y, uint8_t dir);
void plat_vulgon_render(int16_t x, int16_t y, bool highlight, uint8_t anger);
void plat_poomerang_render(int16_t x, int16_t y);
void plat_happyslapper_render(int16_t x, int16_t y, bool sleeping);
void plat_marine_render(int16_t x, int16_t y);
void plat_brain_render(int16_t x, int16_t y, bool highlight);
void plat_zombie_render(int16_t x, int16_t y);
void plat_rifashark_render(int16_t x, int16_t y, uint8_t dir8);
void plat_rifaspawner_render(int16_t x, int16_t y);
void plat_turret_render(int16_t x, int16_t y, uint8_t dir8, bool highlight);
void plat_missile_render(int16_t x, int16_t y, uint8_t dir8);
void plat_boss_render(int16_t x, int16_t y, bool highlight);
void plat_bossseg_render(int16_t x, int16_t y, uint8_t phase, bool atrest, bool highlight);

void plat_bub_render(int16_t x, int16_t y, uint8_t bubidx);
void plat_cursor_render(int16_t x, int16_t y);

#ifdef PLAT_HAS_MOUSE
extern int16_t plat_mouse_x;
extern int16_t plat_mouse_y;
extern uint8_t plat_mouse_buttons;   // left:0x01 right:0x02 middle:0x04
#endif // PLAT_HAS_MOUSE

// start PLAT_HAS_TEXTENTRY (fns should be no-ops if not supported)
extern void plat_textentry_start();
extern void plat_textentry_stop();
// Returns printable ascii or DEL (0x7f) or LF (0x0A), or 0 for no input.
extern char plat_textentry_getchar();
// end PLAT_HAS_TEXTENTRY
 
// noddy profiling
#define GATSO_OFF 0
#define GATSO_ON 1
void plat_gatso(uint8_t t);

// sfx
#define SFX_NONE 0
#define SFX_LASER 1
#define SFX_KABOOM 2
void plat_sfx_play(uint8_t effect);

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
#define INP_MENU_A 0x10     // primary/action button
#define INP_MENU_B 0x20     // secondary/cancel button
#define INP_MENU_START 0x40

// Returns direction + FIRE_ bits.
uint8_t plat_raw_dualstick();

// Returns direction + MENU_ bits.
uint8_t plat_raw_menukeys();

#define INP_CHEAT_POWERUP 0x01
#define INP_CHEAT_EXTRALIFE 0x02
#define INP_CHEAT_NEXTLEVEL 0x04

uint8_t plat_raw_cheatkeys();


// Draw a box in chars (effects layer)
void plat_drawbox(int8_t x, int8_t y, uint8_t w, uint8_t h, uint8_t ch, uint8_t colour);


#endif // SYS_H
