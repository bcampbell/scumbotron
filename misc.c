#include "misc.h"

// by darsie,
// https://www.avrfreaks.net/forum/tiny-fast-prng
// moved to:
// https://www.avrfreaks.net/s/topic/a5C3l000000UQOhEAO/t115265
//
// When you seed with with
// s= one of 15 18 63 101 129 156 172 208 and
// a=96..138
// you'll get a period of 55552. These are contiguous ranges of 43 seeds
// with good period. So you could seed with something like a=96+x%43.

static uint8_t s=15,a=96;

void rnd_seed(uint8_t seed) {
    // Force to values with a known-reasonable period.
    s = 15;
    a = 96 + (seed & 0x1f);
}

uint8_t rnd() {
    s^=s<<3;
    s^=s>>5;
    s^=a++>>2;
    return s;
}

// double dabble: 8 decimal digits in 32 bits BCD
// from https://stackoverflow.com/a/64955167
// NOTE: will likely crash at some point if binary number is >99999999.
uint32_t bin2bcd32(uint32_t in) {
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

// Convert 8bit binary into 8bit bcd.
uint8_t bin2bcd8(uint8_t in) {
  uint8_t bit = 0x40; //  99 max binary
  uint8_t bcd = 0;
  uint8_t carry = 0;
  if (!in) return 0;
  if (in > 99) return 0x99;
  while (!(in & bit)) bit >>= 1;  // skip to MSB

  while (1) {
    bcd <<= 1;
    bcd += carry; // carry 6s to next BCD digits (10 + 6 = 0x10 = LSB of next BCD digit)
    if (bit & in) bcd |= 1;
    if (!(bit >>= 1)) return bcd;
    carry = ((bcd + 0x33) & 0x88) >> 1; // carrys: 8s -> 4s
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

// Return angle between x and y, in range (0..23).
// (coords have topleft origin, ie y=down, x=right)
// Angle 0 is vertically up, proceeding clockwise.
// Method based on:
// http://www.dustmop.io/blog/2015/07/22/discrete-arctan-in-6502/
// TODO: could do this entirely 8bit.
uint8_t arctan24(int16_t x, int16_t y) {

    // Look up result angle (0..23) using quadrant and region.
    // Index is xxQQRRRR
    static const uint8_t arctan_table[32] = {
        0,1,2,3, 6,5,4,3,           // q0
        12,11,10,9, 6,7,8,9,        // q1
        12,13,14,15, 18,17,16,15,   // q2
        0,23,22,21, 18,19,20,21,    // q3
    };

    // Quadrant numbering:
    //
    //    3 | 0
    //    --+--
    //    2 | 1
    uint8_t quadrant;
    if (x > 0) {
        if (y > 0) {
            quadrant = 1;
        } else {
            quadrant = 0;
            y = -y;
        }
    } else {
        x = -x;
        if (y > 0) {
            quadrant = 2;
        } else {
            quadrant = 3;
            y = -y;
        }
    }

    int16_t small, large, half;
    // Calculate region (0..7) within the quadrant.
    // Note: the region number is pretty arbitrary, just used as a lookup.
    // The region ordering varies every 45 degrees.
    uint8_t region;
    if (x < y) {
        region = 0;
        small = x;
        large = y;
    } else {
        region = 4;
        small = y;
        large = x;
    }
    half = small/2;

    // compare to 2.5 slope
    if ((small * 2) + half > large) {
        // compare to 1.25
        if (small + (half/2) > large) {
            region += 3;
        } else {
            region += 2;
        }
    } else {
        // compare to 7.5
        if ((small * 8) - half > large) {
            region += 1;
        } else {
            region += 0;
        }
    }

    uint8_t foo = arctan_table[(quadrant *8) + region];
    return foo;
}

// Return angle between x and y, in range (0..7).
// (coords have topleft origin, ie y=down, x=right)
// Angle 0 is vertically up, proceeding clockwise.
uint8_t arctan8(int16_t x, int16_t y) {

    // Look up result angle (0..7) using quadrant and region.
    // index is: xxxxQQRR
    static const uint8_t lookup8[16] = {
        0,1,1,2,    // q0
        4,3,3,2,    // q1
        4,5,5,6,    // q2
        0,7,7,6,    // q3
    };

    // Quadrant numbering:
    //
    //    3 | 0
    //    --+--
    //    2 | 1
    uint8_t quadrant;
    if (x > 0) {
        if (y > 0) {
            quadrant = 1;
        } else {
            quadrant = 0;
            y = -y;
        }
    } else {
        x = -x;
        if (y > 0) {
            quadrant = 2;
        } else {
            quadrant = 3;
            y = -y;
        }
    }

    // Calculate region (0..3) within the quadrant.
    // Note: the region numbers are relfected per quadrant.
    uint8_t region;
    if (x < y) {
        region = (x*2 < y) ? 0 : 1;
    } else {
        region = (y*2 < x) ? 3 : 2;
    }

    return lookup8[(quadrant << 2) | region];
}

const uint8_t angle24toangle8[24] = {
    0,0,1,1,1,2,
    2,2,3,3,3,4,
    4,4,5,5,5,6,
    6,6,7,7,7,0,
};

// Combined sin & cos table for angle 0..23
const int8_t sincos24[24+6] = {
  0,  // start sin
  32,
  63,
  89,
  109,
  122,
  127, // start cosine
  122,
  109,
  89,
  63,
  32,
  0,
  -32,
  -63,
  -89,
  -109,
  -122,
  -127, // start of cosine 
  -122,
  -109,
  -89,
  -63,
  -32,
  0,    // wrap
  32,
  63,
  89,
  109,
  122,
};


uint8_t turntoward8(uint8_t dir, uint8_t target)
{
    int8_t d = target-dir;

    // find shortest route
    if (d < -4) {
        d += 8;
    }
    if (d > 4) {
        d -= 8;
    }

    // turn
    if (d > 0) {
        dir = (dir+1) & 0x07;
    } else if (d < 0) {
        dir = (dir-1) & 0x07;
    }
    return dir;
}

