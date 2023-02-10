#ifndef MISC_H
#define MISC_H

#include <stdint.h>
#include <stdbool.h>

uint8_t rnd();
uint32_t bin2bcd(uint32_t in);

// "0123456789ABCDEF"
extern const char hexdigits[16];

void hex32(uint32_t n, char buf[8]);

#endif // MISC_H
