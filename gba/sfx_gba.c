#include <gba_base.h>
#include <gba_sound.h>

#include "../sfx.h"


static uint8_t chan[4];
static uint8_t chantime[4];

// duty 0-3
// freq 0-2047  (131072/(2048-n)Hz) 11 bits
// env 0-7
// len 0-63  (64-n) * (1/256)s   6 bits
static void startchan(int ch, uint8_t len, uint8_t duty, uint16_t freq)
{
    switch (ch) {
        case 0:
            REG_SOUNDCNT_L |= 0x1100;   // on
            REG_SOUND1CNT_L = 8;    // no sweep
            REG_SOUND1CNT_H = 0xF000 | (duty<<6) | len;
            REG_SOUND1CNT_X = 0x8000 | freq;
            break;
    }
}

void sfx_init()
{
    REG_SOUNDCNT_X = 0x80;
    REG_SOUNDCNT_L=0x0077;      // left and right full volume
    REG_SOUNDCNT_H = 0x0002;    // 100% volume for 1-4.

    for (uint8_t i = 0; i < 4; ++i) {
        chan[i] = SFX_NONE;
        chantime[i] = 0;
    }
}

void sfx_tick(uint8_t frametick)
{
    ++chantime[0];
    if(chantime[0] > 16) {
        REG_SOUNDCNT_L &= ~0x1100;  // off
        chan[0] = SFX_NONE;
    }

    if(chan[0] == SFX_NONE) {
        return;
    }
    REG_SOUND1CNT_X = (frametick&1) ? (0xf00+ chantime[0]<<4) : (0xf00 - (chantime[0]<<2));

    uint8_t duty = 3;//(chantime[0] & 1) ? 0:3;
    REG_SOUND1CNT_H = 0xF000 | (duty<<6) | 0;
}

void sfx_play(uint8_t effect, uint8_t pri)
{
    chantime[0] = 0;
    chan[0] = effect;
    startchan(0, 63, 2, 0x400);
}
/*
    //turn on sound circuit
    REG_SOUNDCNT_L =0x1177;
    //REG_SOUNDCNT_H = 2;
    REG_SOUND1CNT_L=0x0056; //sweep shifts=6, increment, sweep time=39.1ms
    REG_SOUND1CNT_H=0xf780; //duty=50%,envelope decrement
    REG_SOUND1CNT_X=0x8400; //frequency=0x0400, loop mode
}
*/


uint8_t sfx_continuous = SFX_NONE;

