#include <nds.h>
#include "../sys.h"
#include "../gob.h" // for ZAPPER_*

#include <stdio.h>

volatile uint8_t tick = 0;

static inline void sprout16(int16_t x, int16_t y, uint8_t img);
static inline void sprout16_highlight(int16_t x, int16_t y, uint8_t img);
static inline void sprout32(int16_t x, int16_t y, uint8_t img);

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

void sys_text(uint8_t cx, uint8_t cy, const char* txt, uint8_t colour)
{
	iprintf("\x1b[10;0H%s", txt);
}

void sys_hud(uint8_t level, uint8_t lives, uint32_t score)
{
	iprintf("\x1b[0;l0Hlv %d  score: %d  lives: %d", (int)level, (int)score, (int)lives);
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
            hline_noclip(0, SCREEN_W, (y >> FX) + 8, 15);
            sprout16(x, y, SPR16_HZAPPER_ON);
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
            sprout16(x, y, SPR16_VZAPPER_ON);
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



u16* gfx;
u16* gfxSub;

static void init()
{
    int i;
	videoSetMode(MODE_0_2D);
	videoSetModeSub(MODE_0_2D);

	vramSetBankA(VRAM_A_MAIN_SPRITE);
	vramSetBankD(VRAM_D_SUB_SPRITE);

	oamInit(&oamMain, SpriteMapping_1D_32, false);
	oamInit(&oamSub, SpriteMapping_1D_32, false);

	gfx = oamAllocateGfx(&oamMain, SpriteSize_16x16, SpriteColorFormat_256Color);
	gfxSub = oamAllocateGfx(&oamSub, SpriteSize_16x16, SpriteColorFormat_256Color);

	for(i = 0; i < 16 * 16 / 2; i++)
	{
		gfx[i] = 1 | (1 << 8);
		gfxSub[i] = 1 | (1 << 8);
	}

	SPRITE_PALETTE[1] = RGB15(31,0,0);
	SPRITE_PALETTE_SUB[1] = RGB15(0,31,0);


	irqSet(IRQ_VBLANK, vblank);
	consoleDemoInit();
}



static inline void sprout16_highlight(int16_t x, int16_t y, uint8_t img)
{
}

static inline void sprout32(int16_t x, int16_t y, uint8_t img)
{
}



int spr;

static inline void sprout16(int16_t x, int16_t y, uint8_t img)
{
    oamSet(&oamMain, //main graphics engine context
        spr,           //oam index (0 to 127)  
        x>>FX, y>>FX,   //x and y pixle location of the sprite
        0,                    //priority, lower renders last (on top)
        0,					  //this is the palette index if multiple palettes or the alpha value if bmp sprite	
        SpriteSize_16x16,     
        SpriteColorFormat_256Color, 
        gfx,                  //pointer to the loaded graphics
        -1,                  //sprite rotation data  
        false,               //double the size when rotating?
        false,			//hide the sprite?
        false, false, //vflip, hflip
        false	//apply mosaic
        );              
    
    
    oamSet(&oamSub, 
        spr, 
        x>>FX, y>>FX,
        0, 
        0,
        SpriteSize_16x16, 
        SpriteColorFormat_256Color, 
        gfxSub, 
        -1, 
        false, 
        false,			
        false, false, 
        false	
        );              
    ++spr;
}	


int main(void) {
    init();

	while(1) {
        //sys_render_start();
        //sys_render_finish();
        //sfx_tick();
		scanKeys();
        game_tick();

        spr = 0;
        game_render();

		swiWaitForVBlank();
		
		oamUpdate(&oamMain);
		oamUpdate(&oamSub);
    }
	
	return 0;
}

