#include "../plat.h"
#include "vera_psg.h"

void plat_psg(uint8_t chan, uint16_t freq, uint8_t vol, uint8_t waveform, uint8_t pulsewidth)
{
    uint8_t reg = chan * 4;
    psg_writereg(reg, freq & 0xff);
    psg_writereg(reg+1, freq >> 8);
    psg_writereg(reg+2, (3<<6) | vol);  // lrvvvvvv
    psg_writereg(reg+3, (waveform << 6) | pulsewidth);  // wwpppppp
}

