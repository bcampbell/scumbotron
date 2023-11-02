#ifndef SCUMBOTRON_EFFECTS_H
#define SCUMBOTRON_EFFECTS_H

#include <stdint.h>

// (visual) effect kinds
#define EK_NONE 0
#define EK_SPAWN 1
#define EK_KABOOM 2
#define EK_ZOMBIFY 3
#define EK_WARP 4
#define EK_TEXT 5

void effects_init();
void effects_add(int16_t x, int16_t y, uint8_t kind);
void effects_render();


void effects_start_text(int16_t x, int16_t y, const char* txt);

#endif // SCUMBOTRON_EFFECTS_H
