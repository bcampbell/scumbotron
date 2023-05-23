#ifndef MISC_H
#define MISC_H

#include <stdint.h>
#include <stdbool.h>

void rnd_seed(uint8_t seed);
uint8_t rnd();
uint32_t bin2bcd(uint32_t in);

// "0123456789ABCDEF"
extern const char hexdigits[16];

void hex32(uint32_t n, char buf[8]);

uint8_t arctan24(int16_t x, int16_t y);
#endif // MISC_H
