#ifndef SCUMBOTRON_SFX_H
#define SCUMBOTRON_SFX_H

#include <stdint.h>

#define SFX_NONE 0
#define SFX_LASER 1
#define SFX_KABOOM 2
#define SFX_WARP 3
#define SFX_OWWWW 4
#define SFX_HURRYUP 5
#define SFX_LOOKOUT 6
#define SFX_BONUS 7
#define SFX_ZAPPER_CHARGE 8
#define SFX_ZAPPING 9

void sfx_init();
void sfx_tick();
void sfx_play(uint8_t effect);


// A single effect to play continuously (or SFX_NONE).
// Must be set every frame.
extern uint8_t sfx_continuous;

#endif // SCUMBOTRON_SFX_H
