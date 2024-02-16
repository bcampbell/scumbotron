#ifndef PLAT_DETAILS_H
#define PLAT_DETAILS_H

// Platform specific details for use by common code.

#include <stdint.h>

// For sdl2 version, screen size can change on the fly.
// We'll aim to keep it to a reasonable aspect ratio.
extern uint16_t screen_w;
extern uint16_t screen_h;

#define SCREEN_W screen_w
#define SCREEN_H screen_h

#define PLAT_HAS_MOUSE
#define PLAT_HAS_TEXTENTRY
#define PLAT_HAS_SCORESAVE

#endif // PLAT_DETAILS_H
