#ifndef PLAT_H
#define PLAT_H

/*
 * Common interface for all platforms
 */

// platform capabilities:
// PLAT_HAS_MOUSE
// PLAT_HAS_TEXTENTRY

#include <stdint.h>
#include <stdbool.h>
#include "plat_details.h"    // platform-specific details (screen size etc...)

#define SCREEN_TEXT_W (SCREEN_W/8)
#define SCREEN_TEXT_H (SCREEN_H/8)

// provided by game.c:
void game_init();
void game_tick();
void game_render();


// Incremented every frame.
extern volatile uint8_t tick;

// fixed-point: 6 fractional bits.
#define FX 6
#define FX_ONE (1<<FX)

// Request immediate exit, if supported by platform. A likely no-op :-)
void plat_quit();

// Text layer. This is persistent - if you draw text it'll stay onscreen until
// overwritten, or plat_clr() is called.
// TODO: work out the details here, and how it interacts with effects.
void plat_clr();

// Draw len number of chars
void plat_textn(uint8_t cx, uint8_t cy, const char* txt, uint8_t len, uint8_t colour);
// Draw null-terminated string.
// Provided by draw_common.c
void plat_text(uint8_t cx, uint8_t cy, const char* txt, uint8_t colour);

// TODO: should just pass in level and score as bcd
void plat_hud(uint8_t level, uint8_t lives, uint32_t score);

// Render bonkers encoding where each byte encodes a 4x2 block of pixels,
// to be rendered via charset.
void plat_mono4x2(uint8_t cx, int8_t cy, const uint8_t* src, uint8_t cw, uint8_t ch, uint8_t basecol);

// character indexes used by drawing routines
#define DRAWCHR_2x2 0    // 16 2x2 pixels
#define DRAWCHR_BLOCK 127    // main char used for effects
#define DRAWCHR_VLINE 16     // 8 vline chars
#define DRAWCHR_HLINE 24     // 8 hline chars

// Draw horizontal line of chars, range [cx_begin, cx_end).
void plat_hline_noclip(uint8_t cx_begin, uint8_t cx_end, uint8_t cy, uint8_t chr, uint8_t colour);

// Draw vertical line of chars, range [cy_begin, cy_end).
void plat_vline_noclip(uint8_t cx, uint8_t cy_begin, uint8_t cy_end, uint8_t chr, uint8_t colour);

// Draw a box in chars (effects layer)
// Provided by draw_common.c
void plat_drawbox(int8_t x, int8_t y, uint8_t w, uint8_t h, uint8_t ch, uint8_t colour);


// sprite image defs
#define SPR16_RETICULE 0
#define SPR16_CURSOR 1  // 2 frames
#define SPR16_BLOCK 3
#define SPR16_EXTRALIFE 8
#define SPR16_POWERUP 9
#define SPR16_BAITER 16
#define SPR16_AMOEBA_MED 20
#define SPR16_AMOEBA_SMALL 24
#define SPR16_TANK 28
#define SPR16_GRUNT 30
#define SPR16_HZAPPER 12
#define SPR16_HZAPPER_ON 13
#define SPR16_VZAPPER 14
#define SPR16_VZAPPER_ON 15
#define SPR16_FRAGGER 32
#define SPR16_FRAG_NW 33
#define SPR16_FRAG_NE 34
#define SPR16_FRAG_SW 35
#define SPR16_FRAG_SE 36
#define SPR16_VULGON 37
#define SPR16_VULGON_ANGRY 39
#define SPR16_POOMERANG 41
#define SPR16_HAPPYSLAPPER 45
#define SPR16_HAPPYSLAPPER_SLEEP 47
#define SPR16_PLR_U 48
#define SPR16_PLR_D 50
#define SPR16_PLR_L 52
#define SPR16_PLR_R 54
#define SPR16_MARINE 56
#define SPR16_BOSS_SEG 58
#define SPR16_SHOT 64
#define SPR16_SHOT_MED 68
#define SPR16_SHOT_HEAVY 72
#define SPR16_BRAIN 76
#define SPR16_ZOMBIE 78
#define SPR16_GOBBER 80
#define SPR16_MISSILE 88
#define SPR16_RIFASHARK 96
#define SPR16_RIFASPAWNER 92+16

#define SPR32_AMOEBA_BIG 0
#define SPR32_BOSS_HEAD 2
#define SPR32_BLOB 3

#define SPR64x8_BUB 0   // Start of speech bubble sprites.

void sprout16(int16_t x, int16_t y, uint8_t img);
void sprout16_highlight(int16_t x, int16_t y, uint8_t img);
void sprout32(int16_t x, int16_t y, uint8_t img);
void sprout32_highlight(int16_t x, int16_t y, uint8_t img);
void sprout64x8(int16_t x, int16_t y, uint8_t img );



// these are all provided by draw_common.c
void plat_player_render(int16_t x, int16_t y, uint8_t facing, bool moving);
void plat_shot_render(int16_t x, int16_t y, uint8_t direction, uint8_t power);
void plat_pickup_render(int16_t x, int16_t y, uint8_t kind);
void plat_block_render(int16_t x, int16_t y);
void plat_grunt_render(int16_t x, int16_t y);
void plat_baiter_render(int16_t x, int16_t y);
void plat_tank_render(int16_t x, int16_t y, bool highlight);
void plat_amoeba_big_render(int16_t x, int16_t y);
void plat_amoeba_med_render(int16_t x, int16_t y);
void plat_amoeba_small_render(int16_t x, int16_t y);
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
void plat_gobber_render(int16_t x, int16_t y, uint8_t dir8, bool highlight);
void plat_missile_render(int16_t x, int16_t y, uint8_t dir8);
void plat_boss_render(int16_t x, int16_t y, bool highlight);
void plat_bossseg_render(int16_t x, int16_t y, uint8_t phase, bool atrest, bool highlight);

void plat_bub_render(int16_t x, int16_t y, uint8_t bubidx);
void plat_cursor_render(int16_t x, int16_t y);

// these two are platform-specific
void plat_hzapper_render(int16_t x, int16_t y, uint8_t state);
void plat_vzapper_render(int16_t x, int16_t y, uint8_t state);

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

// waveform types
#define PLAT_PSG_PULSE 0
#define PLAT_PSG_SAWTOOTH 1
#define PLAT_PSG_TRIANGLE 2
#define PLAT_PSG_NOISE 3

// Set all the params for a PSG channel in one go.
//
// vol: 0=off, 63=max
// freq: for frequency f, this should be f/(48828.125/(2^17))
// waveform: one of PLAT_PSG_*
void plat_psg(uint8_t chan, uint16_t freq, uint8_t vol, uint8_t waveform, uint8_t pulsewidth);

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

#define INP_PAD_A 0x10     // primary/action button
#define INP_PAD_B 0x20     // secondary/cancel button
#define INP_PAD_START 0x40
//#define INP_PAD_SELECT 0x80

#define INP_KEY_ENTER 0x10   // Enter key
#define INP_KEY_ESC 0x80     // Escape key
                             //
// Returns direction + FIRE_ bits.
uint8_t plat_raw_dualstick();

// Returns direction + PAD_ bits.
uint8_t plat_raw_gamepad();

// Returns direction + KEY_ bits.
uint8_t plat_raw_keys();


#define INP_CHEAT_POWERUP 0x01
#define INP_CHEAT_EXTRALIFE 0x02
#define INP_CHEAT_NEXTLEVEL 0x04

uint8_t plat_raw_cheatkeys();



bool plat_savescores(const void* begin, int nbytes);
bool plat_loadscores(void* begin, int nbytes);

#endif // PLAT_H
