#include <stdint.h>
#include <stdbool.h>
#include "../plat.h"
#include "../misc.h"
#include "../gob.h" // for ZAPPER_*
#include "../sfx.h" // for ZAPPER_*

extern const unsigned char export_palette_bin[];
extern const unsigned int export_palette_bin_len;
extern const unsigned char export_chars_bin[];
extern const unsigned int export_chars_bin_len;
extern const unsigned char export_spr16_bin[];
extern const unsigned int export_spr16_bin_len;
extern const unsigned char export_spr32_bin[];
extern const unsigned int export_spr32_bin_len;
extern const unsigned char export_spr64x8_bin[];
extern const unsigned int export_spr64x8_bin_len;

// Counts for exported sprite images
// each 64x8 is really a pair of 32x8 sprites
#define SPR16_NUM 128
#define SPR32_NUM 3
#define SPR64x8_NUM 4

// bytesize of sprite data
#define SPR16_SIZE (8*16)
#define SPR32_SIZE (16*32)
#define SPR64x8_SIZE (32*8)

// start of sprite data, in tile indexes
#define SPR16_TILEBASE (VRAM_SPR16/32)
#define SPR32_TILEBASE (VRAM_SPR32/32)
#define SPR64x8_TILEBASE (VRAM_SPR64x8/32)



//
volatile uint8_t tick;
uint16_t screen_h;  // varies according to pal/ntsc
bool pal_mode;
static uint16_t sprites[80*4];  // sprite attribute table
int nsprites;   // number of sprites in use

// Offscreen buffer in main ram, DMAed into vram each vblank.
static uint16_t screen[64*32];

// main palette colours (cooked down from exported rgba)
static uint16_t palette[16];

// VDP defs
static volatile uint16_t* const vdp_data_port = (uint16_t*) 0xC00000;
static volatile uint16_t* const vdp_ctrl_port = (uint16_t*) 0xC00004;
static volatile uint32_t* const vdp_ctrl_wide = (uint32_t*) 0xC00004;



// Our vram memory map
// using multiple charsets in order to get 16 colours (combined with 3 palettes)
#define VRAM_CHARSET0    0x0000      // 128 tiles for charset
#define VRAM_CHARSET1    0x1000      // 128 tiles for charset
#define VRAM_CHARSET2    0x2000      // 128 tiles for charset
#define VRAM_CHARSET3    0x3000      // 128 tiles for charset
#define VRAM_CHARSET4    0x4000      // 128 tiles for charset
#define VRAM_SPR16      0x8000      // 128 16x16 sprites
#define VRAM_SPR32      (VRAM_SPR16 + (SPR16_NUM * SPR16_SIZE))
#define VRAM_SPR64x8    (VRAM_SPR32 + (SPR32_NUM * SPR32_SIZE))

// Not sure you can turn off the planes, so for now they can all point
// to the same memory :-)
#define VRAM_WINDOW     0xE000      // tilemap for window
#define VRAM_SCROLL_A   0xE000      // tilemap for plane A
#define VRAM_SCROLL_B   0xE000      // tilemap for plane B

#define VRAM_SPRITE_ATTRS    0xF800 // 64 * ???
#define VRAM_HSCROLL_TABLE   0xFC00

#define CHARBASE0 (VRAM_CHARSET0/32)
#define CHARBASE1 (VRAM_CHARSET1/32)
#define CHARBASE2 (VRAM_CHARSET2/32)
#define CHARBASE3 (VRAM_CHARSET3/32)
#define CHARBASE4 (VRAM_CHARSET4/32)
#define SCROLL_A_W 64
#define SCROLL_A_H 32


static void load_charset(uint16_t dest, uint8_t colour);
static void megadrive_init();
static inline uint16_t tileattr(uint8_t chr, uint8_t colour);

// DMA stuff


#define ADDR_VRAM_WRITE 0b000001
#define ADDR_CRAM_WRITE 0b000011
#define ADDR_VSRAM_WRITE 0b000101

#define VDP_DMA_VRAM_ADDR(adr)      (((0x4000 + ((adr) & 0x3FFF)) << 16) + (((adr) >> 14) | 0x80))

// calculate longword used for setting vdp addresses
static inline uint32_t vdpaddr(uint8_t code, uint16_t addr) {
    // CD1 CD0 A13 A12 A11 A10 A09 A08     (D31-D24)
    // A07 A06 A05 A04 A03 A02 A01 A00     (D23-D16)
    //  ?   ?   ?   ?   ?   ?   ?   ?      (D15-D8)
    // CD5 CD4 CD3 CD2  ?   ?  A15 A14     (D7-D0)
    //
    // CDx = VDP code (0-3F)
    // Axx = VDP address (00-FFFF)
    uint32_t l = (code & 0x03) << 30;
    l |= (addr & 0x3FFF) << 16;
    l |= (code & 0x3c) << 2;
    l |= (addr >> 14);
    return l;
}


// $13,$14 - DMA Length (in 16bit words)
// $15,$16,$17 - DMA 68000 src addr
static void dma_do(uint32_t from, uint16_t len, uint32_t cmd) {
	// Setup DMA length (in word here)
    *vdp_ctrl_port = 0x9300 + (len & 0xff);
    *vdp_ctrl_port = 0x9400 + ((len >> 8) & 0xff);
    // Setup DMA address
    from >>= 1;
    *vdp_ctrl_port = 0x9500 + (from & 0xff);
    from >>= 8;
    *vdp_ctrl_port = 0x9600 + (from & 0xff);
    from >>= 8;
    *vdp_ctrl_port = 0x9700 + (from & 0x7f);
    // Enable DMA transfer
    *vdp_ctrl_wide = cmd;
}

// len in words
void vdp_dma_vram(uint32_t from, uint16_t to, uint16_t len) {
	dma_do(from, len, ((0x4000 + (((uint32_t)to) & 0x3FFF)) << 16) + ((((uint32_t)to) >> 14) | 0x80));
}

// len in words
void vdp_dma_cram(uint32_t from, uint16_t to, uint16_t len) {
	dma_do(from, len, ((0xC000 + (((uint32_t)to) & 0x3FFF)) << 16) + ((((uint32_t)to) >> 14) | 0x80));
}

// len in words
void vdp_dma_vsram(uint32_t from, uint16_t to, uint16_t len) {
	dma_do(from, len, ((0x4000 + (((uint32_t)to) & 0x3FFF)) << 16) + ((((uint32_t)to) >> 14) | 0x90));
}

// error handler, called from boot.s
void _error()
{
    while(1) {
    }
}

void waitvbl() {
    while(!(*vdp_ctrl_port & 8)) {};
	//while(*vdp_ctrl_port & 8) {};
    ++tick;
}

// palette format:
//----bbb-ggg-rrr-
static void vdp_color(uint16_t index, uint16_t color) {
    *vdp_ctrl_wide = vdpaddr(ADDR_CRAM_WRITE, index << 1);
//    *vdp_ctrl_wide = ((0xC000 + (((uint32_t)index) & 0x3FFF)) << 16) + ((((uint32_t)index) >> 14) | 0x00);
    *vdp_data_port = color;
}

#define TILE_ATTR(pal, prio, flipV, flipH, index)                               \
	((((uint16_t)flipH) << 11) | (((uint16_t)flipV) << 12) |                    \
	(((uint16_t)pal) << 13) | (((uint16_t)prio) << 15) | ((uint16_t)index))


void dbug()
{
    uint8_t n = nsprites;
    char buf[2];
    buf[0] = hexdigits[(n >> 4) & 0x0F];
    buf[1] = hexdigits[(n >> 0) & 0x0F];
    plat_textn(0,0,buf,2,1);
}


// clear our offscreen buffer
static void clearscreen()
{
    uint16_t attr = TILE_ATTR(0,1,0,0,CHARBASE0 + 0);
    // two chars per uint32_t
    uint32_t aa = (attr<<16)|attr;
    for (int y=0; y<(SCREEN_H/8); ++y) {
        // 8 chars at a time
        uint32_t *p = (uint32_t*)(screen + y*SCROLL_A_W);
        for (int x=0; x<((SCREEN_W/8)/(2*4)); ++x) {
            *p++ = aa;
            *p++ = aa;
            *p++ = aa;
            *p++ = aa;
        }

    }

}


void main()
{
    megadrive_init();
    game_init();
    while(1) {
        //vdp_color(0,0x0000);
        dbug();
        //vdp_color(0,0x000e);
        game_tick();

        // start render
        //vdp_color(0,0x0002);
        {
            nsprites = 0;
        }

        clearscreen();
        // Copy the screen to VRAM
        //vdp_color(0,0x0ee0);
        game_render();
        //vdp_color(0,0x0000);

        waitvbl();

        // end of rendering - do any dma while we're in the vblank!
        {
            //vdp_color(0,0x00e0);
            // terminate the sprite chain
            if (nsprites == 0 ) {
                // need a dummy sprite
                sprites[0] = 0x0000;
                sprites[1] = 0x0000;
                sprites[2] = 0x0000;
                sprites[3] = 0x0000;
                ++nsprites;
            }
            // copy sprite table into vram
            vdp_dma_vram((uint32_t)sprites, VRAM_SPRITE_ATTRS, (nsprites*8)/2);
            // copy screen to vram
            vdp_dma_vram((uint32_t)screen, VRAM_SCROLL_A, SCROLL_A_W * SCROLL_A_H);
    
            // cycle colour 15 (palette 0) 
            uint16_t i = (((tick >> 1)) & 0x7) + 2;
            vdp_color(15,palette[i]);
        }
        //vdp_color(0,0x0002);

    }
}

static void megadrive_init()
{
    nsprites = 0;
    tick = 0;
	// Store pal_mode and adjust some stuff based on it
    pal_mode = *vdp_ctrl_port & 1;
    screen_h = pal_mode ? 240 : 224;

    // $00 - Mode Set Register No. 1
	*vdp_ctrl_port = 0x8004;    // enable 9-bit colours
    
    // $01 - Mode Set Register No. 2
	*vdp_ctrl_port = 0x8174 | (pal_mode ? 8 : 0);

    // $02 - Pattern Name Table Address for Scroll A
    // --aaa--- a = a15-a13 of plane A address
    *vdp_ctrl_port = 0x8200 | (VRAM_SCROLL_A >> 10);

    // $03 - Pattern Name Table Address for Window
    // --aaaaa- a = a15-a11 of window address
    *vdp_ctrl_port = 0x8300 | (VRAM_WINDOW >> 10);

    // $04 - Pattern Name Table Address for Scroll B
    // -----aaa a = A15-A11 of plane B address
	*vdp_ctrl_port = 0x8400 | (VRAM_SCROLL_B >> 13);

    // $05 - Sprite Attribute Table Base Address
    // -aaaaaaa a = A15-A09 of sprite attr table
	*vdp_ctrl_port = 0x8500 | (VRAM_SPRITE_ATTRS >> 9);

    // $06 - ????
	*vdp_ctrl_port = 0x8600;

    // $07 - Backdrop Color
	*vdp_ctrl_port = 0x8700;

    // $08 - ????
	*vdp_ctrl_port = 0x8800;

    // $09 - ????
	*vdp_ctrl_port = 0x8900;

    // $0A - H Interrupt Register
    //
	*vdp_ctrl_port = 0x8A01;

    // $0B - Mode Set Register No. 3
	*vdp_ctrl_port = 0x8B00;

    // $0C - Mode Set Register No. 4
    // - interlace off
    // - 40 cells wide
	*vdp_ctrl_port = 0x8C81;

    // $0D - H Scroll Data Table Base Address
	*vdp_ctrl_port = 0x8D00 | (VRAM_HSCROLL_TABLE >> 10);

    // $0E - ????
	*vdp_ctrl_port = 0x8E00;

    // $0F - Auto Increment Data
	*vdp_ctrl_port = 0x8F02; // Auto increment

    // $10 - Scroll Size
    // Both planes are always same size?
    // --vv--hh
    // 00: 32
    // 01: 64
    // 10: invalid (if hh is 10, repeat first row)
    // 11: 128
	*vdp_ctrl_port = 0x9001; // Map size (64x32)

    // $11 - Window H Position
	*vdp_ctrl_port = 0x9100;

    // $12 - Window V Position
    // d--vvvvv
    // v=0 means disable window
	*vdp_ctrl_port = 0x9200;

    // $13,$14 - DMA Length
    // $15,$16,$17 - DMA 68000 src addr

    // load sprites into vram
    vdp_dma_vram((uint32_t)export_spr16_bin, VRAM_SPR16, export_spr16_bin_len/2);
    vdp_dma_vram((uint32_t)export_spr32_bin, VRAM_SPR32, export_spr32_bin_len/2);
    vdp_dma_vram((uint32_t)export_spr64x8_bin, VRAM_SPR64x8, export_spr64x8_bin_len/2);

    // On VDP, colours are baked into the character tiles which makes
    // per-character colouring tricky.
    // But we have 4 palettes to play with and enough vram to load multiple
    // copies of the characters, each of a different colour.
    // palette 0: normal palette (with cycling colour 15)
    // palette 1: extra colours for chars
    // palette 2: more extra colours for chars
    // palette 3: highlight palette - colour 0 black, all others white.
    //
    // Create our charsets, with various colours
    // Start at 15, to hit palette 0 flashing colour
    load_charset(VRAM_CHARSET0, 15);
    load_charset(VRAM_CHARSET1, 14);
    load_charset(VRAM_CHARSET2, 13);
    load_charset(VRAM_CHARSET3, 12);
    load_charset(VRAM_CHARSET4, 11);

    // convert exported rgba palette to megadrive format
    {
        const uint8_t *src = export_palette_bin;
        for (int i=0; i<16; ++i) {
                uint8_t r = src[0];
                uint8_t g = src[1];
                uint8_t b = src[2];
                src += 4;
                palette[i] = ((b>>5)<<9) | ((g>>5)<<5) | ((r>>5)<<1);
        }
    }

    // Palette 0, as exported.
    // We'll cycle colour 15, but otherwise no fancy stuff.
    {
        for (int i=0; i<16; ++i) {
            vdp_color(i,palette[i]);
        }
    }
    // Palette 1
    {
        vdp_color(16 + 11, palette[6]);
        vdp_color(16 + 12, palette[7]);
        vdp_color(16 + 13, palette[8]);
        vdp_color(16 + 14, palette[9]);
        vdp_color(16 + 15, palette[10]);
    }
    // Palette 2
    {
        vdp_color(32 + 11, palette[0]); // fudge to get black! white can use palette 3
        vdp_color(32 + 12, palette[2]);
        vdp_color(32 + 13, palette[3]);
        vdp_color(32 + 14, palette[4]);
        vdp_color(32 + 15, palette[5]);
    }
    // palette 3, for highlighting sprites
    // color 0 black (even if never seen!), all others white
    {
        vdp_color(48 + 0, palette[0]);
        for (int i = 1; i < 16; ++i) {
            vdp_color(48 + i, palette[1]);
        }
    }

    // clear our offscreen buffer in main ram
    {
        for (int i = 0; i < SCROLL_A_W * SCROLL_A_H; ++i) {
            screen[i] = TILE_ATTR(0,1,0,0,CHARBASE0 + 0);
        }
        // Clear scroll A plane.
        vdp_dma_vram((uint32_t)screen, VRAM_SCROLL_A, SCROLL_A_W * SCROLL_A_H);
        // Won't be using scroll B.
        vdp_dma_vram((uint32_t)screen, VRAM_SCROLL_B, SCROLL_A_W * SCROLL_A_H);
    }
}


// render the 1-bit charset in the given colour, loading it to vram
// starting at dest.
// We only use 128 chars in our charset to save space.
// So no accents or whatnot.
static void load_charset(uint16_t dest, uint8_t colour)
{
    uint8_t buf[4*8*128];
    const uint8_t* src = export_chars_bin; 
    uint8_t * p = buf;
    for (int i=0; i<128; ++i) {
        for (int y = 0; y < 8; ++y) {
            uint8_t row = *src++;
            for (int x = 0; x < 8; x += 2) {
                uint8_t b = 0;
                if (row & (1 << (7-x))) {
                    b |= (colour<<4);
                }
                if (row & (1 << ((7-(x+1))))) {
                    b |= (colour &0x0f);
                }
                *p++ = b;
            }
        }
    }
    vdp_dma_vram((uint32_t)buf, dest, (4*8*128)/2);
}



// Map from colour indexes to palette and charset, to allow chars of any colour.
typedef struct {
    uint16_t pal;
    uint16_t chrbase;
} chrcolour_entry;

const static chrcolour_entry chrcolour_table[16] = {
    // palette, charset base
    {2, CHARBASE4},
    {3, CHARBASE0}, // special case - use highlight palette for white
    {2, CHARBASE3},
    {2, CHARBASE2},

    {2, CHARBASE1},
    {2, CHARBASE0},
    {1, CHARBASE4},
    {1, CHARBASE3},

    {1, CHARBASE2},
    {1, CHARBASE1},
    {1, CHARBASE0},
    {0, CHARBASE4},

    {0, CHARBASE3},
    {0, CHARBASE2},
    {0, CHARBASE1},
    {0, CHARBASE0},
};

static inline uint16_t tileattr(uint8_t chr, uint8_t colour)
{
    uint8_t pal = chrcolour_table[colour].pal;
    uint16_t chrbase = chrcolour_table[colour].chrbase;
    return TILE_ATTR(pal, 1, 0, 0, chrbase + chr);
}


// Draw horizontal line of chars, range [cx_begin, cx_end).
void plat_hline_noclip(uint8_t cx_begin, uint8_t cx_end, uint8_t cy, uint8_t chr, uint8_t colour)
{
    uint16_t attr = tileattr(chr, colour);
    uint16_t *p = &screen[cy*SCROLL_A_W + cx_begin];
    for (int x=0; x<cx_end-cx_begin; ++x) {
        *p++ = attr;
    }
}

// Draw vertical line of chars, range [cy_begin, cy_end).
void plat_vline_noclip(uint8_t cx, uint8_t cy_begin, uint8_t cy_end, uint8_t chr, uint8_t colour)
{
    uint16_t attr = tileattr(chr, colour);
    uint16_t *p = &screen[cy_begin*SCROLL_A_W + cx];
    for (int y=0; y<cy_end-cy_begin; ++y) {
        *p = attr;
        p += SCROLL_A_W;
    }
}

// cw must be even!
void plat_mono4x2(uint8_t cx, int8_t cy, const uint8_t* src, uint8_t cw, uint8_t ch, uint8_t basecol)
{
    int y;
    for (y=0; y < ch; ++y) {
        uint8_t colour = (basecol + y) & 0x0f;
        uint16_t* dest = &screen[(cy+y)*SCROLL_A_W + cx];
        int x;
        for (x=0; x < cw; x += 2) {
            // left char
            uint8_t chr = DRAWCHR_2x2 + (*src >> 4);
            *dest++ = tileattr(chr, colour);
            // right char
            chr = DRAWCHR_2x2 + (*src & 0x0f);
            *dest++ = tileattr(chr, colour);
            ++src;
        }
    }
}


// Not used. We clear everything every frame anyway.
void plat_clr()
{
}

// draw len number of chars
void plat_textn(uint8_t cx, uint8_t cy, const char* txt, uint8_t len, uint8_t colour)
{
    // draw into our offscreen array in main ram
    uint16_t *dest = &screen[cy*SCROLL_A_W + cx];
    for (int i=0; i<len; ++i) {
        uint8_t chr = *txt++;
	    *dest++ = tileattr(chr, colour);
	}
}


void plat_hud(uint8_t level, uint8_t lives, uint32_t score)
{
    const uint8_t cx = 0;
    const uint8_t cy = 0;
    uint8_t c = 1;
    uint16_t *dest = &screen[cy*SCROLL_A_W + cx];

    // Level
    {
        uint8_t bcd = bin2bcd8(level);
        *dest++ = tileattr('L', c);
        *dest++ = tileattr('V', c);
        ++dest;
        *dest++ = tileattr(hexdigits[bcd >> 4], c);
        *dest++ = tileattr(hexdigits[bcd & 0x0f], c);
        ++dest;
        ++dest;
    }

    // Score
    {
        uint32_t bcd = bin2bcd32(score);
        *dest++ = tileattr(hexdigits[(bcd >> 28)&0x0f], c);
        *dest++ = tileattr(hexdigits[(bcd >> 24)&0x0f], c);
        *dest++ = tileattr(hexdigits[(bcd >> 20)&0x0f], c);
        *dest++ = tileattr(hexdigits[(bcd >> 16)&0x0f], c);
        *dest++ = tileattr(hexdigits[(bcd >> 12)&0x0f], c);
        *dest++ = tileattr(hexdigits[(bcd >> 8)&0x0f], c);
        *dest++ = tileattr(hexdigits[(bcd >> 4)&0x0f], c);
        *dest++ = tileattr(hexdigits[(bcd >> 0)&0x0f], c);
        ++dest;
        ++dest;
    }


    // Lives
    {
        c = 2;
        for(int i = 0; i < 7 && i < lives; ++i) {
            *dest++ = tileattr('*', c);
        }
        *dest++ = tileattr((lives>7) ? '+' : ' ', c);
    }
}


/*
 Sprite Attribute Table
 ----------------------

 All sprite data is stored in a region of VRAM called sprite attribute table.
 The table is 640 bytes in size. Each 8-byte entry has the following format:

 Index + 0  :   ------yy yyyyyyyy
 Index + 2  :   ----hhvv
 Index + 3  :   -lllllll
 Index + 4  :   pccvhnnn nnnnnnnn
 Index + 6  :   ------xx xxxxxxxx

 y = Vertical coordinate of sprite
 h = Horizontal size in cells (00b=1 cell, 11b=4 cells)
 v = Vertical size in cells (00b=1 cell, 11b=4 cells)
 l = Link field
 p = Priority
 c = Color palette
 v = Vertical flip
 h = Horizontal flip
 n = Sprite pattern start index
 x = Horizontal coordinate of sprite
*/


// size hhvv:
// 0b0101 = 16x16
// 0b1111 = 32x32   
// 0b1100 = 32x8   
static void internal_sprout(int16_t x, int16_t y, uint16_t tile, uint8_t size, uint8_t palette)
{
    if (nsprites>=80) {
        return;
    }

    // Off left or top? (32 is max sprite size)
    if (x < -(32 << FX) || y < -(32 << FX)) {
        return;
    }

    // Convert to unsigned VDP sprite coords.
    uint16_t sprx = (x + (128<<FX)) >> FX;
    uint16_t spry = (y + (128<<FX)) >> FX;

    // Off right or bottom?
    if (sprx > (SCREEN_W + 128) || spry > (SCREEN_H + 128)) {
        return;
    }

    if (nsprites > 0) {
        // patch previous sprite to continue the chain.
        uint16_t *prevspr = &sprites[(nsprites-1)*4];
        prevspr[1] = (prevspr[1] & 0xFF00) | nsprites;
    }

    uint16_t *spr = &sprites[nsprites*4];
    spr[0] = spry & 0x3FF;
    uint8_t link = 0; // link 0 => end of sprite chain
    spr[1] = (size<<8) | link;
    spr[2] = 0x8000 | (palette<<13) | tile;
    spr[3] = sprx & 0x3FF;
    ++nsprites;
}


void sprout16(int16_t x, int16_t y, uint8_t img)
{
    internal_sprout(x, y, (SPR16_TILEBASE + img*4), 0b0101, 0);
}

void sprout16_highlight(int16_t x, int16_t y, uint8_t img)
{
    // palette 3 is all-white
    internal_sprout(x, y, (SPR16_TILEBASE + img*4), 0b0101, 3);
}

void sprout32(int16_t x, int16_t y, uint8_t img)
{
    internal_sprout(x, y, (SPR32_TILEBASE + img*16), 0b1111, 0);
}

void sprout32_highlight(int16_t x, int16_t y, uint8_t img)
{
    // palette 3 is all-white
    internal_sprout(x, y, (SPR32_TILEBASE + img*16), 0b1111, 3);
}

void sprout64x8(int16_t x, int16_t y, uint8_t img)
{
    // use two 32x8 sprites
    internal_sprout(x, y, (SPR64x8_TILEBASE + (img*2) * 4), 0b1100, 0);
    internal_sprout(x + (32 << FX), y, (SPR64x8_TILEBASE + ((img*2) + 1) * 4), 0b1100, 0);
}



void plat_hzapper_render(int16_t x, int16_t y, uint8_t state)
{
    switch(state) {
        case ZAPPER_OFF:
            sprout16(x,y, SPR16_HZAPPER);
            break;
        case ZAPPER_ON:
            {
                int16_t cy = (( y >> FX) + 8) / 8;
                plat_hline_noclip(0, SCREEN_W / 8, cy,
                    DRAWCHR_HLINE + ((y >> FX) & 0x07), 15);
            }
            // fall through
        case ZAPPER_WARMING_UP:
            sprout16(x,y, SPR16_HZAPPER_ON);
            break;
    }
}

void plat_vzapper_render(int16_t x, int16_t y, uint8_t state)
{
    switch(state) {
        case ZAPPER_OFF:
            sprout16(x,y, SPR16_VZAPPER);
            break;
        case ZAPPER_ON:
            {
                int16_t cx = ((x >> FX) + 8 ) / 8;
                plat_vline_noclip(cx, 0, SCREEN_H / 8,
                    DRAWCHR_VLINE + ((x >> FX) & 0x07), 15);
            }
            // fall through
        case ZAPPER_WARMING_UP:
            sprout16(x,y, SPR16_VZAPPER_ON);
            break;
    }
}

// start PLAT_HAS_TEXTENTRY (fns should be no-ops if not supported)
void plat_textentry_start()
{
}

void plat_textentry_stop()
{
}

// Returns printable ascii or DEL (0x7f) or LF (0x0A), or 0 for no input.
char plat_textentry_getchar()
{
    return 0;
}
// end PLAT_HAS_TEXTENTRY
 
void plat_gatso(uint8_t t)
{
}


// Unsupported
void plat_quit()
{
}

/*
 * INPUT
 */


uint8_t plat_raw_gamepad()
{
    const uint8_t port = 0;
    volatile uint8_t *ctrl = (volatile uint8_t *)(0xA10009 + (port<<1));
    volatile uint8_t *data = (volatile uint8_t *)(0xA10003 + (port<<1));

    *ctrl = 0x40;   // output on bit 6
    *data = 0x40;   // TH=1
    __asm__ volatile ("nop");
    __asm__ volatile ("nop");

    uint8_t state0 = (*data) & 0x3F;
    //state0: 0 | 0 | C | B | R | L | D | U
    *data = 0x00;   // TH=0
    __asm__ volatile ("nop");
    __asm__ volatile ("nop");
    uint8_t state1 = (*data) & 0x3F;
    // state1: 0 | 0 | S | A | 0 | 0 | 0 | 0 |

    uint8_t out = 0;
    if ((state0 & 0x01)==0) { out |= INP_UP; }
    if ((state0 & 0x02)==0) { out |= INP_DOWN; }
    if ((state0 & 0x04)==0) { out |= INP_LEFT; }
    if ((state0 & 0x08)==0) { out |= INP_RIGHT; }
    if ((state0 & 0x10)==0) { out |= INP_PAD_B; }
    // if ((state0 & 0x20)==0) { out |= INP_???; } // button C
    if ((state1 & 0x10)==0) { out |= INP_PAD_A; }
    if ((state1 & 0x20)==0) { out |= INP_PAD_START; }

    return out;
}

uint8_t plat_raw_keys()
{
    return 0;   // no keyboards here!
}

uint8_t plat_raw_cheatkeys()
{
    return 0;
}


bool plat_savescores(const void* begin, int nbytes)
{
    return false;
}

bool plat_loadscores(void* begin, int nbytes)
{
    return false;
}


