//#include "md.h"

#include <stdint.h>
#include <stdbool.h>
#include "../plat.h"
#include "../misc.h"
#include "joy.h"

extern const unsigned char export_palette_bin[];
extern const unsigned int export_palette_bin_len;
extern const unsigned char export_chars_bin[];
extern const unsigned int export_chars_bin_len;
extern const unsigned char export_spr16_bin[];
extern const unsigned int export_spr16_bin_len;

const uint16_t blank[128] = {0};

//
volatile uint8_t tick;
uint8_t screen_h;
bool pal_mode;
int nsprites;

// VDP defs
static volatile uint16_t* const vdp_data_port = (uint16_t*) 0xC00000;
static volatile uint16_t* const vdp_ctrl_port = (uint16_t*) 0xC00004;
static volatile uint32_t* const vdp_ctrl_wide = (uint32_t*) 0xC00004;



// Our vram memory map
#define VRAM_CHARSET    0x0000      // 256 tiles for charset
#define VRAM_SPR16      0x2000      // 128 16x16 sprites
#define VRAM_WINDOW     0xB000      // tilemap for window
#define VRAM_SCROLL_A   0xC000      // tilemap for plane A
#define VRAM_SCROLL_B   0xE000      // tilemap for plane B

#define VRAM_SPRITE_ATTRS    0xF800 // 64 * ???
#define VRAM_HSCROLL_TABLE   0xFC00

#define CHARBASE (VRAM_CHARSET/32)
#define SCROLL_A_W 64
#define SCROLL_A_H 32

#define SPR16BASE (VRAM_SPR16/32)
// end VDP defs

static void megadrive_init();
static void megadrive_render_finish();

// DMA stuff


static inline uint32_t vdpcmd(uint8_t code, uint16_t addr) {
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
    l |= addr >> 14;
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

// end DMA stuff
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
    index <<= 1;
    *vdp_ctrl_wide = ((0xC000 + (((uint32_t)index) & 0x3FFF)) << 16) + ((((uint32_t)index) >> 14) | 0x00);
    *vdp_data_port = color;
}

#define TILE_ATTR(pal, prio, flipV, flipH, index)                               \
	((((uint16_t)flipH) << 11) | (((uint16_t)flipV) << 12) |                    \
	(((uint16_t)pal) << 13) | (((uint16_t)prio) << 15) | ((uint16_t)index))

void vdp_puts(uint16_t plan, const char *str, uint16_t x, uint16_t y) {
	uint32_t addr = plan + ((x + (y << 6)) << 1);
	*vdp_ctrl_wide = ((0x4000 + ((addr) & 0x3FFF)) << 16) + (((addr) >> 14) | 0x00);
	for(uint16_t i = 0; i < 64 && *str; ++i) {
		// Wrap around the plane, don't fall to next line
		if(i + x == 64) {
			addr -= x << 1;
			*vdp_ctrl_wide = ((0x4000 + ((addr) & 0x3FFF)) << 16) + (((addr) >> 14) | 0x00);
		}
		uint16_t attr = TILE_ATTR(0,1,0,0,CHARBASE + *str++ - 0x20);
		*vdp_data_port = attr;
	}
}

void dbug()
{
    uint8_t n = nsprites;
    char buf[2];
    buf[0] = hexdigits[(n >> 4) & 0x0F];
    buf[1] = hexdigits[(n >> 0) & 0x0F];
    plat_textn(0,0,buf,2,1);
}

void main()
{
    megadrive_init();

    game_init();
    while(1) {
        waitvbl();
        //vdp_color(0,0x0002);
        // start render
        nsprites = 0;
        //plat_clr();
        //vdp_color(0,0x0e00);
        game_render();
        //vdp_color(0,0x00e0);
        // finish render
        megadrive_render_finish();
        //vdp_color(0,0x0000);
        dbug();
        //vdp_color(0,0x000e);
        game_tick();
        joy_update();
        //vdp_color(0,0x0000);
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

    // load charset into vram
    vdp_dma_vram(export_chars_bin, VRAM_CHARSET, export_chars_bin_len/2);

    // load 16x16 sprites into vram
    vdp_dma_vram(export_spr16_bin, VRAM_SPR16, export_spr16_bin_len/2);

    // set up palette
    {
        const uint8_t *src = export_palette_bin;
        for (uint8_t i=0; i<16; ++i) {
            uint8_t r = src[0];
            uint8_t g = src[1];
            uint8_t b = src[2];
            src += 4;
            uint16_t c = ((b>>5)<<9) | ((g>>5)<<5) | ((r>>5)<<1);
            vdp_color(i,c);
        }
    }

    // Clear Scroll B (won't be using it)
    {
        uint32_t dest = VRAM_SCROLL_B;
        for (int cy = 0; cy < (SCREEN_H / 8); ++cy) {
            vdp_dma_vram(blank, dest, (SCREEN_W / 8) / 2);
            dest += 64 * 2;
        }
    }
#if 0
	// Reset the tilemaps
	vdp_map_clear(VDP_PLAN_A);
	vdp_hscroll(VDP_PLAN_A, 0);
	vdp_vscroll(VDP_PLAN_A, 0);
	vdp_map_clear(VDP_PLAN_B);
	vdp_hscroll(VDP_PLAN_B, 0);
	vdp_vscroll(VDP_PLAN_B, 0);
	// Reset sprites
	vdp_sprites_clear();
	vdp_sprites_update();
	// (Re)load the font
	vdp_font_load(FONT_TILES);
	vdp_color(1, 0x000);
	vdp_color(15, 0xEEE);
	// Put blank tile in index 0
	vdp_tiles_load(TILE_BLANK, 0, 1);
//    vdp_init();
//    enable_ints;
    
//	vdp_color(0, 0x080);
//	vdp_puts(VDP_PLAN_A, "Hello World!", 4, 4);
	
#endif
}



// text layer. This is persistent - if you draw text it'll stay onscreen until
// overwritten, or plat_clr() is called.
void plat_clr()
{
    uint32_t dest = VRAM_SCROLL_A;
    for (int cy = 0; cy < SCROLL_A_H; ++cy) {
        plat_textn(0,cy, "                    ", 20,0);
        //plat_textn(20,cy, "                    ", 20,0);
    }
}

// draw len number of chars
void plat_textn(uint8_t cx, uint8_t cy, const char* txt, uint8_t len, uint8_t colour)
{
	uint32_t dest = VRAM_SCROLL_A + ((cx + (cy * SCROLL_A_W)) << 1);
	*vdp_ctrl_wide = ((0x4000 + ((dest) & 0x3FFF)) << 16) + (((dest) >> 14) | 0x00);
    colour = 0;
    while (len > 0) {
        uint8_t chr = *txt++;
        // pal, pri, flipv, fliph, index
		uint16_t attr = TILE_ATTR(colour,1,0,0,CHARBASE + chr);
		*vdp_data_port = attr;
        --len;
	}
}

// draw null-terminated string
void plat_text(uint8_t cx, uint8_t cy, const char* txt, uint8_t colour)
{
    uint8_t len = 0;
    while(txt[len] != '\0') {
        ++len;
    }
    plat_textn(cx, cy, txt, len, colour);
}


// TODO: should just pass in level and score as bcd
void plat_hud(uint8_t level, uint8_t lives, uint32_t score)
{
}

void plat_mono4x2(uint8_t cx, int8_t cy, const uint8_t* src, uint8_t cw, uint8_t ch, uint8_t basecol)
{
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

static uint16_t sprites[80*4];

static void sprout16(int16_t x, int16_t y, uint8_t img)
{
    if (nsprites>=80) {
        return;
    }
    x = (x>>FX) + 128;
    y = (y>>FX) + 128;

    uint16_t *spr = &sprites[nsprites*4];
    *spr++ = y & 0x3FF;
    uint8_t link = nsprites+1;
    *spr++ = (0b00000101<<8) | link;
    *spr++ = 0x8000 | (SPR16BASE + img*4);
    *spr++ = x & 0x3FF;
    ++nsprites;
}

static void sprout16_highlight(int16_t x, int16_t y, uint8_t img)
{
    sprout16(x,y,img);
}

static void sprout32(int16_t x, int16_t y, uint8_t img)
{
}
static void sprout32_highlight(int16_t x, int16_t y, uint8_t img)
{
}
static void sprout64x8(int16_t x, int16_t y, uint8_t img)
{
}

static void megadrive_render_finish()
{
    if (nsprites == 0 ) {
        sprout16(-128<<FX,-128<<FX,0);
    }
    sprites[4*(nsprites-1) + 1] &= 0xff80;    // link = 0
    vdp_dma_vram((uint32_t)sprites, VRAM_SPRITE_ATTRS, (nsprites*8)/2);
}


#include "../spr_common_inc.h"
void plat_hzapper_render(int16_t x, int16_t y, uint8_t state)
{
}

void plat_vzapper_render(int16_t x, int16_t y, uint8_t state)
{
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
}
// end PLAT_HAS_TEXTENTRY
 
void plat_gatso(uint8_t t)
{
}

// sfx

void plat_psg(uint8_t chan, uint16_t freq, uint8_t vol, uint8_t waveform, uint8_t pulsewidth)
{
}


// controllers

uint8_t plat_raw_dualstick()
{
    uint16_t j = joy_get_state(0);

    uint8_t out = 0;
    if (j & BUTTON_UP) out |= INP_UP;
    if (j & BUTTON_DOWN) out |= INP_DOWN;
    if (j & BUTTON_LEFT) out |= INP_LEFT;
    if (j & BUTTON_RIGHT) out |= INP_RIGHT;

    // replicate dirs to fudge firing for now.
    return ((out&0x0f) <<4) | (out&0x0f);
}

// Returns direction + MENU_ bits.
uint8_t plat_raw_menukeys()
{
    uint16_t j = joy_get_state(0);
    uint8_t out = 0;
    if (j & BUTTON_UP) out |= INP_UP;
    if (j & BUTTON_DOWN) out |= INP_DOWN;
    if (j & BUTTON_LEFT) out |= INP_LEFT;
    if (j & BUTTON_RIGHT) out |= INP_RIGHT;
    if (j & BUTTON_A) out |= INP_MENU_A;
    if (j & BUTTON_B) out |= INP_MENU_B;
    if (j & BUTTON_START) out |= INP_MENU_START;
    if (j & BUTTON_MODE) out |= INP_MENU_ESC;
    return out;
} 

uint8_t plat_raw_cheatkeys()
{
    return 0;
}

// Draw a box in chars (effects layer)
void plat_drawbox(int8_t x, int8_t y, uint8_t w, uint8_t h, uint8_t ch, uint8_t colour)
{
}

bool plat_savescores(const void* begin, int nbytes)
{
    return false;
}

bool plat_loadscores(void* begin, int nbytes)
{
    return false;
}

