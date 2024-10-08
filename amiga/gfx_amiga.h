#pragma once

#include "support/gcc8_c_support.h"
#include <proto/exec.h>
#include <exec/execbase.h>
#include <hardware/custom.h>

#include "../gob.h"    // For ZAPPER_*

// in main_amiga.c
extern volatile struct Custom *custom;

#define CUSTOM_OFFSET(X) offsetof(struct Custom, X)



// 32 pixels around visible screen to sidestep sprite clipping
// (0,0) is offscreen, so (SCREEN_XPAD, SCREEN_YPAD) will be top left.
// Rendering code has to adjust accordingly (game assumes 0,0 is top left).
#define SCREEN_XPAD 32
#define SCREEN_YPAD 32

// num of bytes to get to next bitplane (we're using interleaved bitplanes)
#define SCREEN_PLANE_STRIDE ((SCREEN_XPAD + SCREEN_W + SCREEN_XPAD) / 8)

// num of bytes to get to next line (4 bitplanes)
#define SCREEN_LINE_STRIDE (4*SCREEN_PLANE_STRIDE)

// allocation sizes
#define SCREEN_MEMSIZE (SCREEN_LINE_STRIDE * (SCREEN_YPAD + SCREEN_H + SCREEN_YPAD))
#define COPPERLIST_MEMSIZE 1024


// Exported gfx data (the sprites don't include masks, so we'll calculate them
// as we copy them to chipmem).
extern const unsigned char export_chars_bin[];
extern const unsigned char export_palette_bin[];
extern const unsigned char export_spr16_bin[];
extern const unsigned char export_spr32_bin[];
extern const unsigned char export_spr64x8_bin[];
#define SPR16_SIZE (8*16)   // 16x16, 4 bpl
#define SPR16_NUM 128
#define SPR32_SIZE (16*32)   // 32x32, 4 bpl
#define SPR32_NUM 3
#define SPR64x8_SIZE (32*8) // 64x8, 4bpl
#define SPR64x8_NUM 4

bool gfx_init();
void gfx_shutdown();

void gfx_startrender();
void gfx_wait_for_blitting();
void gfx_present_frame();

void gfx_blit_irq_handler();
