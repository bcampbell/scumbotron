#include <cx16.h>
#include <stdint.h>

#include "sys_cx16.h"

// Platform-specifics for cx16


// irq.s, glue.s
extern void inp_tick();
extern void irq_init();
//  .A, byte 0:      | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
//              SNES | B | Y |SEL|STA|UP |DN |LT |RT |
//
//  .X, byte 1:      | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
//              SNES | A | X | L | R | 1 | 1 | 1 | 1 |
extern volatile uint16_t inp_joystate;

// gfx.c
extern uint8_t sprites[16*16*16];
extern uint8_t palette[16*2];   // just 16 colours


// Our VERA memory map
#define VRAM_SPRITES 0x10000    // sprite images
#define VRAM_LAYER1_MAP 0x1b000
#define VRAM_LAYER1_TILES 0x1F000
#define VRAM_LAYER0_MAP  0x00000    //64x64
#define VRAM_LAYER0_TILES  0x08000

// hardwired:
#define VRAM_PALETTE 0x1FA00    // fixed


#define MAX_EFFECTS 8
static uint8_t ekind[MAX_EFFECTS];
static uint8_t etimer[MAX_EFFECTS];
static uint8_t ex[MAX_EFFECTS];
static uint8_t ey[MAX_EFFECTS];

static void rendereffects();


#if 0
#define verawrite0(addr, inc) \
    VERA.control = 0x00;\
    VERA.address = ((addr)&0xffff); \
    VERA.address_hi = (inc) | (((addr)>>16)&1);
#endif

static inline void verawrite0(uint32_t addr, uint8_t inc) {
    VERA.control = 0x00;
    VERA.address = ((addr)&0xffff);
    VERA.address_hi = (inc) | (((addr)>>16)&1);
}

static inline void verawrite1(uint32_t addr, uint8_t inc) {
    VERA.control = 0x01;
    VERA.address = ((addr)&0xffff);
    VERA.address_hi = (inc) | (((addr)>>16)&1);
}

void testum() {
    VERA.control = 0x00;
    // Writing to 0x1b000 onward.
    VERA.address = 0xB000;
    VERA.address_hi = 0x11; // increment = 1
    uint16_t b = inp_joystate;
    for (uint8_t i=0; i<16; ++i) {
        // char, then colour
        VERA.data0 = (b & 0x8000) ? '.' : 'X';
        VERA.data0 = COLOR_BLACK<<4 | COLOR_GREEN;
        b = b<<1;
    }

        VERA.data0 = ekind[0] + '0';
        VERA.data0 = COLOR_BLACK<<4 | COLOR_RED;
        VERA.data0 = etimer[0] + '0';
        VERA.data0 = COLOR_BLACK<<4 | COLOR_RED;

}

void sys_render_start()
{
    // use channel 1 exclusively for writing sprite attrs,
    // use channel 0 for effects.
    verawrite1(0x1FC00, VERA_INC_1);
}

void sproff() {
    const int16_t x = -16;
    const int16_t y = -16;
    VERA.data1 = ((VRAM_SPRITES)>>5) & 0xFF;
    VERA.data1 = (1 << 7) | ((VRAM_SPRITES)>>13);
    VERA.data1 = x & 0xff;  // x lo
    VERA.data1 = (x>>8) & 0x03;  // x hi
    VERA.data1 = y & 0xff;  // y lo
    VERA.data1 = (y>>8) & 0x03;  // y hi
    VERA.data1 = 3<<2; // collmask(4),z(2),vflip,hflip
    VERA.data1 = (1 << 6) | (1 << 4);  // 16x16, 0 palette offset.
}

void sprout(int16_t x, int16_t y, uint8_t img ) {
    VERA.data1 = ((VRAM_SPRITES+256*img)>>5) & 0xFF;
    VERA.data1 = (1 << 7) | ((VRAM_SPRITES+256*img)>>13);
    VERA.data1 = x & 0xff;  // x lo
    VERA.data1 = (x>>8) & 0x03;  // x hi
    VERA.data1 = y & 0xff;  // y lo
    VERA.data1 = (y>>8) & 0x03;  // y hi
    VERA.data1 = (2)<<2; // collmask(4),z(2),vflip,hflip
    VERA.data1 = (1 << 6) | (1 << 4);  // 16x16, 0 palette offset.
}

void sys_render_finish()
{
    // todo: clear unused sprites...
    rendereffects();
    //testum();
}

void sys_clr()
{
    // text layer
    // 64x32*2 (w*h*(char+colour))
    verawrite0(VRAM_LAYER1_MAP, VERA_INC_1);
    const uint8_t* src = sprites;
    for (int i=0; i<64*32*2; ++i) {
        VERA.data0 = ' '; // tile
        VERA.data0 = 0; // colour
    }

    // effects layer
    verawrite0(VRAM_LAYER0_MAP, VERA_INC_1);
    for (uint8_t y = 0; y < 64; ++y) {
        for (uint8_t x = 0; x < 64; ++x) {
            VERA.data0 = 1; //(x&1) ^ (y&1);
            VERA.data0 = 0; //x & 15;
        }
    }

}


void sys_init()
{
    irq_init();

    // screen mode 40x30
    VERA.control = 0x00;    //DCSEL=0
    uint8_t vid = VERA.display.video;
    vid |= 1<<6;    // sprites on
    vid |= 1<<5;    // layer 1 on
    vid |= 1<<4;    // layer 0 on
    VERA.display.video = vid;

    VERA.display.hscale = (uint8_t)(((int32_t)SCREEN_W*128)/640);       // 64 = 2x scale
    VERA.display.vscale = (uint8_t)(((int32_t)SCREEN_H*128)/480);       // 64 = 2x scale
    VERA.control = 0x02;    //DCSEL=1
    VERA.display.hstart = 2;    // 2 to allow some border for rastertiming
    VERA.display.hstop = 640>>2;
    VERA.display.vstart = 0;
    VERA.display.vstop = 480>>1;
    VERA.control = 0x00;    //DCSEL=0

    // text layer setup
    VERA.layer1.config = 0x10;    // 64x32 tiles, !T256C, !bitmapmode, 1bpp
    VERA.layer1.mapbase = (VRAM_LAYER1_MAP>>9);
    VERA.layer1.tilebase = ((VRAM_LAYER1_TILES)>>11)<<2 | 0<<1 | 0; // 8x8 tiles;
    VERA.layer1.hscroll = 0; // 16bit
    VERA.layer1.vscroll = 0; // 16bit

    // layer 0 for effects (spawning, frags, explosions, whatever)
    // 64x64 tiles (=512x512 pixels), but displaying the center of that area,
    // so we don't have to bother with clipping at the edges.

    // effects layer setup
    VERA.layer0.config = (1<<6) | (1<<4) | (0<<3) | (0<<2) | 0;    // 64x64 tiles, !T256C, !bitmapmode, 1bpp
    VERA.layer0.mapbase = (VRAM_LAYER0_MAP>>9);
    VERA.layer0.tilebase = ((VRAM_LAYER0_TILES)>>11)<<2 | 0<<1 | 0; // 8x8 tiles
    VERA.layer0.hscroll = 12*8; // 16bit
    VERA.layer0.vscroll = 12*8; // 16bit


    // load the palette to vram (fixed at 0x1FA00 onward)
    {
        VERA.control = 0x00;
        VERA.address = 0xFA00;
        VERA.address_hi = VERA_INC_1 | 0x01; // hi bit = 1
        const uint8_t* src = palette;
        // just 16 colours
        for (int i=0; i<16*2; ++i) {
            VERA.data0 = *src++;
        }
    }

    // load the sprite images to vram (0x10000 onward)
    {
        verawrite0(VRAM_SPRITES, VERA_INC_1);
        // 16 16*16 images
        const uint8_t* src = sprites;
        for (int i=0; i<16*16*16; ++i) {
            VERA.data0 = *src++;
        }
    }

    // stand-in images for effect layers
    {
        verawrite0(VRAM_LAYER0_TILES, VERA_INC_1);
        for (int i=0; i<8; ++i) {
            VERA.data0 = 0;
        }
        for (int i=0; i<4; ++i) {
            //VERA.data0 = 0xff;
            //VERA.data0 = 0xff;
            //VERA.data0 = 0b10101010;
            //VERA.data0 = 0x00;
            VERA.data0 = 0b10101010;
            VERA.data0 = 0b01010101;
        }
    }

    // clear effect table
    for( uint8_t i=0; i<MAX_EFFECTS; ++i) {
        ekind[i] = EK_NONE;
    }

    // clear the tile layers
    sys_clr();
}

static uint8_t screencode(char asc)
{
    if (asc >= 0x40 && asc < 0x60) {
        return (uint8_t)asc-0x40;
    }
    return (uint8_t)asc;
}

void sys_text(uint8_t cx, uint8_t cy, const char* txt, uint8_t colour)
{
    verawrite0(VRAM_LAYER1_MAP + cy*64*2 + cx*2, VERA_INC_1);
    const char* p = txt;
    while(*p) {
        VERA.data0 = screencode(*p);
        VERA.data0 = colour;
        ++p;
    }
}

void sys_addeffect(uint16_t x, uint16_t y, uint8_t kind)
{
    // find free one
    uint8_t e = 0;
    while( e < MAX_EFFECTS && ekind[e]!=EK_NONE) {
        ++e;
    }
    if (e==MAX_EFFECTS) {
        return; // none free
    }

    ex[e] = ((x+4)/8) + 12;
    ey[e] = ((y+12)/8) + 12;
    ekind[e] = kind;
    etimer[e] = 0;
}


static uint8_t spawnanim[8*16] = {
    1,0,0,0,0,0,0,0,
    2,1,0,0,0,0,0,0,
    3,2,1,0,0,0,0,0,
    4,3,2,1,0,0,0,0,

    5,4,3,2,1,0,0,0,
    6,5,4,3,2,1,0,0,
    7,6,5,4,3,2,1,0,
    8,7,6,5,4,3,2,1,

    0,7,6,5,4,3,2,1,
    0,0,6,5,4,3,2,1,
    0,0,0,5,4,3,2,1,
    0,0,0,0,4,3,2,1,

    0,0,0,0,0,3,2,1,
    0,0,0,0,0,0,2,1,
    0,0,0,0,0,0,0,1,
    0,0,0,0,0,0,0,0,
};


static void do_spawneffect(uint8_t e) {
    uint8_t t = etimer[e];
    uint8_t cx = ex[e];
    uint8_t cy = ey[e];

    uint8_t start = t*8;
    for( uint8_t j=0; j<2; ++j) {
        // just going for the colour byte
        verawrite0(VRAM_LAYER0_MAP + (cy-8)*64*2 + ((cx+j)*2)+1, VERA_INC_128);
        for (uint8_t i = 0; i < 8; ++i) {
            VERA.data0 = spawnanim[start+i];
        }
        // mirror
        for (uint8_t i = 8; i > 0; --i) {
            VERA.data0 = spawnanim[start+(i-1)];
        }

    }
    if(++etimer[e] == 16) {
        ekind[e] = EK_NONE;
    }
}

static void rendereffects()
{
    for(uint8_t e = 0; e < MAX_EFFECTS; ++e) {
        if (ekind[e] == EK_NONE) {
            continue;
        }
        if (ekind[e] == EK_SPAWN) {
            do_spawneffect(e);
        }
    }
}

