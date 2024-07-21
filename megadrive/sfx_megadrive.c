#include <stdint.h>
#include <stdbool.h>
//#include "../plat.h"
#include "../sfx.h"
#include "../misc.h"

#define PSG_PORT ((volatile uint8_t *) 0xC00011)


// channels 0-2 are square wave
// channel 3 is noise.
static uint8_t chan_effect[4];
static uint8_t chan_time[4];
static uint8_t chan_pri[4];

static uint8_t chan_pick(uint8_t effect, uint8_t pri)
{
    // TODO: specialcase for noise
    uint8_t ch;
    for (ch = 0; ch < 3; ++ch) {
        if (chan_effect[ch] == SFX_NONE) {
            return ch;
        }
    }
    for (ch = 0; ch < 3; ++ch) {
        if (pri >= chan_pri[ch]) {
            return ch;
        }
    }
    return 0xff;
}

uint8_t sfx_continuous = SFX_NONE;


// Attenuation, 4bits. 0x00 = full, 0x0f=silent
static psg_attenuation(uint8_t ch, uint8_t att) {
    *PSG_PORT = 0x90 | (ch << 5) | att;
}

// Frequency registers are 10 bits in size, accepting values from 0x1
// (111860hz) to 0x3FF (109hz)
static psg_freq(uint8_t ch, uint16_t freq) {
    *PSG_PORT = 0x80 | (ch << 5) | (freq & 0x0f);
    *PSG_PORT = 0x00 | (freq >> 4);
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

static bool effect(uint8_t ch, uint8_t effect, uint8_t t)
{
    switch(effect) {
    case SFX_LASER:
        psg_freq(ch, 0x180 + (t<<6) - (rnd() & 0x0f));
        psg_attenuation(ch, 0x04);
        return (t >= 8);
    case SFX_KABOOM:
        psg_freq(ch, 0x300);
        psg_attenuation(ch, t<<2);
        return (t >= 32);
    case SFX_WARP:
        int8_t s = sin24((t<<1) & 0x0f);
        psg_freq(ch, 0x300 - (t>>2) - (s>>1));
        psg_attenuation(ch, 0);//t & 0x03);
        return (t >= 128);
    case SFX_OWWWW:
        psg_freq(ch, 0x100 - (t << 2));
        psg_attenuation(ch, u8clip(t >> 2, 0, 15));
        return (t >= 64);
    case SFX_LOOKOUT:
        psg_freq(ch, 0x030 + ((t & 0x0f)<<6));
        psg_attenuation(ch, 0);
        return;
    case SFX_HURRYUP:
        psg_freq(ch, 0x300 - (t<<4) - ((t & 0x0f)<<6));
        psg_attenuation(ch, 0);
        return;
    default:
        psg_freq(ch, 0);
        psg_attenuation(ch, 0x0f);
        return true;
    }
}


void sfx_init()
{
    for (uint8_t i = 0; i < 3; ++i) {
        psg_attenuation(i, 0x0f);
        psg_freq(i,0);
        //*PSG_PORT = 0x90 | (i << 5) | 0x0F;
    }
}

void sfx_tick(uint8_t frametick)
{
    for (uint8_t ch = 0; ch < 4; ++ch) {
        uint8_t t = chan_time[ch];
        ++chan_time[ch];

        bool fin = effect(ch,chan_effect[ch], t);
        if (fin) {
            // Off.
            chan_effect[ch] = SFX_NONE;
            chan_time[ch] = 0;
            chan_pri[ch] = 0;
            psg_attenuation(ch, 0x0f);
            psg_freq(ch, 0);
            continue;
        }
    }
}


void sfx_play(uint8_t effect, uint8_t pri)
{
    uint8_t ch = chan_pick(effect, pri);
    if (ch == 0xff) {
        return;
    }
    chan_effect[ch] = effect;
    chan_pri[ch] = pri;
    chan_time[ch] = 0;
}


