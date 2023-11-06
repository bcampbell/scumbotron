#ifndef SCUMBOTRON_SFX_H
#define SCUMBOTRON_SFX_H

#include <stdint.h>

#define SFX_NONE 0
#define SFX_LASER 1
#define SFX_KABOOM 2

void sfx_init();
void sfx_tick();
void sfx_play(uint8_t effect);

#endif // SCUMBOTRON_SFX_H
