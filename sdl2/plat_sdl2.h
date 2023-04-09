#ifndef PLAT_SDL2_H
#define PLAT_SDL2_H

// Details private to SDL2 backend.

#include <stdbool.h>

#define TARGET_FPS 60

bool audio_init();
void audio_close();
void audio_render();

void sfx_init();
void sfx_tick();

#endif // PLAT_SDL2_H
