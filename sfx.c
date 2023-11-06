/*
 * SFX
 */

#include "sfx.h"
#include "plat.h"


#define MAX_SFX 4   // TODO: should be plat-specific

static uint8_t sfx_effect[MAX_SFX];
static uint8_t sfx_timer[MAX_SFX];
static uint8_t sfx_next;

void sfx_init()
{
    uint8_t ch;
    sfx_next = 0;
    for (ch = 0; ch < MAX_SFX; ++ch) {
        sfx_effect[ch] = SFX_NONE;
    }
}

void sfx_play(uint8_t effect)
{
    uint8_t ch = sfx_next++;
    if (sfx_next >= MAX_SFX) {
        sfx_next = 0;
    }

    sfx_effect[ch] = effect;
    sfx_timer[ch] = 0;
}


void sfx_tick()
{
    uint8_t ch;
    for (ch=0; ch < MAX_SFX; ++ch) {
        uint8_t t = sfx_timer[ch]++;
        switch (sfx_effect[ch]) {
        case SFX_LASER:
            plat_psg(ch, 2000 - (t << 6), (63 - t) / 4, 0, t);
            if (t>=63) {
                sfx_effect[ch] = SFX_NONE;
            }
            break;
        case SFX_KABOOM:
            plat_psg(ch, 300, 63 - t, t & 3, 0);
            if (t>=63) {
                sfx_effect[ch] = SFX_NONE;
            }
            break;
        default:
            plat_psg(ch, 0, 0, 0, 0);  //off
            break;
        }
    }
}

