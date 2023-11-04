#ifndef SCUMBOTRON_EFFECTS_H
#define SCUMBOTRON_EFFECTS_H

#include <stdint.h>

void vfx_init();
void vfx_render();

void vfx_play_spawn(int16_t x, int16_t y);
void vfx_play_kaboom(int16_t x, int16_t y);
void vfx_play_zombify(int16_t x, int16_t y);
void vfx_play_warp();
void vfx_play_quicktext(int16_t x, int16_t y, const char* txt);

#endif // SCUMBOTRON_EFFECTS_H
