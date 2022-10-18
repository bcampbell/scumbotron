#include <cx16.h>
#include <stdint.h>

// Platform-specifics for cx16

#define SCREEN_W 320
#define SCREEN_H 240

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
}

void sys_render_start()
{
    VERA.control = 0x00;
    // sprite attrs start at 0x1FC00
    VERA.address = 0xFC00;
    VERA.address_hi = VERA_INC_1 | 0x01; // hi bit = 1
}

void sproff() {
    const int16_t x = -16;
    const int16_t y = -16;
    VERA.data0 = ((0x10000+256)>>5) & 0xFF;
    VERA.data0 = (1 << 7) | ((0x10000+256)>>13);
    VERA.data0 = x & 0xff;  // x lo
    VERA.data0 = (x>>8) & 0x03;  // x hi
    VERA.data0 = y & 0xff;  // y lo
    VERA.data0 = (y>>8) & 0x03;  // y hi
    VERA.data0 = 3<<2; // collmask(4),z(2),vflip,hflip
    VERA.data0 = (1 << 6) | (1 << 4);  // 16x16, 0 palette offset.
}

void sprout(int16_t x, int16_t y, uint8_t img) {
    VERA.data0 = ((0x10000+256*img)>>5) & 0xFF;
    VERA.data0 = (1 << 7) | ((0x10000+256*img)>>13);
    VERA.data0 = x & 0xff;  // x lo
    VERA.data0 = (x>>8) & 0x03;  // x hi
    VERA.data0 = y & 0xff;  // y lo
    VERA.data0 = (y>>8) & 0x03;  // y hi
    VERA.data0 = 3<<2; // collmask(4),z(2),vflip,hflip
    VERA.data0 = (1 << 6) | (1 << 4);  // 16x16, 0 palette offset.
}

void sys_render_finish()
{
}

void sys_init()
{
    irq_init();

    // screen mode 40x30
    VERA.control = 0x00;    //DCSEL=0
    uint8_t vid = VERA.display.video;
    vid |= 1<<6;    // enable sprites
    vid |= 1<<5; // layer 1 on
    vid &= ~(1<<4);    // layer 1 off
    VERA.display.video = vid;

    VERA.display.hscale = (uint8_t)(((int32_t)SCREEN_W*128)/640);       // 64 = 2x scale
    VERA.display.vscale = (uint8_t)(((int32_t)SCREEN_H*128)/480);       // 64 = 2x scale
    VERA.control = 0x02;    //DCSEL=1
    VERA.display.hstart = 2;    // 2 to allow some border for rastertiming
    VERA.display.hstop = 640>>2;
    VERA.display.vstart = 0;
    VERA.display.vstop = 480>>1;

    // layer setup
    VERA.layer1.config = 0x10;    // 64x32 tiles, !T256C, !bitmapmode, 1bpp
//    VERA.layer0.mapbase = 
//    VERA.layer0.tilebase = 
    VERA.layer1.hscroll = 0; // 16bit
    VERA.layer1.vscroll = 0; // 16bit

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
        VERA.control = 0x00;
        VERA.address = 0x0000;
        VERA.address_hi = VERA_INC_1 | 0x01; // hi bit = 1

        // 16 16*16 images
        const uint8_t* src = sprites;
        for (int i=0; i<16*16*16; ++i) {
            VERA.data0 = *src++;
        }
    }

    // clear the screen (0x1B0000)
    {
        VERA.control = 0x00;
        VERA.address = 0xB000;
        VERA.address_hi = VERA_INC_1 | 0x01; // hi bit = 1

        // 16 16*16 images
        const uint8_t* src = sprites;
        for (int i=0; i<64*32; ++i) {
            VERA.data0 = ' '; // tile
            VERA.data0 = 0; // colour
        }
    }
}


