#include <nds.h>
#include "../sys.h"
#include "../gob.h" // for ZAPPER_*

#include <stdio.h>

extern unsigned char export_palette_bin[];
extern unsigned int export_palette_bin_len;
extern unsigned char export_spr16_bin[];
extern unsigned int export_spr16_bin_len;
extern unsigned char export_spr32_bin[];
extern unsigned int export_spr32_bin_len;
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

static void internal_sprout( int16_t x, int16_t y, int tile, int w, int h, int sprSize, int pal);
static void sprout16(int16_t x, int16_t y, uint8_t img);
static void sprout16_highlight(int16_t x, int16_t y, uint8_t img);
static void sprout32(int16_t x, int16_t y, uint8_t img);
static void sprout32x8(int16_t x, int16_t y, uint8_t img);
static void sprout8x32(int16_t x, int16_t y, uint8_t img);

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

uint8_t sys_inp_dualsticks()
{
    int i;
    uint8_t out = 0;
    uint32 curr = keysCurrent();
    for (i = 0; i < 8; ++i) {
        if ((curr & key_mapping[i].hw)) {
            out |= key_mapping[i].bitmask;
        }
    }
    return out;
}

void sys_clr()
{
}

static uint8_t glyph(char ascii) {
    // 32-126 directly printable
    if (ascii >= 32 && ascii <= 126) {
        return (uint8_t)ascii;
    }
    return 0;
}

void sys_text(uint8_t cx, uint8_t cy, const char* txt, uint8_t colour)
{
    uint16_t* dest = BG_MAP_RAM(8) + cy*32 + cx;    // addressing 16bit words
    while(*txt) {
        uint16_t i = (uint16_t)glyph(*txt++) | ((uint16_t)colour<<12);
        *dest++ = i;    //0 | 1<<12;
    }
}

void sys_hud(uint8_t level, uint8_t lives, uint32_t score)
{
//	iprintf("\x1b[0;l0Hlv %d  score: %d  lives: %d", (int)level, (int)score, (int)lives);
}

// sprite image defs
#define SPR16_BAITER 16
#define SPR16_AMOEBA_MED 20
#define SPR16_AMOEBA_SMALL 24
#define SPR16_TANK 28
#define SPR16_GRUNT 30
#define SPR16_SHOT 4
#define SPR16_HZAPPER 12
#define SPR16_HZAPPER_ON 13
#define SPR16_VZAPPER 14
#define SPR16_VZAPPER_ON 15
#define SPR16_FRAGGER 32
#define SPR16_FRAG_NW 33
#define SPR16_FRAG_NE 34
#define SPR16_FRAG_SW 35
#define SPR16_FRAG_SE 36

#define SPR32_AMOEBA_BIG 0

#define NUM_SPR16 64
#define NUM_SPR32 8

#define BYTESIZE_SPR16 (16*16/2)   // 16x16 4bpp
#define BYTESIZE_SPR32 (32*32/2)   // 32x32 4bpp

// DS-specific sprites for zapper beams
#define NUM_SPR32x8 1
#define BYTESIZE_SPR32x8 (32*8/2)   // 32x8 4bpp
#define NUM_SPR8x32 1
#define BYTESIZE_SPR8x32 (8*32/2)   // 8x32 4bpp

static void hline_noclip(int x_begin, int x_end, int y, uint8_t colour)
{
}

static void vline_noclip(int x, int y_begin, int y_end, uint8_t colour)
{
}

static const uint8_t shot_spr[16] = {
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


void sys_player_render(int16_t x, int16_t y) {
    sprout16(x, y, 0);
}

void sys_shot_render(int16_t x, int16_t y, uint8_t direction) {
    sprout16(x, y, shot_spr[direction]);
}
void sys_block_render(int16_t x, int16_t y) {
    sprout16(x, y, 2);
}

void sys_grunt_render(int16_t x, int16_t y) {
    sprout16(x, y,  SPR16_GRUNT + ((tick >> 5) & 0x01));
}

void sys_baiter_render(int16_t x, int16_t y) {
    sprout16(x, y,  SPR16_BAITER + ((tick >> 2) & 0x03));
}

void sys_tank_render(int16_t x, int16_t y, bool highlight) {
    if (highlight) {
        sprout16_highlight(x, y,  SPR16_TANK + ((tick>>5) & 0x01));
    } else {
        sprout16(x, y,  SPR16_TANK + ((tick >> 5) & 0x01));
    }
}

void sys_amoeba_big_render(int16_t x, int16_t y) {
    sprout32(x, y,  SPR32_AMOEBA_BIG + ((tick >> 3) & 0x01));
}

void sys_amoeba_med_render(int16_t x, int16_t y) {
    sprout16(x, y, SPR16_AMOEBA_MED + ((tick >> 3) & 0x03));
}

void sys_amoeba_small_render(int16_t x, int16_t y) {
    sprout16(x, y,  SPR16_AMOEBA_SMALL + ((tick >> 3) & 0x03));
}

void sys_hzapper_render(int16_t x, int16_t y, uint8_t state) {
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

void sys_vzapper_render(int16_t x, int16_t y, uint8_t state) {
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

void sys_fragger_render(int16_t x, int16_t y) {
    sprout16(x, y,  SPR16_FRAGGER);
}

static int8_t frag_frames[4] = {SPR16_FRAG_NW, SPR16_FRAG_NE,
    SPR16_FRAG_SW, SPR16_FRAG_SE};

void sys_frag_render(int16_t x, int16_t y, uint8_t dir) {
    sprout16(x, y,  frag_frames[dir]);
}


void sys_gatso(uint8_t t)
{
}

void sys_sfx_play(uint8_t effect)
{
}

void sys_addeffect(int16_t x, int16_t y, uint8_t kind)
{
}

static void vblank()
{
	++tick;
}



static void init()
{
    videoSetMode(MODE_0_2D | DISPLAY_SPR_1D_LAYOUT | DISPLAY_SPR_ACTIVE | DISPLAY_BG0_ACTIVE);
    videoSetModeSub(MODE_0_2D | DISPLAY_SPR_1D_LAYOUT | DISPLAY_SPR_ACTIVE | DISPLAY_BG0_ACTIVE);

    vramSetBankA(VRAM_A_MAIN_BG_0x06000000);
    vramSetBankB(VRAM_B_MAIN_SPRITE_0x06400000);
    vramSetBankC(VRAM_C_SUB_BG_0x06200000);
    vramSetBankD(VRAM_D_SUB_SPRITE);

	//vramSetBankA(VRAM_A_MAIN_SPRITE);
	//vramSetBankD(VRAM_D_SUB_SPRITE);

  	// Set up backgrounds.
    // Tile base at: 0*0x4000 = 0
    // Map0 base at:  8*0x0800 = 0x4000
	REG_BG0CNT = BG_COLOR_16 | BG_32x32 | BG_TILE_BASE(0) | BG_MAP_BASE(8);
	REG_BG0CNT_SUB = BG_COLOR_16 | BG_32x32 | BG_TILE_BASE(0) | BG_MAP_BASE(8);

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
        nbytes = NUM_SPR16 * BYTESIZE_SPR16;
        dmaCopy(export_spr16_bin, destmain, nbytes);
        dmaCopy(export_spr16_bin, destsub, nbytes);
        destmain += nbytes;
        destsub += nbytes;

        nbytes = NUM_SPR32 * BYTESIZE_SPR32;
        dmaCopy(export_spr32_bin, destmain, nbytes);
        dmaCopy(export_spr32_bin, destsub, nbytes);
        destmain += nbytes;
        destsub += nbytes;
 
        // load zapper beam sprites into vram
        // (would prefer 64x8 but GBA/DS doesn't support it)
        nbytes = NUM_SPR32x8 * BYTESIZE_SPR32x8;
        dmaCopy(export_spr32x8_bin, destmain, nbytes);
        dmaCopy(export_spr32x8_bin, destsub, nbytes);
        destmain += nbytes;
        destsub += nbytes;

        nbytes = NUM_SPR8x32 * BYTESIZE_SPR8x32;
        dmaCopy(export_spr8x32_bin, destmain, nbytes);
        dmaCopy(export_spr8x32_bin, destsub, nbytes);
        destmain += nbytes;
        destsub += nbytes;
    }

    // set up BG palettes (use one palette for each colour!)
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
    if (x > -w && x < SCREEN_W && y >-h && y < (SCREEN_H/2)) {
        oamSet(&oamMain, sprMain, x, y, 0, pal,
            sprsize, SpriteColorFormat_16Color,
            oamGetGfxPtr(&oamMain, tile),
            -1, false, false, false, false, false);
        ++sprMain;
    }
    // On sub screen?
    y -= (SCREEN_H/2);
    if (x > -w && x < SCREEN_W && y >-h && y < (SCREEN_H/2)) {
        oamSet(&oamSub, sprSub, x, y, 0, pal,
            sprsize, SpriteColorFormat_16Color,
            oamGetGfxPtr(&oamSub, tile),
            -1, false, false, false, false, false);
        ++sprSub;
    }
}


static void sprout16(int16_t x, int16_t y, uint8_t img)
{
    int tile = img * 4;    // 4 tiles/sprite
    internal_sprout(x>>FX, y>>FX, tile, 16,16, SpriteSize_16x16, 0);
}

static void sprout16_highlight(int16_t x, int16_t y, uint8_t img)
{
    int tile = img * 4;    // 4 tiles/sprite
    internal_sprout(x>>FX, y>>FX, tile, 16,16, SpriteSize_16x16, 1);
}

static void sprout32(int16_t x, int16_t y, uint8_t img)
{
    int tile = (4 * NUM_SPR16) + img * 16;    // 16 tiles/sprite
    internal_sprout(x>>FX, y>>FX, tile, 32,32, SpriteSize_32x32, 0);
}

static void sprout32x8(int16_t x, int16_t y, uint8_t img)
{
    int tile = (4 * NUM_SPR16) + (16 * NUM_SPR32) + (4*img);
    internal_sprout(x>>FX, y>>FX, tile, 32,8, SpriteSize_32x8, 0);
}

static void sprout8x32(int16_t x, int16_t y, uint8_t img)
{
    int tile = (4 * NUM_SPR16) + (16 * NUM_SPR32) + (4 * NUM_SPR32x8) + (4*img);
    internal_sprout(x>>FX, y>>FX, tile, 8,32, SpriteSize_8x32, 0);
}

int main(void) {
    init();

	while(1) {
        //sys_render_start();
        //sys_render_finish();
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
        // clear text layer
        {
            int i;
            uint16_t *dest = BG_MAP_RAM(8);
            for (i = 0; i < 32 * 32; ++i) {
                *dest++ = 0x20;
            }
        }
    }
	
	return 0;
}

