#include "../plat.h"
#include "../gob.h" // for ZAPPER_*
#include "../misc.h"


// added a hack to junior-emulator to log writes to 0xF00D :-)
#define F00D (*(volatile unsigned char *)0xF00D)

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


// VICKY cursor registers
#define VKY_CURSOR_CCR (*(volatile unsigned char *)0xD010)
#define VKY_CURSOR_CCH (*(volatile unsigned char *)0xD012)
#define VKY_CURSOR_CURX (*(volatile uint16_t *)0xD014)
#define VKY_CURSOR_CURY (*(volatile uint16_t *)0xD016)





//

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

// IO page 0 (VICKY_IO_PAGE_REGISTERS)
#define VKY_TEXT_FG_CLUT 0xD800
#define VKY_TEXT_BG_CLUT 0xD840

// IO page 1 (VICKY_IO_PAGE_FONT_AND_LUTS)
#define VKY_GR_CLUT_0 0xD000
#define VKY_GR_CLUT_1 0xD400
#define VKY_GR_CLUT_2 0xD800
#define VKY_GR_CLUT_3 0xDC00



#define VIA_IORB (*(volatile uint8_t *)0xDC00)


#if 0
// ---- lzsa decompression

extern uint16_t lzsa_srcptr;
extern uint16_t lzsa_dstptr;
void lzsa2_unpack();
#endif

static void sprites_init();
static void sprites_beginframe();
static void sprites_endframe();

extern unsigned char export_palette_bin[];
extern unsigned int export_palette_bin_len;

#if 0
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
#endif


// details for all the graphics data installed by the unpacker
#define SPR16_00_START 0x10000
#define SPR16_01_START 0x12000
#define SPR16_02_START 0x14000
#define SPR16_03_START 0x16000
#define SPR32_START 0x18000
#define SPR64x8_START 0x1A000


// PS/2 kbd
//
//

volatile uint8_t ps2_dualstick = 0;
volatile uint8_t ps2_menukeys = 0;
volatile uint8_t ps2_state = 0;     // 0x01 = extend, 0x02 = release
volatile bool ps2_release = false;

static inline uint8_t ps2_code_to_dualstick(uint8_t scancode, bool extended)
{
    if( extended) {
        switch (scancode) {
            // cursor keys E0 + ...
            case 0x75: return INP_FIRE_UP;
            case 0x72: return INP_FIRE_DOWN;
            case 0x6B: return INP_FIRE_LEFT;
            case 0x74: return INP_FIRE_RIGHT;
        }
    } else {
        switch (scancode) {
            // WASD
            case 0x1D: return INP_UP;
            case 0x1B: return INP_DOWN;
            case 0x1C: return INP_LEFT;
            case 0x23: return INP_RIGHT;
        }
    }
    return 0;
}

static inline uint8_t ps2_code_to_menukeys(uint8_t scancode, bool extended) {
    if (extended) {
        switch (scancode) {
            // cursor keys E0 + ...
            case 0x75: return INP_UP;
            case 0x72: return INP_DOWN;
            case 0x6B: return INP_LEFT;
            case 0x74: return INP_RIGHT;
        }
    } else {
        switch (scancode) {
            // cursor keys
            case 0x5A: return INP_MENU_START;   // Enter
            case 0x76: return INP_MENU_ESC;     // Esc 
        }
    }
    return 0;
}



volatile uint8_t tick = 0;
__attribute__((interrupt)) void irq(void)
{
    static uint8_t latch = 0;
    uint8_t foo = MMU_IO_CTRL;
    MMU_IO_CTRL = VICKY_IO_PAGE_REGISTERS;
    uint8_t pending0 = INT_PENDING_0;
    if (pending0 & INT_VKY_SOF) {
        ++tick;
    }

    if (pending0 & INT_PS2_KBD) {
        while( !(PS2_STAT & 0x01)) {
            uint8_t k = KBD_IN;
            if (k == 0xe0) {
                // extend
                ps2_state |= 0x01; // extend
            } else if (k == 0xf0) {
                ps2_state |= 0x02;
            } else {
                // not E0 or F0. Time to decode the sequence.
                bool extended = (ps2_state & 0x01) ? true : false;
                if ((ps2_state & 0x02) == 0) {
                    // key pressed
                    ps2_dualstick |= ps2_code_to_dualstick(k, extended);
                    ps2_menukeys |= ps2_code_to_menukeys(k, extended);
                } else {
                    // key released
                    ps2_dualstick &= ~ps2_code_to_dualstick(k, extended);
                    ps2_menukeys &= ~ps2_code_to_menukeys(k, extended);
                }

                //clear extended and release flags.
                ps2_state = 0;
           }
       }
    }

    // clear the interrupts
    INT_PENDING_0 = 0;
    MMU_IO_CTRL = foo;
}

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

    // TODO: be more careful about MLUT twiddling?
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

    // Enable SOF (vblank)
    MMU_IO_CTRL = VICKY_IO_PAGE_REGISTERS;
    INT_MASK_0 = ~(INT_VKY_SOF|INT_PS2_KBD);
    __asm("cli\n");

#if 0
    // unpack sprite data into ram bank 8 and onward
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

    // decompress charset into place 
    {
        MMU_IO_CTRL = VICKY_IO_PAGE_FONT_AND_LUTS;
        lzsa_srcptr = (uint16_t)export_chars_zbin;
        lzsa_dstptr = 0xC000;
        lzsa2_unpack();
    }
#endif


    // text fg palette
    {
        //
        MMU_IO_CTRL = VICKY_IO_PAGE_REGISTERS;
        const uint8_t* src = export_palette_bin;
        uint8_t * dest = ((uint8_t*)VKY_TEXT_FG_CLUT);
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
        MMU_IO_CTRL = VICKY_IO_PAGE_REGISTERS;
        const uint8_t* src = export_palette_bin;
        uint8_t * dest = ((uint8_t*)VKY_TEXT_BG_CLUT);
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
        uint8_t *dest = ((uint8_t*)VKY_GR_CLUT_0);
        for (uint8_t i = 0; i < 16; ++i) {
            dest[0] = src[2];   // b
            dest[1] = src[1];   // g
            dest[2] = src[0];    // r
            dest[3] = 0;    //src[3];   // a
            src += 4;
            dest += 4;
        }
    }
    // sprite palette 1 - highlight
    {
        MMU_IO_CTRL = VICKY_IO_PAGE_FONT_AND_LUTS;
        uint8_t *dest = ((uint8_t*)VKY_GR_CLUT_1);
        *dest++ = 0;
        *dest++ = 0;
        *dest++ = 0;
        *dest++ = 0;
        for (uint8_t i = 1; i < 16; ++i) {
            *dest++ = 255;
            *dest++ = 255;
            *dest++ = 255;
            *dest++ = 255;
        }
    }
}


// colour-cycle colour 15
static void do_colourcycle()
{
    uint8_t idx = (((tick >> 1)) & 0x7) + 2;
    const uint8_t* src = export_palette_bin + (idx * 4);
 
    // sprite palette 
    MMU_IO_CTRL = VICKY_IO_PAGE_FONT_AND_LUTS;
    uint8_t *dest = ((uint8_t*)(VKY_GR_CLUT_0 + (15 * 4)));
    dest[0] = src[2];   // b
    dest[1] = src[1];   // g
    dest[2] = src[0];   // r
    dest[3] = src[3];   // a

    // text fg palette 
    MMU_IO_CTRL = VICKY_IO_PAGE_REGISTERS;
    dest = ((uint8_t*)(VKY_TEXT_FG_CLUT + (15 * 4)));
    dest[0] = src[2];   // b
    dest[1] = src[1];   // g
    dest[2] = src[0];   // r
    dest[3] = src[3];   // a
}


int main(void) {
    init_gubbins();
    MMU_IO_CTRL = VICKY_IO_PAGE_REGISTERS;
    VKY_MSTR_CTRL_0 = 0b00100111;   // enable sprites,graphics and text
    VKY_MSTR_CTRL_1 = 0b00000110;

    // cursor off
    VKY_CURSOR_CCR = 0;

    game_init();

    MMU_IO_CTRL = 0;

    BRDR_CTRL = 1;
    BRDR_WIDTH = 31;
    BRDR_HEIGHT = 31;

    sprites_init();
    while (1) {

        game_tick();
        waitvbl();
        do_colourcycle();
        sprites_beginframe();
        game_render();
        {
            char buf[5];
            buf[0] = hexdigits[ps2_dualstick >> 4];
            buf[1] = hexdigits[ps2_dualstick & 0x07];
            buf[2] = ' ';
            buf[3] = hexdigits[ps2_menukeys >> 4];
            buf[4] = hexdigits[ps2_menukeys & 0x07];
            plat_textn(SCREEN_TEXT_W-5, 0, buf, 5, 3);
        }
        sprites_endframe();

    }

    return 0;
}

static inline uint8_t *chrmem(uint8_t cx, uint8_t cy)
{
    return ((uint8_t*)0xC000) + (cy*40) + cx;
}



// text layer. This is persistent - if you draw text it'll stay onscreen until
// overwritten, or plat_clr() is called.
void plat_clr()
{
    MMU_IO_CTRL = VICKY_IO_PAGE_CHAR_MEM;
    uint8_t* dest = chrmem(0,0);
    for (uint8_t cy = 0; cy<SCREEN_TEXT_H; ++cy) {
        for (uint8_t cx = 0; cx<SCREEN_TEXT_W; ++cx) {
            *dest++ = ' ';
        }
    }
    /*
    MMU_IO_CTRL = VICKY_IO_PAGE_ATTR_MEM;
    for (uint8_t cy = 0; cy<SCREEN_TEXT_H; ++cy) {
        uint8_t* dest = chrmem(0,cy);
        for (uint8_t cx = 0; cx<SCREEN_TEXT_W; ++cx) {
            *dest++ = 1;
        }
    }
    */
}

// draw len number of chars
void plat_textn(uint8_t cx, uint8_t cy, const char* txt, uint8_t len, uint8_t colour)
{
    MMU_IO_CTRL = VICKY_IO_PAGE_CHAR_MEM;
    uint8_t* dest = chrmem(cx,cy);
    if (colour==0) {
        for (int i=0; i<len; ++i) {
            *dest++ = 0;
        }
    } else {
        for (int i=0; i<len; ++i) {
            *dest++ = *txt++;
        }
        MMU_IO_CTRL = VICKY_IO_PAGE_ATTR_MEM;
        dest = chrmem(cx,cy);
        for (int i=0; i<len; ++i) {
            *dest++ = colour<<4;
        }
    }

}

// TODO: should just pass in level and score as bcd
void plat_hud(uint8_t level, uint8_t lives, uint32_t score)
{
    const uint8_t cx = 0;
    const uint8_t cy = 0;
    uint8_t c = 1;
    char buf[16];

    {
        uint8_t level_bcd = bin2bcd8(level);

        buf[0] = 'L';
        buf[1] = 'V';
        buf[2] = ' ';

        buf[3] = hexdigits[level_bcd >> 4];
        buf[4] = hexdigits[level_bcd & 0x0f];
    }

    buf[5] = ' ';
    buf[6] = ' ';

    {
        uint32_t score_bcd = bin2bcd32(score);

        buf[7] = hexdigits[(score_bcd >> 28)&0x0f];
        buf[8] = hexdigits[(score_bcd >> 24)&0x0f];
        buf[9] = hexdigits[(score_bcd >> 20)&0x0f];
        buf[10] = hexdigits[(score_bcd >> 16)&0x0f];
        buf[11] = hexdigits[(score_bcd >> 12)&0x0f];
        buf[12] = hexdigits[(score_bcd >> 8)&0x0f];
        buf[13] = hexdigits[(score_bcd >> 4)&0x0f];
        buf[14] = hexdigits[(score_bcd >> 0)&0x0f];
    }

    plat_textn(cx, cy, buf, 15, 1);

    // Lives
    {
        uint8_t i=0;
        for(i=0; i < 7 && i < lives; ++i) {
            buf[i] = '*';  // heart
        }
        buf[i] = (lives > 7) ? '+' : ' ';
        ++i;
        plat_textn(cx+17, cy, buf, i, 2);
    }
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

static uint8_t nsprites = 0;
static uint8_t nsprites_prev = 0;

static void sprites_init()
{
    nsprites = 0;
    nsprites_prev = 0;
}

static void sprites_beginframe()
{
    nsprites = 0;
}

static void sprites_endframe()
{
    // clear any additional sprites used on the previous frame
    MMU_IO_CTRL = VICKY_IO_PAGE_REGISTERS;
    uint8_t *spr = (uint8_t*)(0xD900 + (nsprites * 8));
    for (uint8_t i = nsprites; i < nsprites_prev; ++i) {
        *spr = 0;
        spr += 8;
    }

    nsprites_prev = nsprites;
}


// sprite ctrl byte:
// -ssllcce
// ss: size 00=32x32 01=24x24 10=16x16 11=8x8
// ll: layer
// cc: lut
// e: enable
static void sprout_internal(int16_t x, int16_t y, uint8_t ctrl, uint32_t imgaddr)
{
    if (nsprites>=64) {
        return;
    }
    MMU_IO_CTRL = VICKY_IO_PAGE_REGISTERS;
    uint8_t *spr = (uint8_t*)(0xD900 + (nsprites*8));
    *spr++ = ctrl;
    *spr++ = imgaddr & 0xff;
    *spr++ = imgaddr >> 8;
    *spr++ = (imgaddr >> 16);

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

void sprout16(int16_t x, int16_t y, uint8_t img)
{
    uint32_t addr =(uint32_t)(SPR16_00_START + (img*16*16));
    uint8_t ctrl = 0b01000001;  // 16x16, clut #0, enable
    sprout_internal(x,y,ctrl,addr);
}

void sprout16_highlight(int16_t x, int16_t y, uint8_t img)
{
    uint32_t addr =(uint32_t)(SPR16_00_START + (img*16*16));
    uint8_t ctrl = 0b01000011;  // 16x16, clut #1, enable
    sprout_internal(x,y,ctrl,addr);
}

void sprout32(int16_t x, int16_t y, uint8_t img)
{
    uint32_t addr =(uint32_t)(SPR32_START + (img*32*32));
    uint8_t ctrl = 0b00000001;  // 32x32, clut #0, enable
    sprout_internal(x,y,ctrl,addr);
}

void sprout32_highlight(int16_t x, int16_t y, uint8_t img)
{
    uint32_t addr =(uint32_t)(SPR32_START + (img*32*32));
    uint8_t ctrl = 0b00000011;  // 32x32, clut #1, enable
    sprout_internal(x,y,ctrl,addr);
}

void sprout64x8(int16_t x, int16_t y, uint8_t img )
{
    // use 8 8x8 sprites (!!!)
    uint32_t addr =(uint32_t)(SPR64x8_START + (img*8*8*8));
    uint8_t ctrl = 0b01100001;  // 8x8, clut #0, enable
    sprout_internal(x,y,ctrl,addr);
    x += 8<<FX;
    addr += 8*8;
    sprout_internal(x,y,ctrl,addr);
    x += 8<<FX;
    addr += 8*8;
    sprout_internal(x,y,ctrl,addr);
    x += 8<<FX;
    addr += 8*8;
    sprout_internal(x,y,ctrl,addr);
    x += 8<<FX;
    addr += 8*8;
    sprout_internal(x,y,ctrl,addr);
    x += 8<<FX;
    addr += 8*8;
    sprout_internal(x,y,ctrl,addr);
    x += 8<<FX;
    addr += 8*8;
    sprout_internal(x,y,ctrl,addr);
    x += 8<<FX;
    addr += 8*8;
    sprout_internal(x,y,ctrl,addr);
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

/*
 * SOUND
 */

void sfx_init()
{
}

void sfx_tick(uint8_t frametick)
{
}

void sfx_play(uint8_t effect, uint8_t pri)
{
}

uint8_t sfx_continuous = SFX_NONE;

/*
 * INPUT
 */

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

    return out | ps2_dualstick;
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
    return out | ps2_menukeys;
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





