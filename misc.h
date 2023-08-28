#ifndef MISC_H
#define MISC_H

#include <stdint.h>
#include <stdbool.h>

void rnd_seed(uint8_t seed);
uint8_t rnd();
uint32_t bin2bcd32(uint32_t in);
uint8_t bin2bcd8(uint8_t in);

// "0123456789ABCDEF"
extern const char hexdigits[16];

void hex32(uint32_t n, char buf[8]);

uint8_t arctan24(int16_t x, int16_t y);
uint8_t arctan8(int16_t x, int16_t y);

// Convert angle24 (0..23) to angle8 (0..7).
extern const uint8_t angle24toangle8[24];


// circle of radius 512
// TODO: replace with byte-oriented sincos24 table
extern const int16_t circ24x[24];
extern const int16_t circ24y[24];

#endif // MISC_H
