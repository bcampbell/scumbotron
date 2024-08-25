#pragma once

// We're compiling without stdlib.
// Not sure if it's a gcc issue or intended, but the gcc stdint.h
// skips the definitions and expects another stdint.h in the path,
// so here we are.

// Just define the bare minimum we need.

typedef unsigned long uint32_t;
typedef long int32_t;
typedef unsigned short uint16_t;
typedef short int16_t;
typedef unsigned char uint8_t;
typedef char int8_t;
#define INT16_MAX 0x7fff
