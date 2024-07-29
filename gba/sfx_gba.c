#include <gba_base.h>
#include <gba_sound.h>

#include "../sfx.h"
#include "../misc.h"


// channels 0-2 are square wave
// channel 3 is noise.
static uint8_t chan_effect[4];
static uint8_t chan_time[4];
static uint8_t chan_pri[4];
static uint8_t chan_prev = 0;
uint8_t sfx_continuous = SFX_NONE;
static uint8_t prev_sfx_continuous = SFX_NONE;
static uint8_t chan3_duty;   // currently set duty channel3

static void chan_on(uint8_t ch);
static void chan_off(uint8_t ch);
static void chan_freq(uint8_t ch, uint16_t freq);

// waveforms for channel3 - square waves of varying duty cycle.
// 2 cycles, so sample rate matches frequency value in channels 0 and 1.
const uint16_t chan3_waves[4*8] = {
    0xFF00,0x0000,0x0000,0x0000, 0xFF00,0x0000,0x0000,0x0000, // 12.5% duty
    0xFFFF,0x0000,0x0000,0x0000, 0xFFFF,0x0000,0x0000,0x0000, // 25% duty
    0xFFFF,0xFFFF,0x0000,0x0000, 0xFFFF,0xFFFF,0x0000,0x0000, // 50% duty
    0xFFFF,0xFFFF,0xFFFF,0x0000, 0xFFFF,0xFFFF,0xFFFF,0x0000, // 75% duty
};


// set duty for channel 0-2.
// 2bit duty: 0=12.5%, 1=25%, 2=50%, 3=75%
static void chan_duty(uint8_t ch, uint8_t duty)
{
    switch (ch) {
        case 0:
            REG_SOUND1CNT_H = 0xF000 | (duty << 6); // initial env vol, duty.
            break;
        case 1:
            REG_SOUND2CNT_L = 0xF000 | (duty << 6); // initial env vol, duty.
            break;
        case 2:
            {
                //if (chan3_duty == duty) {
                //    return; // already set.
                //}
                chan3_duty = duty;

                const uint16_t* p = &chan3_waves[8*duty];
                // Load waveform (into bank 0)
                REG_SOUND3CNT_L = (1<<6);  // Set bank1 for playback, so writes go into bank0.
                for (int i=0; i<8; ++i) {
                    WAVE_RAM[i] = *p++;
                }
                REG_SOUND3CNT_L = (0<<6);  // bank0 active.
            }
            break;
    }
}

static uint8_t chan_pick(uint8_t pri)
{
    // free channel?
/*    for (uint8_t ch = 0; ch < 3; ++ch) {
        if (chan_effect[ch] == SFX_NONE) {
            chan_prev = ch;
            return ch;
        }
    }
*/
    // Look for one we can replace...
    // (start after most recently started one so we cycle through fairly)
    uint8_t ch = chan_prev;
    for (uint8_t i=0; i<3; ++i) {
        ++ch;
        if (ch >= 3) {
            ch = 0;
        }
        if (chan_effect[ch] == SFX_NONE || pri >= chan_pri[ch]) {
            chan_prev = ch;
            return ch;
        }
    }
    return 0xff;
}


static void chan_stop(uint8_t ch) {
    chan_effect[ch] = SFX_NONE;
    chan_time[ch] = 0;
    chan_pri[ch] = 0;
    //psg_attenuation(ch, 0x0f);
    //if (ch!=3) {
    //    psg_freq(ch, 0);
    //}
    chan_off(ch);
}



// returns true if effect needs a psg channel (0-2)
static bool uses_psg(uint8_t effect) {
    return effect != SFX_INEFFECTIVE_THUD;
}

// return true if effect uses noise channel (3)
static bool uses_noise(uint8_t effect) {
    return effect == SFX_OWWWW || effect == SFX_INEFFECTIVE_THUD;
}

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

// 10 bit freq (0...2047) for channel 0-2
static void chan_freq(uint8_t ch, uint16_t freq)
{
    switch(ch) {
        //Length Flag=0, Initial=0
        case 0: REG_SOUND1CNT_X = 0x8000 | freq; break;
        case 1: REG_SOUND2CNT_H = 0x8000 | freq; break;
        case 2: REG_SOUND3CNT_X = 0x8000 | freq; break; // TODO: depends on waveform
        // Different controls for noise channel 3.
    }
}


static void chan_off(uint8_t ch)
{
    uint16_t mask = 0x1100 << ch;
    REG_SOUNDCNT_L &= ~mask;
}

static void chan_on(uint8_t ch)
{
    uint16_t mask = 0x1100 << ch;
    REG_SOUNDCNT_L |= mask;
    switch(ch) {
        case 0: 
            REG_SOUND1CNT_L = 0x0008;  // need sweep direction bit set or no sound
            REG_SOUND1CNT_H = 0xf000 | (2<<6) | (63);   // duty
            break;
        case 1:
            REG_SOUND2CNT_L = 0xf000 | (2<<6) | 63; // initial env vol, duty, len
            break;
        case 2:
            REG_SOUND3CNT_L = (1<<7) | (0<<5);  // 1<<7: channel on, 0<<5: 32 bit wave, 
            REG_SOUND3CNT_H = 0xe000; // vol
            break;
    }
}

// update channel ch, t=elapsed ticks.
// return true if channel should be stopped.
static bool chan_update(uint8_t ch, uint8_t effect, uint8_t t)
{
    if(effect != SFX_NONE && t == 0) {
        chan_on(ch);
        chan_duty(ch, 2);
    }

    switch(effect) {
    case SFX_LASER:
        chan_freq(ch, 2000-(t<<2));
        return (t >= 8);
    case SFX_KABOOM:
        {
        chan_freq(ch, 800-(t<<1));
        }
        return (t >= 32);
    case SFX_WARP:
        {
        chan_freq(ch, (102 + (t<<6)) & 0x7ff);
        }
        return (t >= 64);
    case SFX_OWWWW:
        {
        chan_freq(ch, 200);
        }
        return (t>=64);
    case SFX_HURRYUP:
        chan_freq(ch, 1500);
        return (t>=64);
    case SFX_LOOKOUT:
        {
        chan_freq(ch, 1200);
        }
        return (t >= 48);
    case SFX_MARINE_SAVED:
        {
        chan_freq(ch, 1000);
        }
        return (t >= 48);
    case SFX_ZAPPER_CHARGE:
        {
        chan_freq(ch, 300);
        }
        return false;
    case SFX_ZAPPING:
        {
        chan_freq(ch, 400);
        }
        return false;   // forever!
    case SFX_INEFFECTIVE_THUD:
        chan_freq(ch, 100);
        return (t >= 6);
    case SFX_HIT:
        chan_freq(ch, 300);
        return (t>=8);
    case SFX_BONUS:
        {
        chan_freq(ch, 600);
        }
        return (t >= 48);
    default:
        return true;
    }
}

#if 0
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
#endif


void sfx_init()
{
    REG_SOUNDCNT_L = 0x0077;    // left and right full volume
    REG_SOUNDCNT_H = 0x0002;    // 100% volume for channels 1-4
    REG_SOUNDCNT_X = 0x80;      // Master enable

    chan_prev = 0;
    for (uint8_t ch = 0; ch < 4; ++ch) {
        chan_effect[ch] = SFX_NONE;
        chan_time[ch] = 0;
        chan_pri[ch] = 0;

        chan_off(ch);
    }
    // init channel 3
    chan3_duty = 0;    // chan3_duty does nothing on if same chan3 value.
    chan_duty(3, 2);   // 50% duty
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

