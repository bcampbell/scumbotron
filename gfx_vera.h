#ifndef GFX_VERA_H
#define GFX_VERA_H

// Graphics support for VERA (as used by Commander X16 and others).
//
// The expectation is the various chunks of data we want are already loaded
// into VERA VRAM before gfx_vera_init() is called.
//
// The chunks of data to load are:
//
// VRAM_SPRITES16
// VRAM_SPRITES32
// VRAM_SPRITES64x8
//    assorted sprite images - see SPRXX defs below.
// VRAM_LAYER1_TILES
//    cx16 uses the charset loaded by the kernal.
//    others have to install charset (256 8x8 1bpp chars (so 8 bytes/char)).
//
// VRAM_PALETTE and VRAM_LAYER0_TILES are set up by gfx_vera_init().


// Set up the vera rendering.
void gfx_vera_init();

// Call this each frame to start rendering.
void gfx_vera_render_start();

// Call this each frame to finish rendering.
void gfx_vera_render_finish();

// Bytesize and counts for the various sprite sizes.
#define SPR16_SIZE (8*16)   // 16x16, 4 bpp
#define SPR16_NUM 128
#define SPR32_SIZE (16*32)   // 32x32, 4 bpp
#define SPR32_NUM 3
#define SPR64x8_SIZE (32*8) // 64x8, 4bpp
#define SPR64x8_NUM 4

// Our VERA memory map
#define VRAM_SPRITES16 0x10000
#define VRAM_SPRITES32 (VRAM_SPRITES16 + (SPR16_NUM * SPR16_SIZE))
#define VRAM_SPRITES64x8 (VRAM_SPRITES32 + (SPR32_NUM * SPR32_SIZE))
// layer1 used for text
#define VRAM_LAYER1_MAP 0x1b000         // default textmode location
#define VRAM_LAYER1_TILES 0x1F000       // default charset location
// layer0 for vfx, double-buffered
#define VRAM_LAYER0_MAP_BUF0  0x00000    //64*64*2 = 0x2000
#define VRAM_LAYER0_MAP_BUF1  0x02000    //64*64*2 = 0x2000
#define VRAM_LAYER0_TILES  0x08000      // separate charset for tiles

// hardwired:
//#define VRAM_PSG 0x1F9C0
#define VRAM_PALETTE 0x1FA00
#define VRAM_SPRITE_ATTRS 0x1FC00

#endif

