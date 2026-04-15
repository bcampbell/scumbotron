#include "gfx_vera.h"

#ifdef __CX16__
#include <cx16.h>
// Other platforms can sort out vera includes in plat_details.h (which
// is inlcuded by plat.h).
#endif

#include "plat.h"
#include "gob.h" // for ZAPPER_*
#include "misc.h" // for bin2bcd32(), hexdigits


extern unsigned char export_palette_bin[];
extern unsigned int export_palette_bin_len;

static void clr_layer0();

// which layer0 buffer we're displaying (we'll draw into the other one)
static uint8_t layer0_displaybuf;


// Number of free hw sprites left.
static uint8_t sprremaining;
// from previous frame
static uint8_t sprremainingprev;

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

void gfx_vera_render_start()
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

void gfx_vera_render_finish()
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
}

// Output a 16x16 sprite, using palette 0 (the normal palette).
void sprout16(int16_t x, int16_t y, uint8_t img )
{
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
    VERA.data1 = (2) << 2; // collmask(4),z(2),vflip,hflip
    // 7: hhwwpppp
    //    h: height
    //    w: width
    //    p: palette offset
    VERA.data1 = (1 << 6) | (1 << 4);  // 16x16, 0 palette offset.
}

// Output a 16x16 sprite, using palette 1 (all white).
void sprout16_highlight(int16_t x, int16_t y, uint8_t img )
{
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
    VERA.data1 = (2) << 2; // collmask(4),z(2),vflip,hflip
    // wwhhpppp
    VERA.data1 = (1 << 6) | (1 << 4) | 1;  // 16x16, palette offset 1.
}

// Output a 32x32 sprite, using palette 0 (the normal palette).
void sprout32(int16_t x, int16_t y, uint8_t img )
{
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
    VERA.data1 = (2) << 2; // collmask(4),z(2),vflip,hflip
    // wwhhpppp
    VERA.data1 = (2 << 6) | (2 << 4);  // 32x32, 0 palette offset.
}

// Output a 32x32 sprite, using palette 1 (all white).
void sprout32_highlight(int16_t x, int16_t y, uint8_t img )
{
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
    VERA.data1 = (2) << 2; // collmask(4),z(2),vflip,hflip
    // wwhhpppp
    VERA.data1 = (2 << 6) | (2 << 4) | 1;  // 32x32, palette offset 1.
}


// Output a 64x8 sprite, using palette 0 (the normal palette).
void sprout64x8(int16_t x, int16_t y, uint8_t img )
{
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
    VERA.data1 = (2) << 2; // collmask(4),z(2),vflip,hflip
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


void gfx_vera_init()
{
    // It's assumed the exported graphics data is already loaded into VRAM.

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
    VERA.display.hstart = 0;
//    VERA.display.hstart = 2;    // 2 to allow some border for gatso rastertiming
    VERA.display.hstop = 640>>2;
    VERA.display.vstart = 0;
    VERA.display.vstop = 480>>1;
    VERA.control = 0x00;    //DCSEL=0

    // text layer setup
    VERA.layer1.config = 0x10;    // 64x32 tiles, !T256C, !bitmapmode, 1bpp
    VERA.layer1.mapbase = (VRAM_LAYER1_MAP>>9);
    VERA.layer1.tilebase = ((VRAM_LAYER1_TILES)>>11)<<2 | 0<<1 | 0; // 8x8 tiles;

    // ****** WARNING WARNING WARNING ******
    // ****** WARNING WARNING WARNING ******
    // ****** WARNING WARNING WARNING ******
    //
    // unaligned 16 bit memory accesses!
    //
    // ****** WARNING WARNING WARNING ******
    // ****** WARNING WARNING WARNING ******
    // ****** WARNING WARNING WARNING ******
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
    // This is the old cx16 VRAM setup code, before I used a separate
    // loader stub. Left in for reference.
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
        veraaddr0(VRAM_LAYER0_TILES + 0*8, VERA_INC_1);
        // char 0
        for (i=0; i<8; ++i) {
            VERA.data0 = 0;
        }
        // main vfx char
        veraaddr0(VRAM_LAYER0_TILES + DRAWCHR_BLOCK*8, VERA_INC_1);
        for (i=0; i<4; ++i) {
            //VERA.data0 = 0b10101010;
            //VERA.data0 = 0b01010101;
            VERA.data0 = 0b11111111;
            VERA.data0 = 0b00000000;
        }
        // 8 vline chars
        veraaddr0(VRAM_LAYER0_TILES + DRAWCHR_VLINE*8, VERA_INC_1);
        // vline
        for (i=0; i<8; ++i) {
            uint8_t j;
            for (j=0; j<8; ++j) {
                VERA.data0 = 1<<(7-i);
            }
        }

        // 8 hline chars
        veraaddr0(VRAM_LAYER0_TILES + DRAWCHR_HLINE*8, VERA_INC_1);
        for (i=0; i<8; ++i) {
            uint8_t j;
            for (j=0; j<8; ++j) {
                VERA.data0 = (i==j) ? 0xff : 0x00;
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
void plat_vline_noclip(uint8_t cx, uint8_t cy_begin, uint8_t cy_end, uint8_t chr, uint8_t colour)
{
    uint8_t n;
    // char
    veraaddr0(layer0addr(cx,cy_begin), VERA_INC_128);
    for (n = cy_end - cy_begin; n; --n) {
        VERA.data0 = chr;
    }
    // colour
    veraaddr0(layer0addr(cx,cy_begin)+1, VERA_INC_128);
    for (n = cy_end - cy_begin; n; --n) {
        VERA.data0 = colour;
    }
}

// Draw horizontal line of chars, range [cx_begin, cx_end).
void plat_hline_noclip(uint8_t cx_begin, uint8_t cx_end, uint8_t cy, uint8_t chr, uint8_t colour)
{
    uint8_t n;
    // char
    veraaddr0(layer0addr(cx_begin,cy), VERA_INC_2);
    for (n = cx_end - cx_begin; n; --n) {
        VERA.data0 = chr;
    }
    // colour
    veraaddr0(layer0addr(cx_begin,cy)+1, VERA_INC_2);
    for (n = cx_end - cx_begin; n; --n) {
        VERA.data0 = colour;
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
                plat_hline_noclip(0, SCREEN_W / 8, cy,
                    DRAWCHR_HLINE + ((y >> FX) & 0x07), 15);
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
                plat_vline_noclip(cx, 0, SCREEN_H / 8,
                    DRAWCHR_VLINE + ((x >> FX) & 0x07), 15);
                sprout16(x, y, SPR16_VZAPPER_ON);
            }
            break;
    }
}

