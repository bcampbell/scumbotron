#ifndef SCUMBOTRON_VFX_H
#define SCUMBOTRON_VFX_H

#include <stdint.h>

void vfx_init();
void vfx_tick();
void vfx_render();

void vfx_play_spawn(int16_t x, int16_t y);
void vfx_play_kaboom(int16_t x, int16_t y);
void vfx_play_zombify(int16_t x, int16_t y);
void vfx_play_warp();
// string ptrs must last duration of effect! no local buffers.
void vfx_play_quicktext(int16_t x, int16_t y, const char* txt);
void vfx_play_alerttext(const char* txt);

#endif // SCUMBOTRON_VFX_H
