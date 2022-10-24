#ifndef SYS_CX16_H
#define SYS_CX16_H


extern void sys_init();
extern void testum();
extern void waitvbl();
extern void inp_tick();

extern void sys_render_start();
extern void sproff();
extern void sprout(int16_t x, int16_t y, uint8_t img);
extern void sys_render_finish();

extern void sys_clr();
extern void sys_text(uint8_t cx, uint8_t cy, const char* txt, uint8_t colour);
extern void sys_addeffect(uint16_t x, uint16_t y, uint8_t kind);

extern volatile uint16_t inp_joystate;
extern volatile uint8_t tick;

// effect kinds
#define EK_NONE 0
#define EK_SPAWN 1

// core game stuff.
#define SCREEN_W 320
#define SCREEN_H 240

#endif // SYS_CX16_H
