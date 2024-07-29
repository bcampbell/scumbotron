#include <stdint.h>
#include <stdbool.h>
#include "../plat.h"
#include "../sfx.h"
#include "../misc.h"

#define PSG_PORT ((volatile uint8_t *) 0xC00011)

// channels 0-2 are square wave
// channel 3 is noise.
static uint8_t chan_effect[4];
static uint8_t chan_time[4];
static uint8_t chan_pri[4];
static uint8_t chan_prev;
uint8_t sfx_continuous = SFX_NONE;
static uint8_t prev_sfx_continuous = SFX_NONE;

static uint8_t chan_pick(uint8_t pri)
{
    // free channel?
    for (uint8_t ch = 0; ch < 3; ++ch) {
        if (chan_effect[ch] == SFX_NONE) {
            chan_prev = ch;
            return ch;
        }
    }

    // Look for one we can replace...
    // (start after most recently started one so we cycle through fairly)
    uint8_t ch = chan_prev;
    for (uint8_t i=0; i<3; ++i) {
        ++ch;
        if (ch >= 3) {
            ch = 0;
        }
        if (pri >= chan_pri[ch]) {
            chan_prev = ch;
            return ch;
        }
    }
    return 0xff;
}

// Attenuation, 4bits. 0x00 = full, 0x0f=silent
static void psg_attenuation(uint8_t ch, uint8_t att) {
    *PSG_PORT = 0x90 | (ch << 5) | att;
}

// Frequency registers are 10 bits in size, accepting values from 0x1
// (111860hz) to 0x3FF (109hz)
static void psg_freq(uint8_t ch, uint16_t freq) {
    *PSG_PORT = 0x80 | (ch << 5) | (freq & 0x0f);
    *PSG_PORT = 0x00 | (freq >> 4);
}


// clock: 0,1,2
// periodic: 0 or 1
static void psg_noise(uint8_t clock, uint8_t periodic) {
    *PSG_PORT = 0xE0 | periodic << 2 | clock;
}

static void chan_stop(uint8_t ch) {
    chan_effect[ch] = SFX_NONE;
    chan_time[ch] = 0;
    chan_pri[ch] = 0;
    psg_attenuation(ch, 0x0f);
    if (ch!=3) {
        psg_freq(ch, 0);
    }
}

static uint8_t u8clip(uint8_t v, uint8_t vmin, uint8_t vmax)
{
    if (v < vmin) {
        return vmin;
    } else if (v > vmax) {
        return vmax;
    } else {
        return v;
    }
}

static bool chan_update(uint8_t ch, uint8_t effect, uint8_t t)
{
    switch(effect) {
    case SFX_LASER:
        psg_freq(ch, 0x180 + (t<<6) - (rnd() & 0x0f));
        psg_attenuation(ch, 0x04);
        return (t >= 8);
    case SFX_KABOOM:
        {
            psg_freq(ch, 0x130 + (t<<4) + ((t&3)<<4) + ((t & 0x07)<<5));
            uint8_t att = 0;
            if (t >16 ) {
                att = u8clip(((t-32)>>1), 0, 15);
            }
            psg_attenuation(ch, att);
        }
        return (t >= 32);
    case SFX_WARP:
        {
            int8_t s = sin24((t<<1) & 0x0f);
            psg_freq(ch, 0x300 - (t<<1) - (s<<2));
            uint8_t att = 0;
            if (t > 8) {
                att = u8clip((t - 8) >> 3,0,15);
            }
            psg_attenuation(ch, att);
        }
        return (t >= 64);
    case SFX_OWWWW:
        {
            int8_t s = sin24((t<<3) & 0x0f);
            if (ch == 3) {
                // noise part
                if (t == 0) {
                    psg_noise(2,1);
                }
                psg_attenuation(3, (128+s)>>5);
            } else {
                // psg part
                psg_freq(ch, 0x200 + (t<<3) + (rnd()&0x7f));
                psg_attenuation(ch, (128+s)>>6);
            }
            return (t >= 48);
        }
        return false;
    case SFX_HURRYUP:
        psg_freq(ch, 0x030 + ((t & 0x0f)<<6));
        psg_attenuation(ch, 0);
        return (t>=64);
    case SFX_LOOKOUT:
        {
            int8_t s = sin24(t & 0x0f);
            psg_freq(ch, (0x220-s) -(t<<3));
            psg_attenuation(ch, 0);
        }
        return (t >= 48);
    case SFX_MARINE_SAVED:
        {
            psg_freq(ch, 0x220 - ((t&0x7)<<5) - (t<<3));
            psg_attenuation(ch, 0);
        }
        return (t >= 48);
    case SFX_ZAPPER_CHARGE:
        {
            int8_t s = sin24((t<<2) & 0x0f);
            psg_freq(ch, 0x3ff - (t<<2) - (rnd() & 0x1f));
            psg_attenuation(ch, (128+s)>>5);
        }
        return false;
    case SFX_ZAPPING:
        {
            psg_freq(ch, 0x3ff - ((t&0x03)<<7));// + (s<<4));
            if ((t & 0x7) == 0) {
                psg_attenuation(ch, 0x04);//s2 & 0x0f);
            } else {
                psg_attenuation(ch, 0x00);//s2 & 0x0f);
            }
        }
        return false;   // forever!
    case SFX_INEFFECTIVE_THUD:
        if (ch != 3) {
            return true;    // free redundantly-allocated channel
        }
        if (t == 0) {
            psg_noise(1,1);
        } else if (t >= 6) {
             return true;
        }
        psg_attenuation(3, t<<1);
        return false;
    case SFX_HIT:
        if (t&0x2) {
            psg_freq(ch, 0x200-(t<<5));
        } else {
            psg_freq(ch, 0x300+(t<<3));
        }

        psg_attenuation(ch, 0);
        return (t>=8);
    case SFX_BONUS:
        {
    //        if (t&1) {
//            } else {
                int8_t s = sin24((t<<2) & 0x0f);
                psg_freq(ch, 0x220 - (t<<4) - (s));
  //              psg_freq(ch, 0x210 - s);
      //      }
            uint8_t att = 0;
            if (t >16 ) {
                att = u8clip(((t-16)>>1), 0, 15);
            }
            psg_attenuation(ch, att);
        }
        return (t >= 48);
    default:
        return true;
    }
}


void sfx_init()
{
    chan_prev = 0;
    for (uint8_t ch = 0; ch < 4; ++ch) {
        chan_effect[ch] = SFX_NONE;
        chan_time[ch] = 0;
        chan_pri[ch] = 0;
        psg_attenuation(ch, 0x0f);
        if (ch != 3) {
            psg_freq(ch, 0);
        }
    }
}

void sfx_tick(uint8_t frametick)
{
    // tick all channels (including noise)
    for (uint8_t ch = 0; ch < 4; ++ch) {
        uint8_t t = chan_time[ch];
        ++chan_time[ch];

        bool fin = chan_update(ch,chan_effect[ch], t);
        if (fin) {
            // Off.
            chan_stop(ch);
        }
    }

    if (sfx_continuous != prev_sfx_continuous) {
        // stop playing old sound
        if (prev_sfx_continuous != SFX_NONE) {
            for (uint8_t ch = 0; ch < 4; ++ch) {
                if (chan_effect[ch] == prev_sfx_continuous) {
                    chan_stop(ch);
                }
            }
        }

        // start playing new sound
        if (sfx_continuous != SFX_NONE) {
            // use highest pri
            sfx_play(sfx_continuous, 255);
        }
    }
    prev_sfx_continuous = sfx_continuous;
    sfx_continuous = SFX_NONE;

}


// returns true if effect needs a psg channel (0-2)
static bool uses_psg(uint8_t effect) {
    return effect != SFX_INEFFECTIVE_THUD;
}

// return true if effect uses noise channel (3)
static bool uses_noise(uint8_t effect) {
    return effect == SFX_OWWWW || effect == SFX_INEFFECTIVE_THUD;
}

void sfx_play(uint8_t effect, uint8_t pri)
{
    if (uses_psg(effect)) {
        uint8_t ch = chan_pick(pri);
        if (ch != 0xff) {
            chan_effect[ch] = effect;
            chan_pri[ch] = pri;
            chan_time[ch] = 0;
        }
    }

    if (uses_noise(effect)) {
        if (chan_effect[3] == SFX_NONE || pri >= chan_pri[3]) {
            // play on noise channel too
            chan_effect[3] = effect;
            chan_pri[3] = pri;
            chan_time[3] = 0;
        }
    }
}

void sfx_render_dbug()
{
    uint8_t basex = 20;
    uint8_t basey = 1;

    for( uint8_t ch = 0; ch < 4; ++ch) {
        uint8_t effect = chan_effect[ch];
        uint8_t t = chan_time[ch];
        uint8_t pri = chan_pri[ch];

        char buf[16] = {
            hexdigits[ch&0x0f],
            ':',
            hexdigits[(effect >> 4) & 0x0f],
            hexdigits[effect & 0x0f],
            ' ',
            hexdigits[(t >> 4) & 0x0f],
            hexdigits[t & 0x0f],
            ' ',
            hexdigits[(pri >> 4) & 0x0f],
            hexdigits[pri & 0x0f]
        };

        uint8_t c = (effect == SFX_NONE) ? 2 : 1;
        plat_textn(basex, basey + ch, buf, 10, c);
    }

    // continuous sound
    {
        char buf[2] = {
            hexdigits[sfx_continuous>>4],
            hexdigits[sfx_continuous&0x0f],
        };
        plat_textn(basex, basey+4, buf,2,1);
    }
    {
        char buf[2] = {
            hexdigits[prev_sfx_continuous>>4],
            hexdigits[prev_sfx_continuous&0x0f],
        };
        plat_textn(basex+3, basey+4, buf,2,1);
    }
}

