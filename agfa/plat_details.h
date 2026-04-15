#ifndef PLAT_DETAILS_H
#define PLAT_DETAILS_H

#define SCREEN_W 320
#define SCREEN_H 240

// vera defines split out from llvm-mos-sdk cx16.h
#include "vera.h"

// VERA access for VERA-enhanced Agfa Compugraphic 9000PS
#define VERA    (*(volatile struct __vera *)0x04000020)

// #define PLAT_HAS_MOUSE
// #define PLAT_HAS_TEXTENTRY
// #define PLAT_HAS_SCORESAVE

#endif // PLAT_DETAILS_H
