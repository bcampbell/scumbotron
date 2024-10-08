/*
 * SFX implementation using VERA PSG hardware.
 */

#include "sfx.h"
#include "misc.h"

// waveform types
#define PLAT_PSG_PULSE 0
#define PLAT_PSG_SAWTOOTH 1
#define PLAT_PSG_TRIANGLE 2
#define PLAT_PSG_NOISE 3

// Set all the params for a PSG channel in one go.
//
// vol: 0=off, 63=max
// freq: for frequency f, this should be f/(48828.125/(2^17))
// waveform: one of PLAT_PSG_*
extern void plat_psg(uint8_t chan, uint16_t freq, uint8_t vol, uint8_t waveform, uint8_t pulsewidth);


#define MAX_SFX (16-1)   // TODO: should be plat-specific
#define CONTINUOUS_CHAN MAX_SFX

static uint8_t sfx_effect[MAX_SFX];
static uint8_t sfx_timer[MAX_SFX];
static uint8_t sfx_pri[MAX_SFX];
static uint8_t sfx_next;
uint8_t sfx_continuous;

void sfx_init()
{
    uint8_t ch;
    sfx_next = 0;
    for (ch = 0; ch < MAX_SFX; ++ch) {
        sfx_effect[ch] = SFX_NONE;
        sfx_pri[ch] = 0;
    }
    sfx_continuous = SFX_NONE;
}

void sfx_play(uint8_t effect, uint8_t pri)
{
    uint8_t ch = MAX_SFX;
    for (uint8_t i = 0; i < MAX_SFX; ++i) {
        if (sfx_effect[i] == SFX_NONE) {
            ch = i;
            break;
        }
    }

    if (ch >= MAX_SFX) {
        // have to replace one...
        // start looking _after_ the most recent one.
        ch = sfx_next;
        for (uint8_t i = 0; i < MAX_SFX; ++i) {
            if (sfx_pri[ch] <= pri ) {
                break;  // that'll do.
            }
            ++ch;
            if (ch >= MAX_SFX) {
                ch -= MAX_SFX;
            }
        }
    }

    if (ch < MAX_SFX) {
        sfx_effect[ch] = effect;
        sfx_timer[ch] = 0;
        sfx_pri[ch] = pri;
        sfx_next = ch + 1;
        if (sfx_next > MAX_SFX) {
            sfx_next -= MAX_SFX;
        }
    }
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
    case SFX_MARINE_SAVED:
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
    case SFX_INEFFECTIVE_THUD:
        {
            if( (t&0x03) == 0) {
                plat_psg(ch, 800+t*16/*+s*/, 64-t, PLAT_PSG_NOISE, 0);
            } else {
                plat_psg(ch, 100+t*32, 64-t, PLAT_PSG_PULSE, (t/8)&0x3f);
            }

            if (t>=6) {
                return true;
            }
        }
        break;
    case SFX_HIT:
        {
            if( (t&0x01) == 0) {
                plat_psg(ch, 5600-t*256, 48, PLAT_PSG_NOISE, 0);
            } else {
                plat_psg(ch, 600+t*64, 63, PLAT_PSG_TRIANGLE, 0);
            }

            if (t>=6) {
                return true;
            }
        }
        break;
    case SFX_BONUS:
        {
            uint16_t f = 400 + (t& 0x7)*256 + t*64;
            plat_psg(ch, f,  63, PLAT_PSG_SAWTOOTH,0);
            if (t>=32) {
                return true;
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

void sfx_tick(uint8_t frametick)
{
    uint8_t ch;
    for (ch=0; ch < MAX_SFX; ++ch) {
        uint8_t t = sfx_timer[ch]++;
        bool fin = gen(ch, sfx_effect[ch], t);
        if (fin) {
            sfx_effect[ch] = SFX_NONE;
        }
    }

    // Update single continuous effect (driven by the frame tick)
    gen(CONTINUOUS_CHAN, sfx_continuous, frametick);
    sfx_continuous = SFX_NONE;  // game has to set this every frame to keep it running.
}


void sfx_render_dbug()
{
}

