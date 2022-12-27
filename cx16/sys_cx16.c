#include <cx16.h>
#include <stdint.h>

#include "sys.h"

// Platform-specifics for cx16



// irq.s, glue.s
extern void inp_tick();

// irq.s
extern void irq_init();
extern void keyhandler_init();
extern uint16_t inp_virtpad;

//  .A, byte 0:      | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
//              SNES | B | Y |SEL|STA|UP |DN |LT |RT |
//
//  .X, byte 1:      | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
//              SNES | A | X | L | R | 1 | 1 | 1 | 1 |
extern volatile uint16_t inp_joystate;
uint8_t* cx16_k_memory_decompress(uint8_t* src, uint8_t* dest);

#define SPR16_SIZE (8*16)   // 16x16, 4 bpp
#define SPR32_SIZE (16*32)   // 32x32, 4 bpp

//extern uint8_t palette[16*2];   // just 16 colours
//extern uint8_t sprites16[64*SPR16_SIZE];   // 64 16x16 sprites
//extern uint8_t sprites32[8*SPR32_SIZE];   // 16 32x32 sprites
extern unsigned char export_palette_zbin[];
extern unsigned int export_palette_zbin_len;
extern unsigned char export_spr16_zbin[];
extern unsigned int export_spr16_zbin_len;
extern unsigned char export_spr32_zbin[];
extern unsigned int export_spr32_zbin_len;

// Our VERA memory map
#define VRAM_SPRITES16 0x10000
#define VRAM_SPRITES32 (VRAM_SPRITES16 + (64 * SPR16_SIZE))
#define VRAM_LAYER1_MAP 0x1b000
#define VRAM_LAYER1_TILES 0x1F000
#define VRAM_LAYER0_MAP  0x00000    //64x64
#define VRAM_LAYER0_TILES  0x08000

// hardwired:
#define VRAM_PALETTE 0x1FA00
#define VRAM_SPRITE_ATTRS 0x1FC00
#define VRAM_PSG 0x1F9C0

#define MAX_EFFECTS 8
static uint8_t ekind[MAX_EFFECTS];
static uint8_t etimer[MAX_EFFECTS];
static uint8_t ex[MAX_EFFECTS];
static uint8_t ey[MAX_EFFECTS];

static void rendereffects();

// Number of sprites used so far in current frame
static uint8_t sprcnt;
// ...and previous frame.
static uint8_t sprcntprev;

static void sfx_init();
static void sfx_tick();

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

void testum() {
    uint16_t b;
    uint8_t i;

    VERA.control = 0x00;
    // Writing to 0x1b000 onward.
    VERA.address = 0xB000;
    VERA.address_hi = 0x11; // increment = 1
    b = inp_joystate;
    for (i=0; i<16; ++i) {
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
    // cycle colour 15
    uint8_t i = (((tick >> 1)) & 0x7) + 2;
    veraaddr0(VRAM_PALETTE + (i*2), VERA_INC_1);
    veraaddr1(VRAM_PALETTE + (15*2), VERA_INC_1);
    // 2 bytes for colour entry
    VERA.data1 = VERA.data0;
    VERA.data1 = VERA.data0;

    rendereffects();

    // Set up for writing sprites.
    // Use vera channel 1 exclusively for writing sprite attrs,
    sprcnt = 0;
    veraaddr1(VRAM_SPRITE_ATTRS, VERA_INC_1);
}

void sys_render_finish()
{
    uint8_t cnt;
    // clear sprites which were used last frame, but not in this one.
    for(cnt = sprcnt; cnt<sprcntprev; ++cnt) {
        uint8_t i;
        for (i=0; i<8; ++i) {
            VERA.data1 = 0x00;
        }
    }
    sprcntprev = sprcnt;


    sfx_tick();
    //testum();
}

void sprout16(int16_t x, int16_t y, uint8_t img ) {
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
    ++sprcnt;
}

void sprout16_highlight(int16_t x, int16_t y, uint8_t img ) {
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
    ++sprcnt;
}

void sprout32(int16_t x, int16_t y, uint8_t img ) {
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
    ++sprcnt;
}

const uint8_t shot_spr[16] = {
    0,              // 0000
    SPR16_SHOT+2,   // 0001 DIR_RIGHT
    SPR16_SHOT+2,   // 0010 DIR_LEFT
    0,              // 0011
    SPR16_SHOT+0,   // 0100 DIR_DOWN
    SPR16_SHOT+3,   // 0101 down+right           
    SPR16_SHOT+1,   // 0110 down+left           
    0,              // 0111

    SPR16_SHOT+0,   // 1000 up
    SPR16_SHOT+1,   // 1001 up+right
    SPR16_SHOT+3,   // 1010 up+left
    0,              // 1011
    0,              // 1100 up+down
    0,              // 1101 up+down+right           
    0,              // 1110 up+down+left           
    0,              // 1111
};


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

    // effects layer
    veraaddr0(VRAM_LAYER0_MAP, VERA_INC_1);
    for (y = 0; y < 64; ++y) {
        uint8_t x;
        for (x = 0; x < 64; ++x) {
            VERA.data0 = 1; //(x&1) ^ (y&1);
            VERA.data0 = 0; //x & 15;
        }
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
    VERA.layer0.mapbase = (VRAM_LAYER0_MAP>>9);
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
        // 64 16x16 images
        const volatile uint8_t* src = BANK_RAM;
        int i;
        cx16_k_memory_decompress(export_spr16_zbin, (uint8_t*)BANK_RAM);
        veraaddr0(VRAM_SPRITES16, VERA_INC_1);
        for (i = 0; i < 64 * SPR16_SIZE; ++i) {
            VERA.data0 = *src++;
        }
    }
    {
        // Uncompress 8 32x32 sprites
        const volatile uint8_t* src = BANK_RAM;
        int i;
        cx16_k_memory_decompress(export_spr32_zbin, (uint8_t*)BANK_RAM);
        veraaddr0(VRAM_SPRITES32, VERA_INC_1);
        // 8 32x32 images
        for (i = 0; i < 8 * SPR32_SIZE; ++i) {
            VERA.data0 = *src++;
        }
    }

    // stand-in images for effect layers
    {
        uint8_t i;
        veraaddr0(VRAM_LAYER0_TILES, VERA_INC_1);
        for (i=0; i<8; ++i) {
            VERA.data0 = 0;
        }
        for (i=0; i<4; ++i) {
            VERA.data0 = 0b10101010;
            VERA.data0 = 0b01010101;
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
        sprcnt = 0;
        sprcntprev = 0;
    }

    // clear effect table
    {
        uint8_t i;
        for(i=0; i<MAX_EFFECTS; ++i) {
            ekind[i] = EK_NONE;
        }
    }

    // clear the tile layers
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

void sys_text(uint8_t cx, uint8_t cy, const char* txt, uint8_t colour)
{
    const char* p = txt;
    veraaddr0(VRAM_LAYER1_MAP + cy*64*2 + cx*2, VERA_INC_1);
    while(*p) {
        VERA.data0 = screencode(*p);
        VERA.data0 = colour;
        ++p;
    }
}


// double dabble: 8 decimal digits in 32 bits BCD
// from https://stackoverflow.com/a/64955167
uint32_t bin2bcd(uint32_t in) {
  uint32_t bit = 0x4000000; //  99999999 max binary
  uint32_t bcd = 0;
  uint32_t carry = 0;
  if (!in) return 0;
  while (!(in & bit)) bit >>= 1;  // skip to MSB

  while (1) {
    bcd <<= 1;
    bcd += carry; // carry 6s to next BCD digits (10 + 6 = 0x10 = LSB of next BCD digit)
    if (bit & in) bcd |= 1;
    if (!(bit >>= 1)) return bcd;
    carry = ((bcd + 0x33333333) & 0x88888888) >> 1; // carrys: 8s -> 4s
    carry += carry >> 1; // carrys 6s  
  }
}

static const char* hexdigits = "0123456789ABCDEF";
static void hex8(uint8_t v, char *dest) {
    dest[0] = hexdigits[v >> 4];
    dest[1] = hexdigits[v & 0xf];
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
        for(i=0; i<7 && i<lives; ++i) {
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
    return VRAM_LAYER0_MAP + cy*64*2 + cx*2;
}


static void drawbox(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t ch, uint8_t colour)
{
    uint8_t w = (x1 - x0) + 1;
    uint8_t h = (y1 - y0) + 1;
    uint8_t i;

    // top
    veraaddr0(layer0addr(x0,y0), VERA_INC_2);
    for (i = w; i; --i) {
        VERA.data0 = ch;
    }
    veraaddr0(layer0addr(x0,y0)+1, VERA_INC_2);
    for (i = w; i; --i) {
        VERA.data0 = colour;
    }

    if (h > 1) {
        // bottom
        veraaddr0(layer0addr(x0,y1), VERA_INC_2);
        for (i = w; i; --i) {
            VERA.data0 = ch;
        }
        veraaddr0(layer0addr(x0,y1)+1, VERA_INC_2);
        for (i = w; i; --i) {
            VERA.data0 = colour;
        }
    }

    // Don't need to draw top or bottom chars.
    if (h < 2) {
        return;
    }
    h -= 2;
    ++y0;

    // left side
    veraaddr0(layer0addr(x0,y0), VERA_INC_128);
    for (i = h; i; --i) {
        VERA.data0 = ch;
    }
    veraaddr0(layer0addr(x0,y0)+1, VERA_INC_128);
    for (i = h; i; --i) {
        VERA.data0 = colour;
    }
    if (w > 1) {
        // right side (but not top or bottom char)
        veraaddr0(layer0addr(x1,y0), VERA_INC_128);
        for (i = h; i; --i) {
            VERA.data0 = ch;
        }
        veraaddr0(layer0addr(x1,y0)+1, VERA_INC_128);
        for (i = h; i; --i) {
            VERA.data0 = colour;
        }
    }

}

static void clrbox(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    uint8_t w = (x1 - x0) + 1;
    uint8_t h = (y1 - y0) + 1;
    uint8_t i;

    // top
    veraaddr0(layer0addr(x0,y0), VERA_INC_2);
    for (i = w; i; --i) {
        VERA.data0 = 0;
    }

    if (h > 1) {
        // bottom
        veraaddr0(layer0addr(x0,y1), VERA_INC_2);
        for (i = w; i; --i) {
            VERA.data0 = 0;
        }
    }

    // Don't need to draw top or bottom chars.
    if (h < 2) {
        return;
    }
    h -= 2;
    ++y0;

    // left side
    veraaddr0(layer0addr(x0,y0), VERA_INC_128);
    for (i = h; i; --i) {
        VERA.data0 = 0;
    }
    if (w > 1) {
        // right side (but not top or bottom char)
        veraaddr0(layer0addr(x1,y0), VERA_INC_128);
        for (i = h; i; --i) {
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

    ex[e] = (((x >> FX) + 4) / 8) + 12;
    ey[e] = (((y >> FX) + 12) / 8) + 12;
    ekind[e] = kind;
    etimer[e] = 0;
}



static void do_kaboomeffect(uint8_t e) {
    uint8_t t = etimer[e];
    uint8_t cx = ex[e];
    uint8_t cy = ey[e];
    if(t<15) {
        drawbox(cx-t, cy-t, cx+t, cy+t, 1, t);
    }
    if(t>0) {
        --t;
        clrbox(cx-t, cy-t, cx+t, cy+t);
    }
    if(++etimer[e] == 16) {
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
            //do_spawneffect(e);
            do_kaboomeffect(e);
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

