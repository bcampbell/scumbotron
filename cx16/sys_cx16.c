#include <cx16.h>

#include "../sys.h"
#include "../gob.h" // for ZAPPER_*
#include "../misc.h"

// Platform-specifics for cx16

// irq.s, glue.s
extern void irq_init();
extern void keyhandler_init();
extern uint16_t inp_virtpad;

static uint8_t inp_dualstick_state = 0;
static uint8_t inp_menu_state = 0;
static uint8_t inp_menu_pressed = 0;
static void update_inp_dualstick();
static void update_inp_menu();

extern void waitvbl();

uint8_t* cx16_k_memory_decompress(uint8_t* src, uint8_t* dest);

#define SPR16_SIZE (8*16)   // 16x16, 4 bpp
#define SPR16_NUM 64
#define SPR32_SIZE (16*32)   // 32x32, 4 bpp
#define SPR32_NUM 2
#define SPR64x8_SIZE (32*8) // 64x8, 4bpp
#define SPR64x8_NUM 4
extern unsigned char export_palette_zbin[];
extern unsigned int export_palette_zbin_len;
extern unsigned char export_spr16_zbin[];
extern unsigned int export_spr16_zbin_len;
extern unsigned char export_spr32_zbin[];
extern unsigned int export_spr32_zbin_len;
extern unsigned char export_spr64x8_zbin[];
extern unsigned int export_spr64x8_zbin_len;

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

static void drawbox(int8_t x, int8_t y, uint8_t w, uint8_t h, uint8_t ch, uint8_t colour);
static void hline_chars_noclip(uint8_t cx_begin, uint8_t cx_end, uint8_t cy, uint8_t ch, uint8_t colour);
static void vline_chars_noclip(uint8_t cx, uint8_t cy_begin, uint8_t cy_end, uint8_t ch, uint8_t colour);

static void clr_layer0();

static void sys_init();
static void sys_render_start();
static void sys_render_finish();

// which layer0 buffer we're displaying (we'll draw into the other one)
static uint8_t layer0_displaybuf;

#define MAX_EFFECTS 8
static uint8_t ekind[MAX_EFFECTS];
static uint8_t etimer[MAX_EFFECTS];
static uint8_t ex[MAX_EFFECTS];
static uint8_t ey[MAX_EFFECTS];

static void rendereffects();

// Number of free hw sprites left.
static uint8_t sprremaining;
// from previous frame
static uint8_t sprremainingprev;

static void sfx_init();
static void sfx_tick();

// set the border colour for raster-timing
void sys_gatso(uint8_t g)
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

void sys_render_start()
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

void sys_render_finish()
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
    rendereffects();
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


void sys_clr()
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


void sys_init()
{
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


    // Load compressed data into vram.
    // Use the $A000 bank ram as a decompression buffer. So the uncompressed
    // chunks need to be <8KB.
    // Would be nice to decompress directly to VERA, but see:
    // https://www.commanderx16.com/forum/index.php?/topic/4931-memory_decompress-not-working/

    {
        // The palette (actually, the compressing the palette likely adds a
        // few bytes, but simpler to handle all exported data the same).
        const volatile uint8_t* src = BANK_RAM;
        uint8_t i;
        cx16_k_memory_decompress(export_palette_zbin, (uint8_t*)BANK_RAM);
        veraaddr0(VRAM_PALETTE, VERA_INC_1);
        // just 16 colours
        for (i=0; i<16*2; ++i) {
            VERA.data0 = *src++;
        }

        // then a set of 16 whites for highlight flashing
        for (i=0; i<16; ++i) {
            VERA.data0 = 0xff;
            VERA.data0 = 0x0f;
        }
    }

    {
        // Uncompress 16x16 sprites
        const volatile uint8_t* src = BANK_RAM;
        int i;
        cx16_k_memory_decompress(export_spr16_zbin, (uint8_t*)BANK_RAM);
        veraaddr0(VRAM_SPRITES16, VERA_INC_1);
        for (i = 0; i < SPR16_NUM * SPR16_SIZE; ++i) {
            VERA.data0 = *src++;
        }
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
            VERA.data0 = 0b10101010;
            VERA.data0 = 0b01010101;
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

    // clear effect table
    {
        uint8_t i;
        for(i=0; i<MAX_EFFECTS; ++i) {
            ekind[i] = EK_NONE;
        }
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
    sys_clr();
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

void sys_textn(uint8_t cx, uint8_t cy, const char* txt, uint8_t len, uint8_t colour)
{
    veraaddr0(VRAM_LAYER1_MAP + cy*64*2 + cx*2, VERA_INC_1);
    while(len--) {
        VERA.data0 = screencode(*txt);
        VERA.data0 = colour;
        ++txt;
    }
}

void sys_text(uint8_t cx, uint8_t cy, const char* txt, uint8_t colour)
{
    uint8_t len = 0;
    while(txt[len] != '\0') {
        ++len;
    }
    sys_textn(cx, cy, txt, len, colour);
}


void sys_hud(uint8_t level, uint8_t lives, uint32_t score)
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

    VERA.data0 = screencode(hexdigits[level >> 4]);
    VERA.data0 = c;
    VERA.data0 = screencode(hexdigits[level & 0x0f]);
    VERA.data0 = c;

    VERA.data0 = screencode(' ');
    VERA.data0 = c;
    VERA.data0 = screencode(' ');
    VERA.data0 = c;

    // Score
    {
        uint32_t bcd = bin2bcd(score);
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
static void drawbox(int8_t cx, int8_t cy, uint8_t w, uint8_t h, uint8_t ch, uint8_t colour)
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

void sys_addeffect(int16_t x, int16_t y, uint8_t kind)
{
    // find free one
    uint8_t e = 0;
    while( e < MAX_EFFECTS && ekind[e]!=EK_NONE) {
        ++e;
    }
    if (e==MAX_EFFECTS) {
        return; // none free
    }

    ex[e] = (((x >> FX) + 4) / 8);
    ey[e] = (((y >> FX) + 4) / 8);
    ekind[e] = kind;
    etimer[e] = 0;
}



static void do_spawneffect(uint8_t e) {
    uint8_t t = 16-etimer[e];
    uint8_t cx = ex[e];
    uint8_t cy = ey[e];
    drawbox(cx-t, cy-t, t*2, t*2, 1, t);
    if (++etimer[e] >= 16) {
        ekind[e] = EK_NONE;
    }
}

static void do_kaboomeffect(uint8_t e) {
    uint8_t t = etimer[e];
    uint8_t cx = ex[e];
    uint8_t cy = ey[e];
    drawbox(cx-t, cy-t, t*2, t*2, 1, t);
    if (++etimer[e] >= 16) {
        ekind[e] = EK_NONE;
    }
}

static void rendereffects()
{
    uint8_t e;
    for(e = 0; e < MAX_EFFECTS; ++e) {
        if (ekind[e] == EK_NONE) {
            continue;
        }
        if (ekind[e] == EK_SPAWN) {
            do_spawneffect(e);
        }
        if (ekind[e] == EK_KABOOM) {
            do_kaboomeffect(e);
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

void sys_sfx_play(uint8_t effect)
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

void sys_hzapper_render(int16_t x, int16_t y, uint8_t state)
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

void sys_vzapper_render(int16_t x, int16_t y, uint8_t state)
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


static void update_inp_dualstick()
{
    uint8_t state = 0;
    uint8_t i;
    struct {uint16_t hw; uint8_t bitmask; } mapping[8] = {
        {JOY_UP_MASK, INP_FIRE_UP},
        {JOY_DOWN_MASK, INP_FIRE_DOWN},
        {JOY_LEFT_MASK, INP_FIRE_LEFT},
        {JOY_RIGHT_MASK, INP_FIRE_RIGHT},
        {0x4000, INP_UP},    // X
        {0x0080, INP_DOWN},  // B
        {0x0040, INP_LEFT},  // Y
        {0x8000, INP_RIGHT}, // A
    };
    for (i = 0; i < 8; ++i) {
        if ((inp_virtpad & mapping[i].hw)) {
            state |= mapping[i].bitmask;
        }
    }
    inp_dualstick_state = state;
}


static void update_inp_menu()
{
    uint8_t state = 0;
    uint8_t i;
    struct {uint16_t hw; uint8_t bitmask; } mapping[8] = {
        {JOY_UP_MASK, INP_UP},
        {JOY_DOWN_MASK, INP_DOWN},
        {JOY_LEFT_MASK, INP_LEFT},
        {JOY_RIGHT_MASK, INP_RIGHT},
        {0x8000 /*A*/, INP_MENU_ACTION},
    };
    for (i = 0; i < 8; ++i) {
        if ((inp_virtpad & mapping[i].hw)) {
            state |= mapping[i].bitmask;
        }
    }
    // Which ones were pressed since last check?
    inp_menu_pressed = (~inp_menu_state) & state;
    inp_menu_state = state;
}


uint8_t sys_inp_dualsticks()
{
    return inp_dualstick_state;
}

uint8_t sys_inp_menu()
{
    return inp_menu_pressed;
}



int main(void) {

    sys_init();
    game_init();
    while(1) {
        waitvbl();
        sys_render_start();
        game_render();
        sys_render_finish();
        sfx_tick();
        update_inp_dualstick();
        update_inp_menu();
        game_tick();
    }
}

