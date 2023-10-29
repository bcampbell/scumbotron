#include <nds.h>
#include "../plat.h"
#include "../misc.h"
#include "../gob.h" // for ZAPPER_*

// sprintf support...
#include <stdio.h>
#include <inttypes.h>

// Combining the screens into a single play area.
// These are the screen extents (in pixels).
#define TOP_MAIN 0
#define BOTTOM_MAIN (TOP_MAIN + SCREEN_HEIGHT)
#define LEFT_MAIN 0
#define RIGHT_MAIN (LEFT_MAIN + SCREEN_WIDTH)

#define TOP_SUB (SCREEN_HEIGHT-32)
#define BOTTOM_SUB (TOP_SUB + SCREEN_HEIGHT)
#define LEFT_SUB 0
#define RIGHT_SUB (LEFT_SUB + SCREEN_WIDTH)

extern unsigned char export_palette_bin[];
extern unsigned int export_palette_bin_len;
extern unsigned char export_spr16_bin[];
extern unsigned int export_spr16_bin_len;
extern unsigned char export_spr32_bin[];
extern unsigned int export_spr32_bin_len;
// NDS doesn't support 64x8 sprites, so these will be arranged as 32x8 pairs.
extern unsigned char export_spr64x8_bin[];
extern unsigned int export_spr64x8_bin_len;
extern unsigned char export_chars_bin[];
extern unsigned int export_chars_bin_len;


extern unsigned char export_spr32x8_bin[];
extern unsigned int export_spr32x8_bin_len;
extern unsigned char export_spr8x32_bin[];
extern unsigned int export_spr8x32_bin_len;

volatile uint8_t tick = 0;

// track sprite usage during frame
int sprMain;
int sprSub;

static void sprout16(int16_t x, int16_t y, uint8_t img);
static void sprout16_highlight(int16_t x, int16_t y, uint8_t img);
static void sprout32(int16_t x, int16_t y, uint8_t img);
static void sprout32_highlight(int16_t x, int16_t y, uint8_t img);
static void sprout64x8(int16_t x, int16_t y, uint8_t img);
static void sprout32x8(int16_t x, int16_t y, uint8_t img);
static void sprout8x32(int16_t x, int16_t y, uint8_t img);

static void internal_sprout( int16_t x, int16_t y, int tile, int w, int h, int sprSize, int pal);

static void clr_bg0();
static void clr_bg1();
static void do_colour_cycling();
static uint8_t glyph(char ascii);

static void vline_chars_noclip(uint8_t cx, uint8_t cy_begin, uint8_t cy_end, uint8_t ch, uint8_t colour);
static void hline_chars_noclip(uint8_t cx_begin, uint8_t cx_end, uint8_t cy, uint8_t ch, uint8_t colour);

uint8_t plat_raw_dualstick()
{
    static const struct {uint16_t hw; uint8_t bitmask; } key_mapping[8] = {
        {KEY_UP, INP_UP},
        {KEY_DOWN, INP_DOWN},
        {KEY_LEFT, INP_LEFT},
        {KEY_RIGHT, INP_RIGHT},
        {KEY_X, INP_FIRE_UP},    // X
        {KEY_B, INP_FIRE_DOWN},  // B
        {KEY_Y, INP_FIRE_LEFT},  // Y
        {KEY_A, INP_FIRE_RIGHT}, // A
    };

    int i;
    uint8_t state = 0;
    uint32 curr = keysCurrent();
    for (i = 0; i < 8; ++i) {
        if ((curr & key_mapping[i].hw)) {
            state |= key_mapping[i].bitmask;
        }
    }
    return state;
}

uint8_t plat_raw_menukeys()
{
    static const struct {uint16_t hw; uint8_t bitmask; } key_mapping[8] = {
        {KEY_UP, INP_UP},
        {KEY_DOWN, INP_DOWN},
        {KEY_LEFT, INP_LEFT},
        {KEY_RIGHT, INP_RIGHT},
        {KEY_START, INP_MENU_START},
        {KEY_A, INP_MENU_A},
        {KEY_B, INP_MENU_B},
    };

    int i;
    uint8_t state = 0;
    uint32 curr = keysCurrent();
    for (i = 0; i < 8; ++i) {
        if ((curr & key_mapping[i].hw)) {
            state |= key_mapping[i].bitmask;
        }
    }
    return state;
}

uint8_t plat_raw_cheatkeys()
{
    // TODO: support some cheat keys
    return 0;
}

void plat_clr()
{
}

static uint8_t glyph(char ascii) {
    // 32-126 directly printable
    if (ascii >= 32 && ascii <= 126) {
        return (uint8_t)ascii;
    }
    return 0;
}

void plat_textn(uint8_t cx, uint8_t cy, const char* txt, uint8_t len, uint8_t colour)
{
    uint16_t* dest = BG_MAP_RAM(8) + cy*32 + cx;    // addressing 16bit words
    // We've got a separate palette for each colour (!)
    while(len--) {
        uint16_t i = (uint16_t)glyph(*txt++) | ((uint16_t)colour<<12);
        *dest++ = i;    //0 | 1<<12;
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
    int y;
    for (y=0; y < ch; ++y) {
        uint8_t c = (basecol + y) & 0x0f;
        uint8_t colour = (basecol + y) & 0x0f;
        uint16_t* dest = BG_MAP_RAM(8) + (cy+y)*32 + cx;    // addressing 16bit words
        int x;
        for (x=0; x < cw; x += 2) {
            // left char
            c = 128 + (*src >> 4);
            *dest++ = ((uint16_t)colour<<12) | c;
            // right char
            c = 128 + (*src & 0x0f);
            *dest++ = ((uint16_t)colour<<12) | c;
            ++src;
        }
    }
}


void plat_hud(uint8_t level, uint8_t lives, uint32_t score)
{
    char buf[40];
    uint8_t i;
    sprintf(buf, "LV %" PRIu8 "", level);
    plat_text(0, 0, buf, 1);

    sprintf(buf,"%08" PRIu32 "", score);
    plat_text(8, 0, buf, 1);

    for (i = 0; i < 7 && i < lives; ++i) {
        buf[i] = '*';
    }
    if (lives > 7) {
        buf[i++] = '+';
    }
    buf[i] = '\0';
    plat_text(18, 0, buf, 3);
}


// 32x8 and 8x32 are DS-specific sprites for zapper beams
#define SPR16_NUM 128
#define SPR32_NUM 3
#define SPR64x8_NUM 4
#define SPR32x8_NUM 1
#define SPR8x32_NUM 1

// Number of tiles per sprite.
#define SPR16_NTILES 4
#define SPR32_NTILES 16
#define SPR64x8_NTILES (4+4)
#define SPR32x8_NTILES 4
#define SPR8x32_NTILES 4

// Bytes per tile.
#define TILE_BYTESIZE (8*4)   // 8x8 4bpp

// First tile for each type of sprite.
#define SPR16_BASETILE 0
#define SPR32_BASETILE (SPR16_BASETILE + (SPR16_NUM * SPR16_NTILES))
#define SPR64x8_BASETILE (SPR32_BASETILE + (SPR32_NUM * SPR32_NTILES))
#define SPR32x8_BASETILE (SPR64x8_BASETILE + (SPR64x8_NUM * SPR64x8_NTILES))
#define SPR8x32_BASETILE (SPR32x8_BASETILE + (SPR32x8_NUM * SPR32x8_NTILES))

static void hline_noclip(int x_begin, int x_end, int y, uint8_t colour)
{
}

static void vline_noclip(int x, int y_begin, int y_end, uint8_t colour)
{
}

#include "../spr_common_inc.h"

void plat_hzapper_render(int16_t x, int16_t y, uint8_t state) {
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
            {
                int px;
                sprout16(x, y, SPR16_HZAPPER_ON);
                for(px = 0; px < SCREEN_W; px += 32) {
                    sprout32x8(px<<FX, y + (4<<FX), 0);
                }
            }
            break;
    }
}

void plat_vzapper_render(int16_t x, int16_t y, uint8_t state) {
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
            vline_noclip((x>>FX)+8, 0, SCREEN_H, 15);
            {
                int py;
                sprout16(x, y, SPR16_VZAPPER_ON);
                for(py = 0; py < SCREEN_H; py += 32) {
                    sprout8x32(x + (4<<FX), py<<FX, 0);
                }
            }
            break;
    }
}

void plat_gatso(uint8_t t)
{
}

void plat_sfx_play(uint8_t effect)
{
}

static void vblank()
{
	++tick;
}



static void plat_init()
{
    // Set both displays up the same.
    // BG0 for text
    // BG1 for effects
    videoSetMode(MODE_0_2D | DISPLAY_SPR_1D_LAYOUT | DISPLAY_SPR_ACTIVE | DISPLAY_BG0_ACTIVE | DISPLAY_BG1_ACTIVE);
    videoSetModeSub(MODE_0_2D | DISPLAY_SPR_1D_LAYOUT | DISPLAY_SPR_ACTIVE | DISPLAY_BG0_ACTIVE | DISPLAY_BG1_ACTIVE);

    vramSetBankA(VRAM_A_MAIN_BG_0x06000000);
    vramSetBankB(VRAM_B_MAIN_SPRITE_0x06400000);
    vramSetBankC(VRAM_C_SUB_BG_0x06200000);
    vramSetBankD(VRAM_D_SUB_SPRITE);

	//vramSetBankA(VRAM_A_MAIN_SPRITE);
	//vramSetBankD(VRAM_D_SUB_SPRITE);

  	// Set up backgrounds.
    // Tile base at: 0*0x4000 = 0
    // BG0 map base at:  8*0x0800 = 0x4000
	REG_BG0CNT = BG_COLOR_16 | BG_32x32 | BG_TILE_BASE(0) | BG_MAP_BASE(8);
	REG_BG0CNT_SUB = BG_COLOR_16 | BG_32x32 | BG_TILE_BASE(0) | BG_MAP_BASE(8);
    // BG1 map base at:  9*0x0800 = 0x4800
	REG_BG1CNT = BG_COLOR_16 | BG_32x32 | BG_TILE_BASE(0) | BG_MAP_BASE(9);
	REG_BG1CNT_SUB = BG_COLOR_16 | BG_32x32 | BG_TILE_BASE(0) | BG_MAP_BASE(9);

	oamInit(&oamMain, SpriteMapping_1D_32, false);
	oamInit(&oamSub, SpriteMapping_1D_32, false);

    // set up sprite palettes
    {
        int i;
        const uint16_t *src = (const uint16_t*)export_palette_bin;
        for (i = 0; i < 16; ++i) {
            SPRITE_PALETTE[i] = src[i];
            SPRITE_PALETTE_SUB[i] = src[i];
        }
        // highlight palette
        for (i = 16; i < 32; ++i) {
            SPRITE_PALETTE[i] = 0x7FFF;
            SPRITE_PALETTE_SUB[i] = 0x7FFF;
        }
    }

    // load sprites into vram
    {
        size_t nbytes;
        uint8_t *destmain = (uint8_t*)SPRITE_GFX;
        uint8_t *destsub = (uint8_t*)SPRITE_GFX_SUB;
        nbytes = SPR16_NUM * SPR16_NTILES * TILE_BYTESIZE;
        dmaCopy(export_spr16_bin, destmain, nbytes);
        dmaCopy(export_spr16_bin, destsub, nbytes);
        destmain += nbytes;
        destsub += nbytes;

        nbytes = SPR32_NUM * SPR32_NTILES * TILE_BYTESIZE;
        dmaCopy(export_spr32_bin, destmain, nbytes);
        dmaCopy(export_spr32_bin, destsub, nbytes);
        destmain += nbytes;
        destsub += nbytes;
 
        nbytes = SPR64x8_NUM * SPR64x8_NTILES * TILE_BYTESIZE;
        dmaCopy(export_spr64x8_bin, destmain, nbytes);
        dmaCopy(export_spr64x8_bin, destsub, nbytes);
        destmain += nbytes;
        destsub += nbytes;
 
        // load zapper beam sprites into vram
        // (would prefer 64x8 but GBA/DS doesn't support it)
        nbytes = SPR32x8_NUM * SPR32x8_NTILES * TILE_BYTESIZE;
        dmaCopy(export_spr32x8_bin, destmain, nbytes);
        dmaCopy(export_spr32x8_bin, destsub, nbytes);
        destmain += nbytes;
        destsub += nbytes;

        nbytes = SPR8x32_NUM * SPR8x32_NTILES * TILE_BYTESIZE;
        dmaCopy(export_spr8x32_bin, destmain, nbytes);
        dmaCopy(export_spr8x32_bin, destsub, nbytes);
        destmain += nbytes;
        destsub += nbytes;
    }

    // set up BG palettes.
    // First palette is the real one, the other 15 are
    // all set to single colours, for text colouring.
    {
        int i;
        const uint16_t *src = (const uint16_t*)export_palette_bin;
        for (i=0; i<16; ++i) {
            BG_PALETTE[(i*16) + 0] = 0;
            BG_PALETTE_SUB[(i*16) + 0] = 0;
            int j;
            for (j=1; j<16; ++j) {
                BG_PALETTE[(i*16) + j] = src[i];
                BG_PALETTE_SUB[(i*16) + j] = src[i];
            }

        }
    }
    // load charset into vram
    {
        dmaCopy(export_chars_bin, BG_TILE_RAM(0), (8*4)*256);   // 256 4bpp tiles
        dmaCopy(export_chars_bin, BG_TILE_RAM_SUB(0), (8*4)*256);   // 256 4bpp tiles
    }

	irqSet(IRQ_VBLANK, vblank);
	//consoleDemoInit();
}

// Plonk a sprite on the virtual playfield, which spans both screens.
// x,y:  pixel coords (not fixedpoint)
// tile: first tile of image
// pal:  palette index to use
// w,h:  size in pixels (not fixedpoint)
// sprSize: SpriteSize_* enum
static void internal_sprout( int16_t x, int16_t y, int tile, int w, int h, int sprsize, int pal)
{
    // On main screen?
    if ((x + w) >= LEFT_MAIN && x < RIGHT_MAIN && (y + h) >= TOP_MAIN && y < BOTTOM_MAIN) {
        oamSet(&oamMain, sprMain, x - LEFT_MAIN, y - TOP_MAIN, 0, pal,
            sprsize, SpriteColorFormat_16Color,
            oamGetGfxPtr(&oamMain, tile),
            -1, false, false, false, false, false);
        ++sprMain;
    }
    // On sub screen?
    if ((x + w) >= LEFT_SUB && x < RIGHT_SUB && (y + h) >= TOP_SUB && y < BOTTOM_SUB) {
        oamSet(&oamSub, sprSub, x - LEFT_SUB, y - TOP_SUB, 0, pal,
            sprsize, SpriteColorFormat_16Color,
            oamGetGfxPtr(&oamSub, tile),
            -1, false, false, false, false, false);
        ++sprSub;
    }
}


static void sprout16(int16_t x, int16_t y, uint8_t img)
{
    int tile = SPR16_BASETILE + (img * SPR16_NTILES);
    internal_sprout(x>>FX, y>>FX, tile, 16,16, SpriteSize_16x16, 0);
}

static void sprout16_highlight(int16_t x, int16_t y, uint8_t img)
{
    int tile = SPR16_BASETILE + (img * SPR16_NTILES);
    internal_sprout(x>>FX, y>>FX, tile, 16,16, SpriteSize_16x16, 1);
}

static void sprout32(int16_t x, int16_t y, uint8_t img)
{
    int tile = SPR32_BASETILE + (img * SPR32_NTILES);
    internal_sprout(x>>FX, y>>FX, tile, 32,32, SpriteSize_32x32, 0);
}

static void sprout32_highlight(int16_t x, int16_t y, uint8_t img)
{
    int tile = SPR32_BASETILE + (img * SPR32_NTILES);
    internal_sprout(x>>FX, y>>FX, tile, 32,32, SpriteSize_32x32, 1);
}

// NDS doesn't support 64x8, so we use a 32x8 pair instead.
static void sprout64x8(int16_t x, int16_t y, uint8_t img)
{
    int tile = SPR64x8_BASETILE + (img * SPR64x8_NTILES);
    internal_sprout(x >> FX, y >> FX, tile, 32, 8, SpriteSize_32x8, 0);
    internal_sprout((x >> FX) + 32, y >> FX, tile + 4, 32, 8, SpriteSize_32x8, 0);
}


static void sprout32x8(int16_t x, int16_t y, uint8_t img)
{
    int tile = SPR32x8_BASETILE + (img * SPR32x8_NTILES);
    internal_sprout(x>>FX, y>>FX, tile, 32,8, SpriteSize_32x8, 0);
}

static void sprout8x32(int16_t x, int16_t y, uint8_t img)
{
    int tile = SPR8x32_BASETILE + (img * SPR8x32_NTILES);
    internal_sprout(x>>FX, y>>FX, tile, 8,32, SpriteSize_8x32, 0);
}


static void do_colour_cycling()
{
    int i = (((tick >> 1)) & 0x7) + 2;
    uint16_t c = ((const uint16_t*)export_palette_bin)[i];
    SPRITE_PALETTE[15] = c;
    SPRITE_PALETTE_SUB[15] = c;
    BG_PALETTE_SUB[15] = c;
    BG_PALETTE_SUB[15] = c;
}

// Clear text layer.
static void clr_bg0()
{
    int i;
    uint16_t *dest;
    dest = BG_MAP_RAM(8);
    for (i = 0; i < 32 * 32; ++i) {
        *dest++ = 0x20;
    }
    dest = BG_MAP_RAM_SUB(8);
    for (i = 0; i < 32 * 32; ++i) {
        *dest++ = 0x20;
    }
}

// Clear effects layer.
static void clr_bg1()
{
    int i;
    uint16_t *dest;
    dest = BG_MAP_RAM_SUB(9);
    for (i = 0; i < 32 * 32; ++i) {
        uint16_t cc = (uint16_t)0x20 | ((uint16_t)0 << 12);
        *dest++ = cc;
    }
    dest = BG_MAP_RAM(9);
    for (i = 0; i < 32 * 32; ++i) {
        uint16_t cc = (uint16_t)0x20 | ((uint16_t)0 << 12);
        *dest++ = cc;
    }
}

/*
 * effects
 */


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


static uint16_t* bg1_mapaddr_main(int cx, int cy)
    { return BG_MAP_RAM(9) + ((cy - TOP_MAIN / 8) * 32) + cx; }

static uint16_t* bg1_mapaddr_sub(int cx, int cy)
    { return BG_MAP_RAM_SUB(9) + ((cy - TOP_SUB / 8) * 32) + cx; }

// Draw vertical line of chars, range [cy_begin, cy_end).
static void vline_chars_noclip(uint8_t cx, uint8_t cy_begin, uint8_t cy_end, uint8_t ch, uint8_t colour)
{
    uint8_t cy;
    uint16_t *dest;
    uint16_t out = (uint16_t)ch | (((uint16_t)colour)<<12);
    int8_t cy_begin_main = cclip(cy_begin, TOP_MAIN/8, BOTTOM_MAIN/8);
    int8_t cy_end_main = cclip(cy_end, TOP_MAIN/8, BOTTOM_MAIN/8);
    int8_t cy_begin_sub = cclip(cy_begin, TOP_SUB/8, BOTTOM_SUB/8);
    int8_t cy_end_sub = cclip(cy_end, TOP_SUB/8, BOTTOM_SUB/8);

    // Main display
    dest = bg1_mapaddr_main(cx, cy_begin_main);
    for (cy = cy_begin_main; cy < cy_end_main; ++cy) {
        *dest = out;
        dest += 32;
    }
    //Sub display
    dest = bg1_mapaddr_sub(cx, cy_begin_sub);
    for (cy = cy_begin_sub; cy < cy_end_sub; ++cy) {
        *dest = out; 
        dest += 32;
    }
}

// Draw horizontal line of chars, range [cx_begin, cx_end).
static void hline_chars_noclip(uint8_t cx_begin, uint8_t cx_end, uint8_t cy, uint8_t ch, uint8_t colour)
{
    uint8_t cx;
    uint16_t *dest;
    uint16_t out = (uint16_t)ch | (((uint16_t)colour)<<12);
    if ( cy >= (TOP_MAIN / 8) && cy < (BOTTOM_MAIN / 8)) {
        // Main display
        dest = bg1_mapaddr_main(cx_begin, cy);
        for (cx = cx_begin; cx < cx_end; ++cx) {
            *dest++ = out;
        }
    }
    if ( cy >= (TOP_SUB / 8) && cy < (BOTTOM_SUB / 8)) {
        // Main display
        dest = bg1_mapaddr_sub(cx_begin, cy);
        for (cx = cx_begin; cx < cx_end; ++cx) {
            *dest++ = out;
        }
    }
}

// draw box in char coords, with clipping
// (note cx,cy can be negative)
void plat_drawbox(int8_t cx, int8_t cy, uint8_t w, uint8_t h, uint8_t ch, uint8_t colour)
{
    int x0,y0,x1,y1;
    x0 = cclip(cx, 0, SCREEN_W / 8);
    x1 = cclip(cx + w, 0, SCREEN_W / 8);
    y0 = cclip(cy, 0, SCREEN_H / 8);
    y1 = cclip(cy + h, 0, SCREEN_H / 8);

    // top
    if (y0 == cy) {
        hline_chars_noclip(x0, x1, y0, ch, colour);
    }
    if (h<=1) {
        return;
    }

    // bottom
    if (y1 - 1 == cy + h-1) {
        hline_chars_noclip(x0, x1, y1 - 1, ch, colour);
    }
    if (h<=2) {
        return;
    }

    // left (excluding top and bottom)
    if (x0 == cx) {
        vline_chars_noclip(x0, y0, y1, ch, colour);
    }
    if (w <= 1) {
        return;
    }

    // right (excluding top and bottom)
    if (x1 - 1 == cx + w - 1) {
        vline_chars_noclip(x1 - 1, y0, y1, ch, colour);
    }
}


/*
 * main
 */

int main(void) {
    plat_init();
    game_init();

	while(1) {
        clr_bg0();
        clr_bg1();

        do_colour_cycling();
        //sfx_tick();
		scanKeys();
        game_tick();

        sprMain = 0;
        sprSub = 0;
        game_render();

        // clear leftover sprites
        oamClear(&oamMain, sprMain, 128-sprMain);
        oamClear(&oamSub, sprSub, 128-sprSub);

		swiWaitForVBlank();

		oamUpdate(&oamMain);
		oamUpdate(&oamSub);

    }
	
	return 0;
}

