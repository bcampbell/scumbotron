#include <cx16.h>
#include <cbm.h>

#include "../plat.h"
#include "../gob.h" // for ZAPPER_*
#include "../misc.h"


// Platform-specifics for cx16

// To break:
// asm ("stp");

// irq.s
// Joystick 0 is the built-into-the-kernal keyboard joystick,
// but we don't like the mapping, so we'll do our own keyboard-driven
// joystick!
extern void irq_init();
extern void keyhandler_init();
extern uint8_t inp_keystates[16];
extern void waitvbl();
extern void inp_enabletextentry();
extern void inp_disabletextentry();

static void update_inp_mouse();

// start PLAT_HAS_MOUSE
int16_t plat_mouse_x = 0;
int16_t plat_mouse_y = 0;
uint8_t plat_mouse_buttons = 0;
static uint8_t mouse_watchdog = 0; // >0 = active
// end PLAT_HAS_MOUSE

#define SPR16_SIZE (8*16)   // 16x16, 4 bpp
#define SPR16_NUM 128
#define SPR32_SIZE (16*32)   // 32x32, 4 bpp
#define SPR32_NUM 3
#define SPR64x8_SIZE (32*8) // 64x8, 4bpp
#define SPR64x8_NUM 4
extern unsigned char export_palette_bin[];
extern unsigned int export_palette_bin_len;

// Our VERA memory map
#define VRAM_SPRITES16 0x10000
#define VRAM_SPRITES32 (VRAM_SPRITES16 + (SPR16_NUM * SPR16_SIZE))
#define VRAM_SPRITES64x8 (VRAM_SPRITES32 + (SPR32_NUM * SPR32_SIZE))
#define VRAM_LAYER1_MAP 0x1b000
#define VRAM_LAYER1_TILES 0x1F000
// layer0 is double-buffered
#define VRAM_LAYER0_MAP_BUF0  0x00000    //64*64*2 = 0x2000
#define VRAM_LAYER0_MAP_BUF1  0x02000    //64*64*2 = 0x2000
#define VRAM_LAYER0_TILES  0x08000

// hardwired:
#define VRAM_PALETTE 0x1FA00
#define VRAM_SPRITE_ATTRS 0x1FC00
#define VRAM_PSG 0x1F9C0

static void sprout16(int16_t x, int16_t y, uint8_t img);
static void sprout16_highlight(int16_t x, int16_t y, uint8_t img);
static void sprout32(int16_t x, int16_t y, uint8_t img);
static void sprout32_highlight(int16_t x, int16_t y, uint8_t img);

static void hline_chars_noclip(uint8_t cx_begin, uint8_t cx_end, uint8_t cy, uint8_t ch, uint8_t colour);
static void vline_chars_noclip(uint8_t cx, uint8_t cy_begin, uint8_t cy_end, uint8_t ch, uint8_t colour);

static void clr_layer0();

static void plat_init();
static void plat_render_start();
static void plat_render_finish();

// which layer0 buffer we're displaying (we'll draw into the other one)
static uint8_t layer0_displaybuf;


// Number of free hw sprites left.
static uint8_t sprremaining;
// from previous frame
static uint8_t sprremainingprev;

static void sfx_init();
static void sfx_tick();

// set the border colour for raster-timing
void plat_gatso(uint8_t g)
{
    VERA.display.border = g;
}

static inline void veraaddr0(uint32_t addr, uint8_t inc) {
    VERA.control = 0x00;
    VERA.address = ((addr)&0xffff);
    VERA.address_hi = (inc) | (((addr)>>16)&1);
}

static inline void veraaddr1(uint32_t addr, uint8_t inc) {
    VERA.control = 0x01;
    VERA.address = ((addr)&0xffff);
    VERA.address_hi = (inc) | (((addr)>>16)&1);
}

void plat_render_start()
{
    // Toggle the layer0 buffer.
    if(layer0_displaybuf) {
       layer0_displaybuf = 0;
       VERA.layer0.mapbase = (VRAM_LAYER0_MAP_BUF0>>9);
    } else {
       layer0_displaybuf = 1;
       VERA.layer0.mapbase = (VRAM_LAYER0_MAP_BUF1>>9);
    }
    clr_layer0();

    // cycle colour 15
    uint8_t i = (((tick >> 1)) & 0x7) + 2;
    veraaddr0(VRAM_PALETTE + (i*2), VERA_INC_1);
    veraaddr1(VRAM_PALETTE + (15*2), VERA_INC_1);
    // 2 bytes for colour entry
    VERA.data1 = VERA.data0;
    VERA.data1 = VERA.data0;

    // Set up for writing sprites.
    sprremaining = 128;
}

void plat_render_finish()
{
    if (sprremainingprev < sprremaining) {
        // Clear sprites used last frame but not this one.
        uint8_t i;
        // Set zdepth=0 for disable (in 6th byte of sprite attr)
        veraaddr1(VRAM_SPRITE_ATTRS + (sprremainingprev*8) + 6, VERA_INC_8);
        for (i = 0; i < sprremaining - sprremainingprev; ++i) {
            VERA.data1 = 0x00;
        }
    }
    sprremainingprev = sprremaining;

    //testum();
}

static void sprout16(int16_t x, int16_t y, uint8_t img ) {
    if (sprremaining == 0) {
        return;
    }
    --sprremaining;
    veraaddr1(VRAM_SPRITE_ATTRS + (sprremaining*8), VERA_INC_1);

    const uint32_t addr = VRAM_SPRITES16 + (SPR16_SIZE * img);
    // 0: aaaaaaaa
    //    a: img address (bits 12:5) so always 16-byte aligned.
    VERA.data1 = (addr>>5) & 0xFF;
    // 1: m---aaaa
    //    a: img address (bits 16:15)
    //    m: mode (0: 4bpp 1: 8bpp)
    VERA.data1 = (0 << 7) | (addr>>13);
    // 2: xxxxxxxx
    //    x: xpos (bits 0:7)
    VERA.data1 = (x >> FX) & 0xff;  // x lo
    // 3: ------xx
    VERA.data1 = (x >> (FX + 8)) & 0x03;  // x hi
    // 4: yyyyyyyy
    //    x: xpos (bits 0:7)
    VERA.data1 = (y >> FX) & 0xff;  // y lo
    VERA.data1 = (y >> (FX + 8)) & 0x03;  // y hi
    // 6: cccczzvh
    //    c: collisionmask
    //    z: zdepth (0=sprite off)
    //    v: vflip
    //    h: hflip
    VERA.data1 = (3) << 2; // collmask(4),z(2),vflip,hflip
    // 7: hhwwpppp
    //    h: height
    //    w: width
    //    p: palette offset
    VERA.data1 = (1 << 6) | (1 << 4);  // 16x16, 0 palette offset.
}

static void sprout16_highlight(int16_t x, int16_t y, uint8_t img ) {
    if (sprremaining == 0) {
        return;
    }
    --sprremaining;
    veraaddr1(VRAM_SPRITE_ATTRS + (sprremaining*8), VERA_INC_1);
    const uint32_t addr = VRAM_SPRITES16 + (SPR16_SIZE * img);
    VERA.data1 = (addr>>5) & 0xFF;
    VERA.data1 = (0 << 7) | (addr>>13);
    VERA.data1 = (x >> FX) & 0xff;  // x lo
    VERA.data1 = (x >> (FX + 8)) & 0x03;  // x hi
    VERA.data1 = (y >> FX) & 0xff;  // y lo
    VERA.data1 = (y >> (FX + 8)) & 0x03;  // y hi
    VERA.data1 = (3) << 2; // collmask(4),z(2),vflip,hflip
    // wwhhpppp
    VERA.data1 = (1 << 6) | (1 << 4) | 1;  // 16x16, palette offset 1.
}

static void sprout32(int16_t x, int16_t y, uint8_t img ) {
    if (sprremaining == 0) {
        return;
    }
    --sprremaining;
    veraaddr1(VRAM_SPRITE_ATTRS + (sprremaining*8), VERA_INC_1);
    const uint32_t addr = VRAM_SPRITES32 + (SPR32_SIZE * img);
    VERA.data1 = (addr>>5) & 0xFF;
    VERA.data1 = (0 << 7) | (addr>>13);
    VERA.data1 = (x >> FX) & 0xff;  // x lo
    VERA.data1 = (x >> (FX + 8)) & 0x03;  // x hi
    VERA.data1 = (y >> FX) & 0xff;  // y lo
    VERA.data1 = (y >> (FX + 8)) & 0x03;  // y hi
    VERA.data1 = (3) << 2; // collmask(4),z(2),vflip,hflip
    // wwhhpppp
    VERA.data1 = (2 << 6) | (2 << 4);  // 32x32, 0 palette offset.
}

static void sprout32_highlight(int16_t x, int16_t y, uint8_t img ) {
    if (sprremaining == 0) {
        return;
    }
    --sprremaining;
    veraaddr1(VRAM_SPRITE_ATTRS + (sprremaining*8), VERA_INC_1);
    const uint32_t addr = VRAM_SPRITES32 + (SPR32_SIZE * img);
    VERA.data1 = (addr>>5) & 0xFF;
    VERA.data1 = (0 << 7) | (addr>>13);
    VERA.data1 = (x >> FX) & 0xff;  // x lo
    VERA.data1 = (x >> (FX + 8)) & 0x03;  // x hi
    VERA.data1 = (y >> FX) & 0xff;  // y lo
    VERA.data1 = (y >> (FX + 8)) & 0x03;  // y hi
    VERA.data1 = (3) << 2; // collmask(4),z(2),vflip,hflip
    // wwhhpppp
    VERA.data1 = (2 << 6) | (2 << 4) | 1;  // 32x32, palette offset 1.
}


static void sprout64x8(int16_t x, int16_t y, uint8_t img ) {
    if (sprremaining == 0) {
        return;
    }
    --sprremaining;
    veraaddr1(VRAM_SPRITE_ATTRS + (sprremaining*8), VERA_INC_1);
    const uint32_t addr = VRAM_SPRITES64x8 + (SPR64x8_SIZE * img);
    VERA.data1 = (addr>>5) & 0xFF;
    VERA.data1 = (0 << 7) | (addr>>13);
    VERA.data1 = (x >> FX) & 0xff;  // x lo
    VERA.data1 = (x >> (FX + 8)) & 0x03;  // x hi
    VERA.data1 = (y >> FX) & 0xff;  // y lo
    VERA.data1 = (y >> (FX + 8)) & 0x03;  // y hi
    VERA.data1 = (3) << 2; // collmask(4),z(2),vflip,hflip
    // wwhhpppp
    VERA.data1 = (0 << 6) | (3 << 4);  // 64x8, 0 palette offset.
}


void plat_clr()
{
    uint8_t y;
    int i;
    // text layer
    // 64x32*2 (w*h*(char+colour))
    veraaddr0(VRAM_LAYER1_MAP, VERA_INC_1);
    for (i=0; i<64*32*2; ++i) {
        VERA.data0 = ' '; // tile
        VERA.data0 = 0; // colour
    }
}


void plat_init()
{
    // It's assumed the exported graphics data is already loaded into VRAM.
    // In theory we could load the graphics off disk here instead of using the
    // scumbotron.asm unpacker.
    uint8_t vid;
    irq_init();
    sfx_init();

    // screen mode 40x30
    VERA.control = 0x00;    //DCSEL=0
    vid = VERA.display.video;
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
    layer0_displaybuf = 0;
    VERA.layer0.mapbase = (VRAM_LAYER0_MAP_BUF0>>9);
    VERA.layer0.tilebase = ((VRAM_LAYER0_TILES)>>11)<<2 | 0<<1 | 0; // 8x8 tiles
    VERA.layer0.hscroll = 12*8; // 16bit
    VERA.layer0.vscroll = 12*8; // 16bit

    // Init mouse
    cx16_k_mouse_config(0xff, SCREEN_W/8, SCREEN_H/8);

    // Load palette into VRAM
    {
        const uint8_t* src = export_palette_bin;
        uint8_t i;
        veraaddr0(VRAM_PALETTE, VERA_INC_1);
        // just 16 colours
        for (i=0; i<16; ++i) {
            VERA.data0 = *src++;
            VERA.data0 = *src++;
        }

        // then a set of 16 whites for highlight flashing
        for (i=0; i<16; ++i) {
            VERA.data0 = 0xff;
            VERA.data0 = 0x0f;
        }
    }
#if 0
    {
        // Uncompress 16x16 sprites
        // We're uncompressing into 8KB bank... so
        // 64 sprites is our limit...
        /*const volatile uint8_t* src = BANK_RAM;
        int i;
        cx16_k_memory_decompress(export_spr16_zbin, (uint8_t*)BANK_RAM);
        veraaddr0(VRAM_SPRITES16, VERA_INC_1);
        for (i = 0; i < SPR16_NUM * SPR16_SIZE; ++i) {
            VERA.data0 = *src++;
        }
        */
        // Uncompress directly into vram.
        // Earlier kernal had a bug which meant I couldn't decompress
        // directly to vram, so I used the 8KB banked ram as a temp
        // buffer. But then I decided we needed more than 8KB of 16x16
        // sprites. Luckily the decompress-to-vram had been fixed by then.
        veraaddr0(VRAM_SPRITES16, VERA_INC_1);
        cx16_k_memory_decompress(export_spr16_zbin, (uint8_t*)&(VERA.data0));
    }
    {
        // Uncompress 32x32 sprites
        const volatile uint8_t* src = BANK_RAM;
        int i;
        cx16_k_memory_decompress(export_spr32_zbin, (uint8_t*)BANK_RAM);
        veraaddr0(VRAM_SPRITES32, VERA_INC_1);
        // 8 32x32 images
        for (i = 0; i < SPR32_NUM * SPR32_SIZE; ++i) {
            VERA.data0 = *src++;
        }
    }
    {
        // Uncompress 64x8 sprites
        const volatile uint8_t* src = BANK_RAM;
        int i;
        cx16_k_memory_decompress(export_spr64x8_zbin, (uint8_t*)BANK_RAM);
        veraaddr0(VRAM_SPRITES64x8, VERA_INC_1);
        // 8 32x32 images
        for (i = 0; i < SPR64x8_NUM * SPR64x8_SIZE; ++i) {
            VERA.data0 = *src++;
        }
    }
#endif

    // stand-in images for effect layers
    {
        uint8_t i;
        veraaddr0(VRAM_LAYER0_TILES, VERA_INC_1);
        // char 0
        for (i=0; i<8; ++i) {
            VERA.data0 = 0;
        }
        //char 1
        for (i=0; i<4; ++i) {
            //VERA.data0 = 0b10101010;
            //VERA.data0 = 0b01010101;
            VERA.data0 = 0b11111111;
            VERA.data0 = 0b00000000;
        }
        // hline
        for (i=0; i<8; ++i) {
            uint8_t j;
            for (j=0; j<8; ++j) {
                VERA.data0 = (i==j) ? 0xff : 0x00;
            }
        }
        // vline
        for (i=0; i<8; ++i) {
            uint8_t j;
            for (j=0; j<8; ++j) {
                VERA.data0 = 1<<(7-i);
            }
        }
    }

    // Clear the VERA sprite attr table.
    {
        uint8_t i;
        veraaddr0(VRAM_SPRITE_ATTRS, VERA_INC_1);
        for(i=0; i<128; ++i) {
            uint8_t j;
            for(j=0; j<8; ++j) {
                VERA.data0 = 0x00;
            }
        }
        sprremaining = 128;
        sprremainingprev = 128;
    }

    // clear both layer0 buffers (used for effects)
    {
        int i;
        veraaddr0(VRAM_LAYER0_MAP_BUF0, VERA_INC_1);
        for (i=0; i<64*64; ++i) {
            VERA.data0 = 0; // tile
            VERA.data0 = 0; // colour
        }
        veraaddr0(VRAM_LAYER0_MAP_BUF1, VERA_INC_1);
        for (i=0; i<64*64; ++i) {
            VERA.data0 = 0; // tile
            VERA.data0 = 0; // colour
        }
    }

    // clear layer1 (text layer)
    plat_clr();
}

/*
 * HUD/onscreen text
 */

static uint8_t screencode(char asc)
{
    if (asc >= 0x40 && asc < 0x60) {
        return (uint8_t)asc-0x40;
    }
    return (uint8_t)asc;
}

void plat_textn(uint8_t cx, uint8_t cy, const char* txt, uint8_t len, uint8_t colour)
{
    veraaddr0(VRAM_LAYER1_MAP + cy*64*2 + cx*2, VERA_INC_1);
    while(len--) {
        VERA.data0 = screencode(*txt);
        VERA.data0 = colour;
        ++txt;
    }
}

void plat_text(uint8_t cx, uint8_t cy, const char* txt, uint8_t colour)
{
    uint8_t len = 0;
    while(txt[len] != '\0') {
        ++len;
    }
    plat_textn(cx, cy, txt, len, colour);
}


// cw must be even!
void plat_mono4x2(uint8_t cx, int8_t cy, const uint8_t* src, uint8_t cw, uint8_t ch, uint8_t basecol)
{
    // translate from 2x2 pixel block to charset rom char.
    static const uint8_t chr[16] = {
        0x20,0x7c,0x7e,0xe2,0x6c,0xe1,0x7f,0xfb,
        0x7b,0xff,0x61,0xec,0x62,0xfe,0xfc,0xa0
    };
    uint8_t x;
    uint8_t y;

    for (y=0; y < ch; ++y) {
        veraaddr0(VRAM_LAYER1_MAP + (cy+y)*64*2 + cx*2, VERA_INC_1);
        uint8_t colour = (basecol + y) & 0x0f;
        for (x=0; x < cw; x+=2) {
            // left char
            VERA.data0 = chr[(*src) >>4];
            VERA.data0 = (colour & 0x0f);
            // right char
            VERA.data0 = chr[*(src) & 0x0f];
            VERA.data0 = (colour & 0x0f);
            //--c;
            ++src;
        }
    }
}


void plat_hud(uint8_t level, uint8_t lives, uint32_t score)
{
    const uint8_t cx = 0;
    const uint8_t cy = 0;
    uint8_t c = 1;
    veraaddr0(VRAM_LAYER1_MAP + cy*64*2 + cx*2, VERA_INC_1);


    // Level
    VERA.data0 = screencode('L');
    VERA.data0 = c;
    VERA.data0 = screencode('V');
    VERA.data0 = c;
    VERA.data0 = screencode(' ');
    VERA.data0 = c;

    uint8_t bcd = bin2bcd8(level);
    VERA.data0 = screencode(hexdigits[bcd >> 4]);
    VERA.data0 = c;
    VERA.data0 = screencode(hexdigits[bcd & 0x0f]);
    VERA.data0 = c;

    VERA.data0 = screencode(' ');
    VERA.data0 = c;
    VERA.data0 = screencode(' ');
    VERA.data0 = c;

    // Score
    {
        uint32_t bcd = bin2bcd32(score);
        VERA.data0 = screencode(hexdigits[(bcd >> 28)&0x0f]);
        VERA.data0 = c;
        VERA.data0 = screencode(hexdigits[(bcd >> 24)&0x0f]);
        VERA.data0 = c;
        VERA.data0 = screencode(hexdigits[(bcd >> 20)&0x0f]);
        VERA.data0 = c;
        VERA.data0 = screencode(hexdigits[(bcd >> 16)&0x0f]);
        VERA.data0 = c;
        VERA.data0 = screencode(hexdigits[(bcd >> 12)&0x0f]);
        VERA.data0 = c;
        VERA.data0 = screencode(hexdigits[(bcd >> 8)&0x0f]);
        VERA.data0 = c;
        VERA.data0 = screencode(hexdigits[(bcd >> 4)&0x0f]);
        VERA.data0 = c;
        VERA.data0 = screencode(hexdigits[(bcd >> 0)&0x0f]);
        VERA.data0 = c;
    }

    VERA.data0 = screencode(' ');
    VERA.data0 = c;
    VERA.data0 = screencode(' ');
    VERA.data0 = c;

    // Lives
    {
        uint8_t i;
        c = 2;
        for(i=0; i < 7 && i < lives; ++i) {
            VERA.data0 = 0x53;  // heart
            VERA.data0 = c;
        }
        VERA.data0 = screencode((lives>7) ? '+' : ' ');
        VERA.data0 = c;
    }
#if 0
    //inp_virtpad = tick;
    // debug cruft
    VERA.data0 = screencode(' ');
    VERA.data0 = c;
    VERA.data0 = screencode(' ');
    VERA.data0 = c;
    uint16_t p = inp_virtpad;
    for (int8_t i = 0; i<16; ++i) {
        VERA.data0 = screencode((p & 0x8000) ? 'O' : '.');
        VERA.data0 = c;
        p = p<<1;
    }
#endif
}

/*
 * Effects
 */


static inline uint32_t layer0addr(uint8_t cx, uint8_t cy)
{
    // Layer is 64 x 64 chars, border 12.
    // And doublebuffered. Draw into the non-displayed one.
    if (layer0_displaybuf==0) {
        return VRAM_LAYER0_MAP_BUF1 + (cy+12)*64*2 + (cx+12)*2;
    } else {
        return VRAM_LAYER0_MAP_BUF0 + (cy+12)*64*2 + (cx+12)*2;
    }
}

// Draw vertical line of chars, range [cy_begin, cy_end).
static void vline_chars_noclip(uint8_t cx, uint8_t cy_begin, uint8_t cy_end, uint8_t ch, uint8_t colour)
{
    uint8_t n;
    // char
    veraaddr0(layer0addr(cx,cy_begin), VERA_INC_128);
    for (n = cy_end - cy_begin; n; --n) {
        VERA.data0 = ch;
    }
    // colour
    veraaddr0(layer0addr(cx,cy_begin)+1, VERA_INC_128);
    for (n = cy_end - cy_begin; n; --n) {
        VERA.data0 = colour;
    }
}

// Draw horizontal line of chars, range [cx_begin, cx_end).
static void hline_chars_noclip(uint8_t cx_begin, uint8_t cx_end, uint8_t cy, uint8_t ch, uint8_t colour)
{
    uint8_t n;
    // char
    veraaddr0(layer0addr(cx_begin,cy), VERA_INC_2);
    for (n = cx_end - cx_begin; n; --n) {
        VERA.data0 = ch;
    }
    // colour
    veraaddr0(layer0addr(cx_begin,cy)+1, VERA_INC_2);
    for (n = cx_end - cx_begin; n; --n) {
        VERA.data0 = colour;
    }
}

static inline int8_t cclip(int8_t v, int8_t low, int8_t high)
{
    if (v < low) {
        return low;
    } else if (v > high) {
        return high;
    } else {
        return v;
    }
}

// draw box in char coords, with clipping
// (note cx,cy can be negative)
void plat_drawbox(int8_t cx, int8_t cy, uint8_t w, uint8_t h, uint8_t ch, uint8_t colour)
{
    int8_t x0,y0,x1,y1;
    x0 = cclip(cx, 0, SCREEN_W / 8);
    x1 = cclip(cx + w, 0, SCREEN_W / 8);
    y0 = cclip(cy, 0, SCREEN_H / 8);
    y1 = cclip(cy + h, 0, SCREEN_H / 8);

    // top
    if (y0 == cy) {
        hline_chars_noclip((uint8_t)x0, (uint8_t)x1, (uint8_t)y0, ch, colour);
    }
    if (h<=1) {
        return;
    }

    // bottom
    if (y1 - 1 == cy + h-1) {
        hline_chars_noclip((uint8_t)x0, (uint8_t)x1, (uint8_t)y1 - 1, ch, colour);
    }
    if (h<=2) {
        return;
    }

    // left (excluding top and bottom)
    if (x0 == cx) {
        vline_chars_noclip((uint8_t)x0, (uint8_t)y0, (uint8_t)y1, ch, colour);
    }
    if (w <= 1) {
        return;
    }

    // right (excluding top and bottom)
    if (x1 - 1 == cx + w - 1) {
        vline_chars_noclip((uint8_t)x1 - 1, (uint8_t)y0, (uint8_t)y1, ch, colour);
    }
}

static void clr_layer0()
{
    uint8_t cy;
    for (cy = 0; cy < (SCREEN_H / 8); ++cy) {
        uint8_t cx;
        // just set colour to 0
        veraaddr0(layer0addr(0, cy) + 1, VERA_INC_2);
        for (cx = 0; cx < (SCREEN_W / 8); ++cx) {
            VERA.data0 = 0;
        }
    }
}

/*
 * SFX
 */

#define MAX_SFX 4

uint8_t sfx_effect[MAX_SFX];
uint8_t sfx_timer[MAX_SFX];
uint8_t sfx_next;

static void sfx_init()
{
    uint8_t ch;
    sfx_next = 0;
    for (ch = 0; ch < MAX_SFX; ++ch) {
        sfx_effect[ch] = SFX_NONE;
    }
}

void plat_sfx_play(uint8_t effect)
{
    uint8_t ch = sfx_next++;
    if (sfx_next >= MAX_SFX) {
        sfx_next = 0;
    }

    sfx_effect[ch] = effect;
    sfx_timer[ch] = 0;
}


static inline void psg(uint16_t freq, uint8_t vol, uint8_t waveform, uint8_t pulsewidth)
{
    VERA.data0 = freq & 0xff;
    VERA.data0 = freq >> 8;
    VERA.data0 = (3 << 6) | vol;  // lrvvvvvv
    VERA.data0 = (waveform << 6) | pulsewidth;         // wwpppppp
}


static void sfx_tick()
{
    uint8_t ch;
    veraaddr0(VRAM_PSG, VERA_INC_1);
    for (ch=0; ch < MAX_SFX; ++ch) {
        uint8_t t = sfx_timer[ch]++;
        switch (sfx_effect[ch]) {
        case SFX_LASER:
            psg(2000 - (t<<6), (63-t)/4, 0, t);
            if (t>=63) {
                sfx_effect[ch] = SFX_NONE;
            }
            break;
        case SFX_KABOOM:
            psg(300, 63-t, t&3, 0);
            if (t>=63) {
                sfx_effect[ch] = SFX_NONE;
            }
            break;
        default:
            // off.
            VERA.data0 = 0;
            VERA.data0 = 0;
            VERA.data0 = 0;
            VERA.data0 = 0;
            break;
        }
    }
}

#include "../spr_common_inc.h"

void plat_hzapper_render(int16_t x, int16_t y, uint8_t state)
{
    switch(state) {
        case ZAPPER_OFF:
            sprout16(x, y, SPR16_HZAPPER);
            break;
        case ZAPPER_WARMING_UP:
            sprout16(x, y, SPR16_HZAPPER_ON);
            //if (tick & 0x01) {
            //    hline_noclip(0, SCREEN_W, (y >> FX) + 8, 3);
            //}
            break;
        case ZAPPER_ON:
            //hline_noclip(0, SCREEN_W, (y >> FX) + 8, 15);
            {
                int16_t cy = (( y >> FX) + 8) / 8;
                hline_chars_noclip(0, SCREEN_W / 8, cy,
                    2 + ((y >> FX) & 0x07), 15);
                sprout16(x, y, SPR16_HZAPPER_ON);
            }
            break;
    }
}

void plat_vzapper_render(int16_t x, int16_t y, uint8_t state)
{
    switch(state) {
        case ZAPPER_OFF:
            sprout16(x, y, SPR16_VZAPPER);
            break;
        case ZAPPER_WARMING_UP:
            sprout16(x, y, SPR16_VZAPPER_ON);
            //if (tick & 0x01) {
            //    vline_noclip((x>>FX)+8, 0, SCREEN_H, 3);
            //}
            break;
        case ZAPPER_ON:
            //vline_noclip((x>>FX)+8, 0, SCREEN_H, 15);
            {
                int16_t cx = ((x >> FX) + 8 ) / 8;
                vline_chars_noclip(cx, 0, SCREEN_H / 8,
                    10 + ((x >> FX) & 0x07), 15);
                sprout16(x, y, SPR16_VZAPPER_ON);
            }
            break;
    }
}


static inline bool inp_keypressed(uint8_t key) {
    return inp_keystates[key / 8] & (1 << (key & 0x07));
}

// some keycodes for r43+. (previous roms used ps/2 multibye sequences) 
// see https://github.com/X16Community/x16-rom/blob/master/inc/keycode.inc
#define KEYCODE_W 0x12
#define KEYCODE_A 0x1F
#define KEYCODE_S 0x20
#define KEYCODE_D 0x21
#define KEYCODE_ENTER 0x2B
#define KEYCODE_LEFTARROW 0x4F
#define KEYCODE_UPARROW 0x53
#define KEYCODE_DOWNARROW 0x54
#define KEYCODE_RIGHTARROW 0x59
#define KEYCODE_ESC 0x6E
#define KEYCODE_BACKSPACE 0x0F
#define KEYCODE_HOME 0x50
#define KEYCODE_END 0x50

#define KEYCODE_F1 0x70
#define KEYCODE_F2 0x71
#define KEYCODE_F3 0x72
#define KEYCODE_F4 0x73
#define KEYCODE_F5 0x74


static void update_inp_mouse()
{
    mouse_pos_t m;
    uint8_t mb = cx16_k_mouse_get(&m);
    int16_t mx = m.x << FX;
    int16_t my = m.y << FX;

    if (mb != 0 || mx != plat_mouse_x || my != plat_mouse_y) {
        plat_mouse_buttons = mb;
        plat_mouse_x = mx;
        plat_mouse_y = my;
        mouse_watchdog = 60;
    } else {
       if (mouse_watchdog > 0) {
           --mouse_watchdog;
       }
    }
}


uint8_t plat_raw_dualstick()
{
    uint8_t raw = 0;

    // Keys pretending to be a joypad.
    // (cx16 provides a key-driven joypad as joystick 0, but the mapping
    // is no good for twin-stick style controls)
    if (inp_keypressed(KEYCODE_W)) { raw |= INP_UP; }
    if (inp_keypressed(KEYCODE_A)) { raw |= INP_LEFT; }
    if (inp_keypressed(KEYCODE_S)) { raw |= INP_DOWN; }
    if (inp_keypressed(KEYCODE_D)) { raw |= INP_RIGHT; }
    if (inp_keypressed(KEYCODE_LEFTARROW)) { raw |= INP_FIRE_LEFT; }
    if (inp_keypressed(KEYCODE_UPARROW)) { raw |= INP_FIRE_UP; }
    if (inp_keypressed(KEYCODE_DOWNARROW)) { raw |= INP_FIRE_DOWN; }
    if (inp_keypressed(KEYCODE_RIGHTARROW)) { raw |= INP_FIRE_RIGHT; }

    // Read the first _real_ joypad, if plugged in
    uint16_t j1 = (uint16_t)cx16_k_joystick_get(1);
    if (!(j1 & JOY_UP_MASK)) {raw |= INP_UP;}
    if (!(j1 & JOY_DOWN_MASK)) {raw |= INP_DOWN;}
    if (!(j1 & JOY_LEFT_MASK)) {raw |= INP_LEFT;}
    if (!(j1 & JOY_RIGHT_MASK)) {raw |= INP_RIGHT;}
    if (!(j1 & 0x4000)) {raw |= INP_FIRE_UP;}     // X
    if (!(j1 & 0x0080)) {raw |= INP_FIRE_DOWN;}   //B
    if (!(j1 & 0x0040)) {raw |= INP_FIRE_LEFT;}   //Y
    if (!(j1 & 0x8000)) {raw |= INP_FIRE_RIGHT;}  //A
    return raw;
}

uint8_t plat_raw_menukeys()
{
    uint8_t state = 0;

    if (inp_keypressed(KEYCODE_UPARROW)) { state |= INP_UP; }
    if (inp_keypressed(KEYCODE_LEFTARROW)) { state |= INP_LEFT; }
    if (inp_keypressed(KEYCODE_DOWNARROW)) { state |= INP_DOWN; }
    if (inp_keypressed(KEYCODE_RIGHTARROW)) { state |= INP_RIGHT; }
    if (inp_keypressed(KEYCODE_ENTER)) { state |= INP_MENU_START; }

    long j1 = (uint16_t)cx16_k_joystick_get(1);
    if (!(j1 & JOY_UP_MASK)) {state |= INP_UP;}
    if (!(j1 & JOY_DOWN_MASK)) {state |= INP_DOWN;}
    if (!(j1 & JOY_LEFT_MASK)) {state |= INP_LEFT;}
    if (!(j1 & JOY_RIGHT_MASK)) {state |= INP_RIGHT;}
    //if (!(j1 & 0x4000)) {state |= ???;}     // X
    //if (!(j1 & 0x0080)) {state |= ???;}   //B
    //if (!(j1 & 0x0040)) {state |= ???;}   //Y
    if (!(j1 & 0x8000)) {state |= INP_MENU_A;}  // A
    if (!(j1 & 0x0080)) {state |= INP_MENU_B;}  // A
    if (!(j1 & 0x0010)) {state |= INP_MENU_START;}  // START
    
    return state;
}

uint8_t plat_raw_cheatkeys()
{
    uint8_t state = 0;
    if (inp_keypressed(KEYCODE_F1)) { state |= INP_CHEAT_POWERUP; }
    if (inp_keypressed(KEYCODE_F2)) { state |= INP_CHEAT_EXTRALIFE; }
    if (inp_keypressed(KEYCODE_F3)) { state |= INP_CHEAT_NEXTLEVEL; }
    return state;
}


// dump out all the joystick data as raw hex
void debug_gamepad()
{
    return;
    for (uint8_t n = 0; n <= 4; ++n) {
        uint32_t j;
        //if (n == 5) {
        //   j = (uint32_t)inp_virtpad;   // keyboard, from irq.s
        //} else {
           j  = cx16_k_joystick_get(n);
        //}
        // $YYYYXXAA
        char buf[9];
        for(uint8_t i = 0; i < 8; ++i) {
            buf[8-i-1] = hexdigits[j & 0xf];
            j >>= 4;
        }
        plat_textn(0,15+n,buf,8,2);
    }

    // keystates
    {
        char buf[32];
        for (uint8_t n = 0; n < 16; ++n) {
            uint8_t b = inp_keystates[n];
            buf[n*2] = hexdigits[b>>4];
            buf[(n*2)+1] = hexdigits[b & 0x0f];
        }
        plat_textn(0,13,buf,32,2);
    }
}



void debug_getin()
{

    static char buf[40] = {0};
    static uint8_t n = 0;

    while (1) {
        char c = plat_textentry_getchar();
        if (c == 0) {
            break;
        }
        if (c==0x0a) {
            c = 'X';
        }
        if (c==0x7f) {
            // del
            if (n>0) {
                --n;
                buf[n] = 0;
            }
            continue;
        }
        if (n<40) {
            buf[n++] = c;
        }
    }
    plat_textn(0,23,buf,n,2);
}


#ifdef PLAT_HAS_TEXTENTRY

void plat_textentry_start()
{
    inp_enabletextentry();
}

void plat_textentry_stop()
{
    inp_disabletextentry();
}

char plat_textentry_getchar()
{
    char c = cbm_k_getin();
    if (c==0) {
        return 0;
    }
    // map keys DEL to ASCII DEL
    if (c == 0x14) {
        return 0x7F;
    }
    // map LEFT to ASCII non-destructive backspace
    if (c == 0x9D ) {
        return '\b';
    }

    // map CR to LF
    if (c==0x0D) {
        return '\n';
    }
    // suppress other control codes.
    if (c < 32 || (c >= 0x80 && c < 0xa0)) {
        return 0;
    }
    return c;
}

#endif // PLAT_HAS_TEXTENTRY
 
int main(void) {

    plat_init();
    game_init();
    while(1) {
        waitvbl();
        plat_render_start();
        game_render();
        if (mouse_watchdog > 0) {
            sprout16(plat_mouse_x, plat_mouse_y, 0);
        }

        debug_gamepad();
        //debug_getin();
        plat_render_finish();
        sfx_tick();
        //cx16_k_joystick_scan();
        update_inp_mouse();
        game_tick();
    }
}

