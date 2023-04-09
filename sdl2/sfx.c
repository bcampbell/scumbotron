#include "../plat.h"
#include "vera_psg.h"

#include <SDL.h>


#define MAX_SFX 4
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

void plat_sfx_play(uint8_t effect)
{
    uint8_t ch = sfx_next++;
    if (sfx_next >= MAX_SFX) {
        sfx_next = 0;
    }

    sfx_effect[ch] = effect;
    sfx_timer[ch] = 0;
}


static inline void psg(uint8_t ch, uint16_t freq, uint8_t vol, uint8_t waveform, uint8_t pulsewidth)
{
    uint8_t reg = ch*4;
    psg_writereg(reg, freq & 0xff);
    psg_writereg(reg+1, freq >> 8);
    psg_writereg(reg+2, (3<<6) | vol);  // lrvvvvvv
    psg_writereg(reg+3, (waveform << 6) | pulsewidth);  // wwpppppp
}

static inline void psgoff(uint8_t ch)
{
    uint8_t reg = ch*4;
    psg_writereg(reg, 0);
    psg_writereg(reg+1, 0);
    psg_writereg(reg+2, 0);
    psg_writereg(reg+3, 0);
}


void sfx_tick()
{
    uint8_t ch;
    for (ch=0; ch < MAX_SFX; ++ch) {
        uint8_t t = sfx_timer[ch]++;
        switch (sfx_effect[ch]) {
        case SFX_LASER:
            if (t<64) {
                psg(ch, 2000 - (t<<6), (63-t)/4, 0, t);
            } else {
                psgoff(ch);
                sfx_effect[ch] = SFX_NONE;
            }
            break;
        case SFX_KABOOM:
            if (t < 64) {
                psg(ch, 300, 63-t, t&3, 0);
            } else {
                psgoff(ch);
                sfx_effect[ch] = SFX_NONE;
            }
            break;
        default:
            //psgoff(ch);
            break;
        }
    }
}

