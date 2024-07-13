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
#define SFX_MARINE_SAVED 7
#define SFX_ZAPPER_CHARGE 8
#define SFX_ZAPPING 9
#define SFX_INEFFECTIVE_THUD 10
#define SFX_HIT 11
#define SFX_BONUS 12
// TODO:
// collect bonus/marine
// level clear
// hit/thud (shoot invulerable dudes)
// hit/damage (shoot dudes which take multiple hits)
// zapper chargeup
// zapper fire
// spawner launch
// poomerang launch? (maybe)
// zombification

void sfx_init();
void sfx_tick(uint8_t frametime);
void sfx_play(uint8_t effect, uint8_t pri);

// A single effect to play continuously (or SFX_NONE).
// Must be set every frame.
extern uint8_t sfx_continuous;

#endif // SCUMBOTRON_SFX_H
