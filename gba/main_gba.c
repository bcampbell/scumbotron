#include <gba_base.h>
#include <gba_video.h>
#include <gba_systemcalls.h>
#include <gba_interrupt.h>
#include <gba_input.h>
#include <gba_sprites.h>

#include "../plat.h"
#include "../misc.h"
#include "../gob.h" // for ZAPPER_*

// sprintf support...
#include <stdio.h>
#include <inttypes.h>

// bg0: textlayer
// bg1: effects layer
// both use same charset
#define TEXTLAYER_MAP_SLOT 8
#define TEXTLAYER_TILE_SLOT 0
#define EFFECTLAYER_MAP_SLOT 9
#define EFFECTLAYER_TILE_SLOT TEXTLAYER_TILE_SLOT

// VRAM use
//
// 0x6000000 Tile images (charset) 8*2*256 = 8KB
// 0x6002000 Unused 8KB
// 0x6004000 (map 8) Text layer 32x32 words 2KB
// 0x6004800 (map 9) Effect layer 32x32 words 2KB
// 0x6005000 Unused
// 0x6010000 Sprite images

extern unsigned char export_palette_bin[];
extern unsigned int export_palette_bin_len;
extern unsigned char export_spr16_bin[];
extern unsigned int export_spr16_bin_len;
extern unsigned char export_spr32_bin[];
extern unsigned int export_spr32_bin_len;
// GBA doesn't support 64x8 sprites, so these will be arranged as 32x8 pairs.
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
int oamIdx;

static void sprout32x8(int16_t x, int16_t y, uint8_t img);
static void sprout8x32(int16_t x, int16_t y, uint8_t img);


static void do_colour_cycling();
static uint8_t glyph(char ascii);
static void clr_textlayer();
static void clr_effectlayer();

// Unsupported
void plat_quit()
{
}


/*
 *  INPUT
 */

// Returns direction + FIRE_ bits.
// dualstick faked using plat_raw_gamepad()
static uint8_t firelock = 0;    // fire bits if locked (else 0)
static uint8_t facing = 0;  // last non-zero direction

uint8_t plat_raw_dualstick()
{
    uint8_t pad = plat_raw_gamepad();

    uint8_t out = pad & (INP_UP|INP_DOWN|INP_LEFT|INP_RIGHT);
    if (out != 0) {
        facing = out;
    }

    if (pad & INP_PAD_A) {
        if (!firelock) {
            firelock = (facing<<4);
        }
        out |= firelock;
    } else {
        firelock = 0;
    }

    return out;
}


uint8_t plat_raw_gamepad()
{
    static const struct {uint16_t hw; uint8_t bitmask; } key_mapping[7] = {
        {KEY_UP, INP_UP},
        {KEY_DOWN, INP_DOWN},
        {KEY_LEFT, INP_LEFT},
        {KEY_RIGHT, INP_RIGHT},
        {KEY_START, INP_PAD_START},
        {KEY_A, INP_PAD_A},
        {KEY_B, INP_PAD_B},
    };

    int i;
    uint8_t state = 0;
    uint16_t curr = ~REG_KEYINPUT;   //keysDown();
    for (i = 0; i < 7; ++i) {
        if ((curr & key_mapping[i].hw)) {
            state |= key_mapping[i].bitmask;
        }
    }
    return state;
}

uint8_t plat_raw_keys()
{
    return 0;   // no keyboards here!
}

uint8_t plat_raw_cheatkeys()
{
    // TODO: support some cheat keys
    return 0;
}

void plat_clr()
{
    clr_textlayer();
}

static uint8_t glyph(char ascii) {
    // 32-126 directly printable
    if (ascii >= 32 && ascii <= 126) {
        return (uint8_t)ascii;
    }
    return 0;
}

// get address of cell in text layer (bg0)
static uint16_t* textlayerptr(uint8_t cx, uint8_t cy) {
    return ((uint16_t*)MAP_BASE_ADR(TEXTLAYER_MAP_SLOT)) + cy*32 +cx;
}

// get address of cell in effects layer (bg0)
static uint16_t* effectlayerptr(uint8_t cx, uint8_t cy) {
    return ((uint16_t*)MAP_BASE_ADR(EFFECTLAYER_MAP_SLOT)) + cy*32 +cx;
}

void plat_textn(uint8_t cx, uint8_t cy, const char* txt, uint8_t len, uint8_t colour)
{
    uint16_t *dest = textlayerptr(cx, cy);
    // We've got a separate palette for each colour (!)
    if (colour ==0 ) {
        while(len--) {
            *dest++ = (uint16_t)glyph(' ');
        }
    } else {
        while(len--) {
            *dest++ = (uint16_t)glyph(*txt++) | CHAR_PALETTE(colour);
        }
    }
}

// cw must be even!
void plat_mono4x2(uint8_t cx, int8_t cy, const uint8_t* src, uint8_t cw, uint8_t ch, uint8_t basecol)
{
    for (int y=0; y < ch; ++y) {
        uint8_t c = (basecol + y) & 0x0f;
        uint8_t colour = (basecol + y) & 0x0f;
        uint16_t *dest = textlayerptr(cx, cy+y);
        for (int x=0; x < cw; x += 2) {
            // left char
            c = DRAWCHR_2x2 + (*src >> 4);
            *dest++ = ((uint16_t)colour<<12) | c;
            // right char
            c = DRAWCHR_2x2 + (*src & 0x0f);
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

void plat_psg(uint8_t chan, uint16_t freq, uint8_t vol, uint8_t waveform, uint8_t pulsewidth)
{
    // TODO: sound!
}

static uint16_t *copyWords(const uint16_t* src, uint16_t* dest, size_t nwords)
{
    while(nwords > 0) {
        *dest++ = *src++;
        --nwords;
    }
    return dest;
}


static void install_vram_data()
{
    // set up sprite palettes
    {
        int i;
        const uint16_t *src = (const uint16_t*)export_palette_bin;
        // sprite palette 0 for normal colours
        for (i = 0; i < 16; ++i) {
            SPRITE_PALETTE[i] = src[i];
        }
        // sprite palette 1 for highlight colours
        for (i = 16; i < 32; ++i) {
            SPRITE_PALETTE[i] = 0x7FFF;
        }
    }

    // set up BG palettes.
    // First palette is the real one, the other 15 are
    // all set to single colours, for text colouring.
    {
        int i;
        const uint16_t *src = (const uint16_t*)export_palette_bin;
        for (i=0; i<16; ++i) {
            BG_PALETTE[(i*16) + 0] = 0;
            int j;
            for (j=1; j<16; ++j) {
                BG_PALETTE[(i*16) + j] = src[i];
            }
        }
    }

    // load sprite images into vram
    {
        size_t nbytes;
        uint16_t *dest = SPRITE_GFX;

        // 16x16 sprites
        nbytes = (SPR16_NUM * SPR16_NTILES * TILE_BYTESIZE);
        dest = copyWords((const uint16_t*)export_spr16_bin, dest, nbytes/2);

        // 32x32
        nbytes = SPR32_NUM * SPR32_NTILES * TILE_BYTESIZE;
        dest = copyWords((const uint16_t*)export_spr32_bin, dest, nbytes/2);
 
        nbytes = SPR64x8_NUM * SPR64x8_NTILES * TILE_BYTESIZE;
        dest = copyWords((const uint16_t*)export_spr64x8_bin, dest, nbytes/2);
 
        // load zapper beam sprites into vram
        // (would prefer 64x8 but GBA/DS doesn't support it)
        nbytes = SPR32x8_NUM * SPR32x8_NTILES * TILE_BYTESIZE;
        dest = copyWords((const uint16_t*)export_spr32x8_bin, dest, nbytes/2);

        nbytes = SPR8x32_NUM * SPR8x32_NTILES * TILE_BYTESIZE;
        dest = copyWords((const uint16_t*)export_spr8x32_bin, dest, nbytes/2);
    }

    // load charset into vram
    {
        // 256 4bpp tiles
        copyWords((const uint16_t*)export_chars_bin,
                TILE_BASE_ADR(TEXTLAYER_TILE_SLOT),
                export_chars_bin_len / 2);
    }
}

void sprout16(int16_t x, int16_t y, uint8_t img)
{
    x >>= FX;
    y >>= FX;
    int tile = SPR16_BASETILE + (img * SPR16_NTILES);
    --oamIdx;
    OAM[oamIdx].attr0 = ATTR0_COLOR_16 | ATTR0_TYPE_NORMAL | ATTR0_NORMAL | ATTR0_SQUARE | OBJ_Y(y); 
    OAM[oamIdx].attr1 = ATTR1_SIZE_16 | OBJ_X(x);
    OAM[oamIdx].attr2 = OBJ_CHAR(tile) | OBJ_PALETTE(0) | OBJ_PRIORITY(0);
}

void sprout16_highlight(int16_t x, int16_t y, uint8_t img)
{
    x >>= FX;
    y >>= FX;
    int tile = SPR16_BASETILE + (img * SPR16_NTILES);
    uint16_t colour = 1;

    --oamIdx;
    OAM[oamIdx].attr0 = ATTR0_COLOR_16 | ATTR0_TYPE_NORMAL | ATTR0_NORMAL | ATTR0_SQUARE | OBJ_Y(y); 
    OAM[oamIdx].attr1 = ATTR1_SIZE_16 | OBJ_X(x);
    OAM[oamIdx].attr2 = OBJ_CHAR(tile) | OBJ_PALETTE(colour) | OBJ_PRIORITY(0);
}

void sprout32(int16_t x, int16_t y, uint8_t img)
{
    x >>= FX;
    y >>= FX;
    int tile = SPR32_BASETILE + (img * SPR32_NTILES);
    --oamIdx;
    OAM[oamIdx].attr0 = ATTR0_COLOR_16 | ATTR0_TYPE_NORMAL | ATTR0_NORMAL | ATTR0_SQUARE | OBJ_Y(y); 
    OAM[oamIdx].attr1 = ATTR1_SIZE_32 | OBJ_X(x);
    OAM[oamIdx].attr2 = OBJ_CHAR(tile) | OBJ_PALETTE(0) | OBJ_PRIORITY(0);
}

void sprout32_highlight(int16_t x, int16_t y, uint8_t img)
{
    x >>= FX;
    y >>= FX;
    int tile = SPR32_BASETILE + (img * SPR32_NTILES);
    uint16_t colour = 1;

    --oamIdx;
    OAM[oamIdx].attr0 = ATTR0_COLOR_16 | ATTR0_TYPE_NORMAL | ATTR0_NORMAL | ATTR0_SQUARE | OBJ_Y(y); 
    OAM[oamIdx].attr1 = ATTR1_SIZE_32 | OBJ_X(x);
    OAM[oamIdx].attr2 = OBJ_CHAR(tile) | OBJ_PALETTE(colour) | OBJ_PRIORITY(0);
}

// GBA doesn't support 64x8, so we use a 32x8 pair instead.
void sprout64x8(int16_t x, int16_t y, uint8_t img)
{
    x >>= FX;
    y >>= FX;
    int tile = SPR64x8_BASETILE + (img * SPR64x8_NTILES);

    --oamIdx;
    OAM[oamIdx].attr0 = ATTR0_COLOR_16 | ATTR0_TYPE_NORMAL | ATTR0_NORMAL | ATTR0_WIDE | OBJ_Y(y); 
    OAM[oamIdx].attr1 = ATTR1_SIZE_16 | OBJ_X(x);
    OAM[oamIdx].attr2 = OBJ_CHAR(tile) | OBJ_PALETTE(0) | OBJ_PRIORITY(0);

    tile += 4;
    x += 32;
    --oamIdx;
    OAM[oamIdx].attr0 = ATTR0_COLOR_16 | ATTR0_TYPE_NORMAL | ATTR0_NORMAL | ATTR0_WIDE | OBJ_Y(y); 
    OAM[oamIdx].attr1 = ATTR1_SIZE_16 | OBJ_X(x);
    OAM[oamIdx].attr2 = OBJ_CHAR(tile) | OBJ_PALETTE(0) | OBJ_PRIORITY(0);
}


static void sprout32x8(int16_t x, int16_t y, uint8_t img)
{
    x >>= FX;
    y >>= FX;
    int tile = SPR32x8_BASETILE + (img * SPR32x8_NTILES);
    --oamIdx;
    OAM[oamIdx].attr0 = ATTR0_COLOR_16 | ATTR0_TYPE_NORMAL | ATTR0_NORMAL | ATTR0_WIDE | OBJ_Y(y); 
    OAM[oamIdx].attr1 = ATTR1_SIZE_16 | OBJ_X(x);
    OAM[oamIdx].attr2 = OBJ_CHAR(tile) | OBJ_PALETTE(0) | OBJ_PRIORITY(0);
}

static void sprout8x32(int16_t x, int16_t y, uint8_t img)
{
    x >>= FX;
    y >>= FX;
    int tile = SPR8x32_BASETILE + (img * SPR8x32_NTILES);
    --oamIdx;
    OAM[oamIdx].attr0 = ATTR0_COLOR_16 | ATTR0_TYPE_NORMAL | ATTR0_NORMAL | ATTR0_TALL | OBJ_Y(y); 
    OAM[oamIdx].attr1 = ATTR1_SIZE_16 | OBJ_X(x);
    OAM[oamIdx].attr2 = OBJ_CHAR(tile) | OBJ_PALETTE(0) | OBJ_PRIORITY(0);
}


static void do_colour_cycling()
{
    int i = (((tick >> 1)) & 0x7) + 2;
    uint16_t c = ((const uint16_t*)export_palette_bin)[i];
    OBJ_COLORS[15] = c;
    BG_COLORS[15] = c;
    // bg palette 16 is all flashing (other than first colour)
    for(int j=1; j<16;++j) {
        BG_COLORS[15*16 +j] = c;
    }
}

// Clear text layer.
static void clr_textlayer()
{
    uint16_t *dest = textlayerptr(0,0);
    for (uint16_t i = 0; i < 32 * 32; ++i) {
        *dest++ = 0;    //(i & 0xff) | CHAR_PALETTE(i&15);   // upper 4 bits are palette
    }
}

static void clr_effectlayer()
{
    uint16_t *dest = effectlayerptr(0,0);
    for (uint16_t i = 0; i < 32 * 32; ++i) {
        *dest++ = 0;    //(i & 0xff) | CHAR_PALETTE(i&15);   // upper 4 bits are palette
    }
}

/*
 * drawing
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

// Draw vertical line of chars, range [cy_begin, cy_end).
void plat_vline_noclip(uint8_t cx, uint8_t cy_begin, uint8_t cy_end, uint8_t chr, uint8_t colour)
{
    chr = 127;
    uint16_t* dest = effectlayerptr(cx,cy_begin);
    uint16_t out = (uint16_t)chr | (((uint16_t)colour)<<12);
    for(int i = 0; i < (cy_end - cy_begin); ++i) {
        *dest = out;
        dest += 32;
    }
}

// Draw horizontal line of chars, range [cx_begin, cx_end).
void plat_hline_noclip(uint8_t cx_begin, uint8_t cx_end, uint8_t cy, uint8_t chr, uint8_t colour)
{
    chr = 127;
    uint16_t* dest = effectlayerptr(cx_begin,cy);
    uint16_t out = (uint16_t)chr | (((uint16_t)colour)<<12);
    for(int i = 0; i < (cx_end - cx_begin); ++i) {
        *dest++ = out;
    }
}

bool plat_savescores(const void* begin, int nbytes)
{
    return false;
}

bool plat_loadscores(void* begin, int nbytes)
{
    return false;
}


/*
 * main
 */

int main(void) {
	irqInit();
	irqEnable(IRQ_VBLANK);
	REG_IME = 1;

	SetMode(MODE_0 | BG0_ON | BG1_ON | OBJ_ON | OBJ_1D_MAP);

    // textlayer in bg0 - 32x32 chars (256x256 pixels)
    // effectlayer in bg1 - 32x32 chars (256x256 pixels)
    REG_BG0CNT = BG_SIZE_0 | BG_16_COLOR | BG_TILE_BASE(TEXTLAYER_TILE_SLOT) | BG_MAP_BASE(TEXTLAYER_MAP_SLOT);
    REG_BG0HOFS = 0;
    REG_BG0VOFS = 0;

    REG_BG1CNT = BG_SIZE_0 | BG_16_COLOR | BG_TILE_BASE(EFFECTLAYER_TILE_SLOT) | BG_MAP_BASE(EFFECTLAYER_MAP_SLOT);
    REG_BG1HOFS = 0;
    REG_BG1VOFS = 0;


    install_vram_data();
    game_init();

	while(1) {
        clr_effectlayer();
#if 0
        char buf[8];
        uint16_t keys = REG_KEYINPUT;
        hex32((uint32_t)keys, buf);
        plat_textn(0,0,buf,8,15);

        uint8_t raw = plat_raw_gamepad();
        hex32((uint32_t)raw, buf);
        plat_textn(0,1,buf,8,15);
#endif
        do_colour_cycling();
		//scanKeys();
        game_tick();

        // allocate sprites from highest to lowest
        oamIdx = 128;
        game_render();
        // clear leftover sprites
        while(oamIdx > 0) {
            --oamIdx;
            OAM[oamIdx].attr0 = OBJ_DISABLE;
        }


		VBlankIntrWait();
        ++tick;
    }
	
	return 0;
}

