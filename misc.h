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


extern const int8_t sincos24[24+6];

// sin table for angle24 (0..23)
static inline int8_t sin24(uint8_t theta) {return sincos24[theta];}
static inline int8_t cos24(uint8_t theta) {return sincos24[6+theta];}

static inline uint8_t inc24(uint8_t a) { return a<23 ? a+1 : 0; }
static inline uint8_t dec24(uint8_t a) { return a>0 ? a-1 : 23; }

#endif // MISC_H
