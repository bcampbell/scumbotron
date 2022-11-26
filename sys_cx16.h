#ifndef SYS_CX16_H
#define SYS_CX16_H

#include <stdint.h>
#include <stdbool.h>

extern void sys_init();
extern void waitvbl();
extern void inp_tick();

extern void sys_render_start();
extern void sprout(int16_t x, int16_t y, uint8_t img);
extern void sprout_highlight(int16_t x, int16_t y, uint8_t img);
extern void sys_spr32(int16_t x, int16_t y, uint8_t img);
extern void sys_render_finish();

extern void sys_clr();
extern void sys_text(uint8_t cx, uint8_t cy, const char* txt, uint8_t colour);
extern void sys_addeffect(int16_t x, int16_t y, uint8_t kind);

extern void sys_hud(uint8_t level, uint8_t lives, uint32_t score);
extern volatile uint16_t inp_joystate;
extern volatile uint8_t tick;

// effect kinds
#define EK_NONE 0
#define EK_SPAWN 1
#define EK_KABOOM EK_SPAWN

// core game stuff.
#define SCREEN_W 320
#define SCREEN_H 240

// fixed-point: 6 fractional bits.
#define FX 6
#define FX_ONE (1<<FX)


// sprite image defs
#define SPR16_BAITER 16
#define SPR16_AMOEBA_MED 20
#define SPR16_AMOEBA_SMALL 24
#define SPR16_TANK 28
#define SPR16_GRUNT 30
#define SPR16_SHOT 4

#define SPR32_AMOEBA_BIG 0

static inline void sys_tank_render(int16_t x, int16_t y, bool highlight) {
    if (highlight) {
        sprout_highlight(x, y,  SPR16_TANK + ((tick>>5) & 0x01));
    } else {
        sprout(x, y,  SPR16_TANK + ((tick >> 5) & 0x01));
    }
}

#endif // SYS_CX16_H
