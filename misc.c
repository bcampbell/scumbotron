#include "misc.h"

// by darsie,
// https://www.avrfreaks.net/forum/tiny-fast-prng
uint8_t rnd() {
    static uint8_t s=0xaa,a=0;
    s^=s<<3;
    s^=s>>5;
    s^=a++>>2;
    return s;
}

// double dabble: 8 decimal digits in 32 bits BCD
// from https://stackoverflow.com/a/64955167
uint32_t bin2bcd(uint32_t in) {
  uint32_t bit = 0x4000000; //  99999999 max binary
  uint32_t bcd = 0;
  uint32_t carry = 0;
  if (!in) return 0;
  while (!(in & bit)) bit >>= 1;  // skip to MSB

  while (1) {
    bcd <<= 1;
    bcd += carry; // carry 6s to next BCD digits (10 + 6 = 0x10 = LSB of next BCD digit)
    if (bit & in) bcd |= 1;
    if (!(bit >>= 1)) return bcd;
    carry = ((bcd + 0x33333333) & 0x88888888) >> 1; // carrys: 8s -> 4s
    carry += carry >> 1; // carrys 6s  
  }
}

const char hexdigits[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

void hex32(uint32_t n, char buf[8])
{
    buf[0] = hexdigits[(n >> 28) & 0x0F];
    buf[1] = hexdigits[(n >> 24) & 0x0F];
    buf[2] = hexdigits[(n >> 20) & 0x0F];
    buf[3] = hexdigits[(n >> 16) & 0x0F];
    buf[4] = hexdigits[(n >> 12) & 0x0F];
    buf[5] = hexdigits[(n >> 8) & 0x0F];
    buf[6] = hexdigits[(n >> 4) & 0x0F];
    buf[7] = hexdigits[(n >> 0) & 0x0F];
}

