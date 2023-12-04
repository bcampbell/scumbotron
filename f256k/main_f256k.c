#include "../plat.h"
#include "../gob.h" // for ZAPPER_*




#define MMU_IO_CTRL (*(volatile unsigned char *)0x01)
// Tiny VICKY I/O pages
#define VICKY_IO_PAGE_REGISTERS			0	// Low level I/O registers
#define VICKY_IO_PAGE_FONT_AND_LUTS		1	// Text display font memory and graphics color LUTs
#define VICKY_IO_PAGE_CHAR_MEM			2	// Text display character matrix
#define VICKY_IO_PAGE_ATTR_MEM			3	// Text display color matrix



#define VKY_MSTR_CTRL_0 (*(volatile unsigned char *)0xD000)
#define VKY_MSTR_CTRL_1 (*(volatile unsigned char *)0xD001)
// X GAMMA SPRITE TILE BITMAP GRAPH OVRLY TEXT
// - - FON_SET FON_OVLY MON_SLP DBL_Y DBL_X CLK_70

#define BRDR_CTRL (*(volatile unsigned char *)0xD004)
#define BRDR_BLUE (*(volatile unsigned char *)0xD005)
#define BRDR_GREEN (*(volatile unsigned char *)0xD006)
#define BRDR_RED (*(volatile unsigned char *)0xD007)
#define BRDR_WIDTH (*(volatile unsigned char *)0xD008)
#define BRDR_HEIGHT (*(volatile unsigned char *)0xD009)

volatile uint8_t tick = 0;

extern unsigned char export_palette_bin[];
extern unsigned int export_palette_bin_len;
extern unsigned char export_spr16_bin[];
extern unsigned int export_spr16_bin_len;
extern unsigned char export_spr32_bin[];
extern unsigned int export_spr32_bin_len;
extern unsigned char export_spr64x8_bin[];
extern unsigned int export_spr64x8_bin_len;
extern unsigned char export_chars_bin[];
extern unsigned int export_chars_bin_len;


void init_text();

int main(void) {
    MMU_IO_CTRL = VICKY_IO_PAGE_REGISTERS;
    VKY_MSTR_CTRL_0 = 0b00000001;
    VKY_MSTR_CTRL_1 = 0b00000110;

    MMU_IO_CTRL = VICKY_IO_PAGE_CHAR_MEM;
    uint8_t *foo = (uint8_t*)0xC000;
    for (uint8_t i=0; i<255; ++i) 
    {
        foo[i] = i;
    }

    init_text();

    game_init();



    MMU_IO_CTRL = 0;

    BRDR_CTRL = 1;
    BRDR_WIDTH = 31;
    BRDR_HEIGHT = 31;

    uint8_t i = 0;
    while (1) {
        ++tick;
        MMU_IO_CTRL = VICKY_IO_PAGE_CHAR_MEM;
        uint8_t *p = ((uint8_t*)0xC000);
        *p = tick;
        BRDR_BLUE = i;
        ++i;
        game_tick();
        game_render();
    }

    return 0;
}

static inline uint8_t *chrmem(uint8_t cx, uint8_t cy)
{
    return ((uint8_t*)0xC000) + (cy*40) + cx;
}


// set up text layer stuff
void init_text()
{

    // load font
    {
        asm("nop\nnop\nnop\n");

        MMU_IO_CTRL = VICKY_IO_PAGE_FONT_AND_LUTS;
        const uint8_t* src = export_chars_bin;
        uint8_t* dest = ((uint8_t*)0xC000);
        for (int i=0; i<128; ++i) {
            for (int j=0; j<8; ++j) {
                *dest++ = *src++;
                asm("nop\n");
            }
            asm("nop\nnop\n");
        }
        asm("nop\nnop\nnop\n");
    }

    // text fg palette
    {
        //
        MMU_IO_CTRL = VICKY_IO_PAGE_FONT_AND_LUTS;
        const uint8_t* src = export_palette_bin;
        uint8_t * dest = ((uint8_t*)0xD800);
        for (uint8_t i = 0; i < 16; ++i) {
            dest[0] = src[2];   // b
            dest[1] = src[1];   // g
            dest[2] = src[0];    // r
            dest[3] = 0;    //src[3];   // a
            src += 4;
            dest += 4;
        }
    }
    // text bg palette
    {
        //
        MMU_IO_CTRL = VICKY_IO_PAGE_FONT_AND_LUTS;
        const uint8_t* src = export_palette_bin;
        uint8_t * dest = ((uint8_t*)0xD840);
        for (uint8_t i = 0; i < 16; ++i) {
            dest[0] = src[2];   // b
            dest[1] = src[1];   // g
            dest[2] = src[0];    // r
            dest[3] = 0;    //src[3];   // a
            src += 4;
            dest += 4;
        }
    }
}



// text layer. This is persistent - if you draw text it'll stay onscreen until
// overwritten, or plat_clr() is called.
void plat_clr()
{
    MMU_IO_CTRL = VICKY_IO_PAGE_ATTR_MEM;
    //MMU_IO_CTRL = VICKY_IO_PAGE_CHAR_MEM;
    for (uint8_t cy = 0; cy<SCREEN_TEXT_H; ++cy) {
        uint8_t* dest = chrmem(0,cy);
        for (uint8_t cx = 0; cx<SCREEN_TEXT_W; ++cx) {
            *dest++ = 0;
        }
    }
}

// draw len number of chars
void plat_textn(uint8_t cx, uint8_t cy, const char* txt, uint8_t len, uint8_t colour)
{
    MMU_IO_CTRL = VICKY_IO_PAGE_CHAR_MEM;
    uint8_t* dest = chrmem(cx,cy);
    for (int i=0; i<len; ++i) {
        *dest++ = *txt++;
    }

    MMU_IO_CTRL = VICKY_IO_PAGE_ATTR_MEM;
    dest = chrmem(cx,cy);
    for (int i=0; i<len; ++i) {
        *dest++ = colour<<4;
    }
}

// TODO: should just pass in level and score as bcd
void plat_hud(uint8_t level, uint8_t lives, uint32_t score)
{
}

// Render bonkers encoding where each byte encodes a 4x2 block of pixels,
// to be rendered via charset.
void plat_mono4x2(uint8_t cx, int8_t cy, const uint8_t* src, uint8_t cw, uint8_t ch, uint8_t basecol)
{
}

// Draw horizontal line of chars, range [cx_begin, cx_end).
void plat_hline_noclip(uint8_t cx_begin, uint8_t cx_end, uint8_t cy, uint8_t chr, uint8_t colour)
{
}

// Draw vertical line of chars, range [cy_begin, cy_end).
void plat_vline_noclip(uint8_t cx, uint8_t cy_begin, uint8_t cy_end, uint8_t chr, uint8_t colour)
{
}

void sprout16(int16_t x, int16_t y, uint8_t img)
{
}

void sprout16_highlight(int16_t x, int16_t y, uint8_t img)
{
}

void sprout32(int16_t x, int16_t y, uint8_t img)
{
}

void sprout32_highlight(int16_t x, int16_t y, uint8_t img)
{
}

void sprout64x8(int16_t x, int16_t y, uint8_t img )
{
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

void plat_gatso(uint8_t t)
{
}

void plat_psg(uint8_t chan, uint16_t freq, uint8_t vol, uint8_t waveform, uint8_t pulsewidth)
{
}


// Returns direction + FIRE_ bits.
uint8_t plat_raw_dualstick()
{
    return 0;
}


// Returns direction + MENU_ bits.
uint8_t plat_raw_menukeys()
{
    return 0;
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


