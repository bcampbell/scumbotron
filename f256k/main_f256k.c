#include "../plat.h"
#include "../gob.h" // for ZAPPER_*

#include "api.h"


#define MMU_MEM_CTRL (*(volatile unsigned char *)0x00)
#define MMU_IO_CTRL (*(volatile unsigned char *)0x01)

#define MMU_EDIT_EN 0x80

#define MMU_MEM_BANK_0 (*(volatile unsigned char *)0x08)
#define MMU_MEM_BANK_1 (*(volatile unsigned char *)0x09)
#define MMU_MEM_BANK_2 (*(volatile unsigned char *)0x0A)
#define MMU_MEM_BANK_3 (*(volatile unsigned char *)0x0B)
#define MMU_MEM_BANK_4 (*(volatile unsigned char *)0x0C)
#define MMU_MEM_BANK_5 (*(volatile unsigned char *)0x0D)
#define MMU_MEM_BANK_6 (*(volatile unsigned char *)0x0E)
#define MMU_MEM_BANK_7 (*(volatile unsigned char *)0x0F)


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




#define PS2_CTRL (*(volatile uint8_t *)0xD640)
#define PS2_OUT  (*(volatile uint8_t *)0xD641)
#define KBD_IN (*(volatile uint8_t *)0xD642)
#define MS_IN (*(volatile uint8_t *)0xD643)
#define PS2_STAT (*(volatile uint8_t *)0xD644)


// irq vector
#define VIRQ (*(uint16_t*)0xFFFE)

// group 0 regs
#define INT_PENDING_0 (*(volatile uint8_t *)0xD660)
#define INT_POLARITY_0 (*(volatile uint8_t *)0xD664)
#define INT_EDGE_0 (*(volatile uint8_t *)0xD668)
#define INT_MASK_0 (*(volatile uint8_t *)0xD66C)

// group 0 bits
#define INT_VKY_SOF 0x01 // TinyVicky Start Of Frame interrupt.
#define INT_VKY_SOL 0x02 // TinyVicky Start Of Line interrupt
#define INT_PS2_KBD 0x04 // PS/2 keyboard event
#define INT_PS2_MOUSE 0x08 // PS/2 mouse event
#define INT_TIMER_0 0x10 // TIMER0 has reached its target value
#define INT_TIMER_1 0x20 // TIMER1 has reached its target value

// group 1 bits

#define INT_VIA1 0x40   // F256k Only: Local keyboard

//0x40 RESERVED
//0x80 Cartridge Interrupt asserted by the cartidge

// group 1 regs
#define INT_PENDING_1 (*(volatile uint8_t *)0xD661)
#define INT_POLARITY_1 (*(volatile uint8_t *)0xD665)
#define INT_EDGE_1 (*(volatile uint8_t *)0xD669)
#define INT_MASK_1 (*(volatile uint8_t *)0xD66D)

// TODO: group 2 and 3 interrupt regs

#define VKY_GR_CLUT_0 0xD000


#define VIA_IORB (*(volatile uint8_t *)0xDC00)



// ---- lzsa decompression

extern uint16_t lzsa_srcptr;
extern uint16_t lzsa_dstptr;
void lzsa2_unpack();

volatile uint8_t tick = 0;

extern unsigned char export_palette_bin[];
extern unsigned int export_palette_bin_len;
extern unsigned char export_chars_zbin[];
extern unsigned int export_chars_zbin_len;

extern const unsigned char export_spr16_00_zbin[];
extern const unsigned int export_spr16_00_zbin_len;
extern const unsigned char export_spr16_01_zbin[];
extern const unsigned int export_spr16_01_zbin_len;
extern const unsigned char export_spr16_02_zbin[];
extern const unsigned int export_spr16_02_zbin_len;
extern const unsigned char export_spr16_03_zbin[];
extern const unsigned int export_spr16_03_zbin_len;

static uint8_t nsprites = 0;

__attribute__((interrupt)) void irq(void)
{
    uint8_t foo = MMU_IO_CTRL;
    MMU_IO_CTRL = VICKY_IO_PAGE_REGISTERS;
    uint8_t pending0 = INT_PENDING_0;
    if (pending0 & INT_VKY_SOF) {
        ++tick;
    }
    if (pending0 & INT_PS2_KBD) {
        
    }
    // clear the interrupts
    INT_PENDING_0 = 0;
    MMU_IO_CTRL = foo;
}

void init_text();

static void waitvbl()
{
    uint8_t t = tick;
    while (t==tick)
    {
    }
}


void init_gubbins()
{
    __asm("clc\nsei\n");

    // TODO: don't switch to an unknown MLUT!
    MMU_MEM_CTRL = MMU_EDIT_EN;

    MMU_MEM_BANK_0 = 0;
    MMU_MEM_BANK_1 = 1;
    MMU_MEM_BANK_2 = 2;
    MMU_MEM_BANK_3 = 3;
    MMU_MEM_BANK_4 = 4;
    MMU_MEM_BANK_5 = 5;
    MMU_MEM_BANK_6 = 6;
    MMU_MEM_BANK_7 = 7;
    MMU_MEM_CTRL = 0;   // done!

    VIRQ = (uint16_t)irq;

    // SOF (vblank)

    INT_MASK_0 = ~(INT_VKY_SOF|INT_PS2_KBD);



    // unpack sprite data into higher banks
    // each chunk unpacks to 8KB
    const uint16_t spr16_chunks[4] = {
        (uint16_t)export_spr16_00_zbin,
        (uint16_t)export_spr16_01_zbin,
        (uint16_t)export_spr16_02_zbin,
        (uint16_t)export_spr16_03_zbin,
    };
    for (uint8_t i = 0; i < 4; ++i) {

        MMU_MEM_CTRL = MMU_EDIT_EN;
        MMU_MEM_BANK_7 = (8+i);
        MMU_MEM_CTRL = 0;   // done!
    
        lzsa_srcptr = spr16_chunks[i];
        lzsa_dstptr = 0xE000;
        lzsa2_unpack();
    }
    // restore bank 7 so our irq handler is there!
    MMU_MEM_CTRL = MMU_EDIT_EN;
    MMU_MEM_BANK_7 = 7;
    MMU_MEM_CTRL = 0;

    __asm("cli\n");
}

int main(void) {
    init_gubbins();
    MMU_IO_CTRL = VICKY_IO_PAGE_REGISTERS;
    VKY_MSTR_CTRL_0 = 0b00100111;   // enable sprites,graphics and text
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
        waitvbl();
        MMU_IO_CTRL = VICKY_IO_PAGE_CHAR_MEM;
        uint8_t *p = ((uint8_t*)0xC000);
        game_tick();
        nsprites = 0;
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

    // decompress charset into place 
    {
        MMU_IO_CTRL = VICKY_IO_PAGE_FONT_AND_LUTS;
        lzsa_srcptr = (uint16_t)export_chars_zbin;
        lzsa_dstptr = 0xC000;
        lzsa2_unpack();
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
    // sprite palette 0
    {
        MMU_IO_CTRL = VICKY_IO_PAGE_FONT_AND_LUTS;
        const uint8_t* src = export_palette_bin;
        uint8_t * dest = ((uint8_t*)VKY_GR_CLUT_0);
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
    if (nsprites>=64) {
        return;
    }
    MMU_IO_CTRL = VICKY_IO_PAGE_REGISTERS;
    uint32_t addr =(uint32_t)(0x10000 + (img*16*16));    // TODO
    uint8_t *spr = (uint8_t*)(0xD900 + (nsprites*8));
    // -ssllcce
    // ss: size 00=32x32 01=24x24 10=16x16 11=8x8
    // ll: layer
    // cc: lut
    // e: enable
    *spr++ = 0b01000001;
    *spr++ = addr & 0xff;
    *spr++ = addr >> 8;
    *spr++ = (addr >> 16);

    x = (x >> FX);
    y = (y >> FX);
    x += 32;
    y += 32;

    *spr++ = x & 0xff;
    *spr++ = x >> 8;
    *spr++ = y & 0xff;
    *spr++ = y >> 8;
    ++nsprites;
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


static uint8_t firelock = 0;    // fire bits if locked (else 0)
static uint8_t facing = 0;  // last non-zero direction

// Returns direction + FIRE_ bits.
uint8_t plat_raw_dualstick()
{
    MMU_IO_CTRL = VICKY_IO_PAGE_REGISTERS;
    uint8_t j = VIA_IORB;
    uint8_t out = 0;
    if ((j & 0x01) == 0) out |= INP_UP;
    if ((j & 0x02) == 0) out |= INP_DOWN;
    if ((j & 0x04) == 0) out |= INP_LEFT;
    if ((j & 0x08) == 0) out |= INP_RIGHT;

    if (out != 0) {
        facing = out;
    }

    // button pressed?
    if ((j & 0x10) == 0) {
        if (!firelock) {
            firelock = (facing<<4);
        }
        out |= firelock;
    } else {
        firelock = 0;
    }

    return out;
}
 

// Returns direction + MENU_ bits.
uint8_t plat_raw_menukeys()
{
    MMU_IO_CTRL = VICKY_IO_PAGE_REGISTERS;
    uint8_t j = VIA_IORB;
    uint8_t out = 0;
    if ((j & 0x01) == 0) out |= INP_UP;
    if ((j & 0x02) == 0) out |= INP_DOWN;
    if ((j & 0x04) == 0) out |= INP_LEFT;
    if ((j & 0x08) == 0) out |= INP_RIGHT;
    if ((j & 0x10) == 0) out |= INP_MENU_START | INP_MENU_A;
    return out;
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





