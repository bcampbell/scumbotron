/*
 * SFX
 */

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

#include "sfx.h"
#include "plat.h"
#include "misc.h"

#define MAX_SFX (8-1)   // TODO: should be plat-specific
#define CONTINUOUS_CHAN MAX_SFX

static uint8_t sfx_effect[MAX_SFX];
static uint8_t sfx_timer[MAX_SFX];
static uint8_t sfx_next;
uint8_t sfx_continuous;

void sfx_init()
{
    uint8_t ch;
    sfx_next = 0;
    for (ch = 0; ch < MAX_SFX; ++ch) {
        sfx_effect[ch] = SFX_NONE;
    }
    sfx_continuous = SFX_NONE;
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

static bool gen(uint8_t ch, uint8_t effect, uint8_t t)
{
    switch (effect) {
    case SFX_LASER:
        plat_psg(ch, 2000 - (t << 6), (63 - t) / 4, 0, t);
        if (t>=63) {
            return true;
        }
        break;
    case SFX_KABOOM:
        plat_psg(ch, 300, 63 - t, t & 3, 0);
        if (t>=63) {
            return true;
        }
        break;
    case SFX_WARP:
        {
            int8_t s = sin24((t*2) & 0x0f);
            //if((t&0x03)==0) {
           //     plat_psg(ch, 8000+t*8+s*32, 63-(t/4), PLAT_PSG_TRIANGLE, 0);
            //} else {
            uint8_t vol = (t < 64) ? 63 : 64-((t-64));
                plat_psg(ch, 300+(t/2)+(s/4), vol, PLAT_PSG_PULSE, 31+ (s/4)/*(t/8) & 0x3f*/);
            //}
            if (t>=128) {
                return true;
            }
        }
        break;
    case SFX_OWWWW:
        {
            if( t&0x01) {
                plat_psg(ch, 2800-t*8/*+s*/, 63, PLAT_PSG_NOISE, 0);
            } else {
                plat_psg(ch, (800-t) -(t&0xf)*16, 63, PLAT_PSG_PULSE, (t)&0x3f);
            }

            if (t>=64) {
                return true;
            }
        }
        break;
    case SFX_LOOKOUT:
        {
            plat_psg(ch, 5400+(t&0xf)*64, 63, PLAT_PSG_PULSE, (t)&0x3f);
            if (t>=64) {
                return true;
            }
        }
        break;
    case SFX_HURRYUP:
        {
            plat_psg(ch, 200 + t*16 + (t&0xf)*64, 63, PLAT_PSG_TRIANGLE,0);
            if (t>=64) {
                return true;
            }
        }
        break;
    case SFX_BONUS:
        {
            plat_psg(ch, 800+t*64 + ((t*2)&0x7)*256,  63, PLAT_PSG_TRIANGLE,0);
            if (t>=12) {
                return true;
            }
        }
        break;
    case SFX_ZAPPING:
        {
            // Plays forever
            int8_t s = sin24((t*8) & 0x0f);
            plat_psg(ch, 200 + s, 63, PLAT_PSG_PULSE, t & 0x3f);
        }
        break;
    case SFX_ZAPPER_CHARGE:
        {
            if ((t & 0x03) > 1) {
                plat_psg(ch, 2000+t*16, 63, PLAT_PSG_TRIANGLE, 0);
            } else {
                plat_psg(ch, 0,0,0,0);
            }
        }
        break;

#if 0 /* small splosion */
        {
            if( t&0x01) {
                int8_t s = sin24((t*4)&0x0f);
        
                plat_psg(ch, 800+t*16/*+s*/, 64-t, PLAT_PSG_NOISE, 0);
            } else {
                plat_psg(ch, 800-t*16, 64-t, PLAT_PSG_PULSE, (t/8)&0x3f);
            }

            if (t>=32) {
                return true;
            }
        }
        break;
    #endif
#if 0
        /* computery burbling*/
        plat_psg(ch, 1000+rnd()*16, 63, PLAT_PSG_TRIANGLE, 0);
            if (t>=64) {
                return true;
            }
#endif
#if 0
        {
            // good spawn sound?
            int8_t s = sin24((t*2) & 0x0f);
            if((t&0x01)==0) {
                plat_psg(ch, 8000+t*8+s*32, 63-(t/4), PLAT_PSG_TRIANGLE, 0);
            } else {
            uint8_t vol = 64-t;
                plat_psg(ch, 200+t*4, vol, PLAT_PSG_PULSE, 31+ (s/4)/*(t/8) & 0x3f*/);
            }
            if (t>=64) {
                return true;
            }
        }
#endif

#if 0
        /* downward ooof */
        plat_psg(ch, 200-t, 63, PLAT_PSG_PULSE, (t)&0x03f);
        if (t>=128) {
            return true;
        }
#endif
        /* promising chime. */
#if 0
        if (t&1) {
        plat_psg(ch, 6000 - ((t*512)&0x0fff ), 63-t/2, 0, (t) & 63);
        } else {
        plat_psg(ch, 130, 63, 2, (t) & 63);
        }
        if (t>=63) {
            return true;
        }
#endif
        break;
    default:
        plat_psg(ch, 0, 0, 0, 0);  //off
        break;
    }
    return false;
}

void sfx_tick()
{
    uint8_t ch;
    for (ch=0; ch < MAX_SFX; ++ch) {
        uint8_t t = sfx_timer[ch]++;
        bool fin = gen(ch, sfx_effect[ch], t);
        if (fin) {
            sfx_effect[ch] = SFX_NONE;
        }
    }

    // Update single continuous effect (driven by the global tick)
    gen(CONTINUOUS_CHAN, sfx_continuous, tick);
    sfx_continuous = SFX_NONE;  // game has to set this every frame to keep it running.
}



