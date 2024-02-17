#include "support/gcc8_c_support.h"
#include <proto/exec.h>
//#include <proto/dos.h>
//#include <proto/graphics.h>
//#include <graphics/gfxbase.h>
//#include <graphics/view.h>
#include <exec/execbase.h>
//#include <graphics/gfxmacros.h>
#include <hardware/custom.h>
//#include <hardware/dmabits.h>
//#include <hardware/intbits.h>

#include "../plat.h"

extern volatile struct Custom *custom;
extern uint8_t *screen_mem;
extern UWORD *copperlist_mem;



extern const unsigned char export_chars_bin[];


#define CUSTOM_OFFSET(X) offsetof(struct Custom, X)


// num of bytes to get to next bitplane (we're using interleaved bitplanes)
#define SCREEN_PLANE_STRIDE (SCREEN_W/8)

// num of bytes to get to next line (4 bitplanes)
#define SCREEN_LINE_STRIDE (4*SCREEN_PLANE_STRIDE)


bool gfx_init()
{
    // now build the copperlist
    {
        UWORD* cl = copperlist_mem;

        // set up display
        *cl++ = CUSTOM_OFFSET(diwstrt);
        *cl++ = (0x2c << 8) | 0x81;         // yyyyyyyyxxxxxxxx 

        *cl++ = CUSTOM_OFFSET(diwstop);
        *cl++ = ((0x2c + SCREEN_H - 256) << 8) | (0x81 + SCREEN_W - 256);
        *cl++ = CUSTOM_OFFSET(diwhigh);
        *cl++ = (1 << 13) | (1 << 8);

        *cl++ = CUSTOM_OFFSET(ddfstrt);
        *cl++ = 0x81 / 2;
        *cl++ = CUSTOM_OFFSET(ddfstop);
        *cl++ = (0x81 / 2) - 8 + (8 * (SCREEN_W / 16));

        *cl++ = CUSTOM_OFFSET(fmode);
        *cl++ = 0;

        *cl++ = CUSTOM_OFFSET(bplcon0);
        *cl++ = 4<<12;      // 4 bitplanes

        *cl++ = CUSTOM_OFFSET(bplcon1);
        *cl++ = 0;
    //        *cl++ = CUSTOM_OFFSET(bplcon2);
    //        *cl ++ = 0;
        *cl++ = CUSTOM_OFFSET(bplcon3);
        *cl ++ = (1<<6);    // lores sprites

        *cl++ = CUSTOM_OFFSET(bplcon4);
        *cl++ = 0x11;

        *cl++ = CUSTOM_OFFSET(bpl1mod);
        *cl++ = (SCREEN_W/8)*3;
        *cl++ = CUSTOM_OFFSET(bpl2mod);
        *cl++ = (SCREEN_W/8)*3;

        uint8_t *plane = screen_mem;
        for (int i = 0; i < 4; ++i) {
            *cl++ = CUSTOM_OFFSET(bplpt[i]);
            *cl++ = (UWORD)((ULONG)plane >> 16);
            *cl++ = CUSTOM_OFFSET(bplpt[i]) + 2;
            *cl++ = (UWORD)((ULONG)plane & 0xffff);
            plane += SCREEN_PLANE_STRIDE;
        }


        *cl++ = CUSTOM_OFFSET(color[0]);
        *cl++ = 0x0000;
        *cl++ = CUSTOM_OFFSET(color[1]);
        *cl++ = (15<<8) | (15<<4) | (15);
        *cl++ = CUSTOM_OFFSET(color[1]);
        *cl++ = (7<<8) | (7<<4) | (7);

        *cl++ = CUSTOM_OFFSET(color[2]);
        *cl++ = (15<<8) | (0<<4) | (0);
        *cl++ = CUSTOM_OFFSET(color[3]);
        *cl++ = (0<<8) | (15<<4) | (0);
        *cl++ = CUSTOM_OFFSET(color[4]);
        *cl++ = (0<<8) | (0<<4) | (15);


        *cl++ = CUSTOM_OFFSET(color[5]);
        *cl++ = (15<<8) | (15<<4) | (0);
        *cl++ = CUSTOM_OFFSET(color[6]);
        *cl++ = (0<<8) | (15<<4) | (15);
        *cl++ = CUSTOM_OFFSET(color[7]);
        *cl++ = (15<<8) | (0<<4) | (15);

        *cl++ = CUSTOM_OFFSET(color[8]);
        *cl++ = (7<<8) | (0<<4) | (0);
        *cl++ = CUSTOM_OFFSET(color[9]);
        *cl++ = (0<<8) | (7<<4) | (0);
        *cl++ = CUSTOM_OFFSET(color[10]);
        *cl++ = (0<<8) | (0<<4) | (7);

        *cl++ = CUSTOM_OFFSET(color[11]);
        *cl++ = (7<<8) | (7<<4) | (0);
        *cl++ = CUSTOM_OFFSET(color[12]);
        *cl++ = (0<<8) | (7<<4) | (7);
        *cl++ = CUSTOM_OFFSET(color[13]);
        *cl++ = (7<<8) | (0<<4) | (7);

        *cl++ = CUSTOM_OFFSET(color[14]);
        *cl++ = (12<<8) | (12<<4) | (12);
        *cl++ = CUSTOM_OFFSET(color[15]);
        *cl++ = (15<<8) | (15<<4) | (15);

/*
        *cl++ = 0x2001;
        *cl++ = 0xFF00;
        *cl++ = CUSTOM_OFFSET(color[0]);
        *cl++ = (15<<8) | (0<<4) | (0);

        *cl++ = 0x3001;
        *cl++ = 0xFF00;
        *cl++ = CUSTOM_OFFSET(color[0]);
        *cl++ = (0<<8) | (15<<4) | (0);
        
        *cl++ = 0x9001;
        *cl++ = 0xFF00;
        *cl++ = CUSTOM_OFFSET(color[0]);
        *cl++ = (0<<8) | (0<<4) | (15);

        *cl++ = 0xFFFF;
        *cl++ = 0xFFFE;
        *cl++ = CUSTOM_OFFSET(color[0]);
        *cl++ = (4<<8) | (4<<4) | (4);
*/

    }
    //custom->cop1lc = (ULONG)copperlist_mem;
    //custom->copjmp1 = 1;
    return true;
}



void gfx_shutdown()
{
}

// Draw len number of chars
void plat_textn(uint8_t cx, uint8_t cy, const char* txt, uint8_t len, uint8_t colour)
{
    uint8_t* dest = screen_mem + (cy * 8 * SCREEN_LINE_STRIDE) + cx;

    for( ; len>0; --len) {
        uint8_t chr = (uint8_t)*txt++;
        const uint8_t* src = export_chars_bin + (chr * 8);
        // for each bitplane:
        for (int i = 0; i < 4; ++i) {
            if (colour & (1<<i)) {
                dest[0 * SCREEN_LINE_STRIDE] = src[0];
                dest[1 * SCREEN_LINE_STRIDE] = src[1];
                dest[2 * SCREEN_LINE_STRIDE] = src[2];
                dest[3 * SCREEN_LINE_STRIDE] = src[3];
                dest[4 * SCREEN_LINE_STRIDE] = src[4];
                dest[5 * SCREEN_LINE_STRIDE] = src[5];
                dest[6 * SCREEN_LINE_STRIDE] = src[6];
                dest[7 * SCREEN_LINE_STRIDE] = src[7];
            } else {
                dest[0 * SCREEN_LINE_STRIDE] = 0;
                dest[1 * SCREEN_LINE_STRIDE] = 0;
                dest[2 * SCREEN_LINE_STRIDE] = 0;
                dest[3 * SCREEN_LINE_STRIDE] = 0;
                dest[4 * SCREEN_LINE_STRIDE] = 0;
                dest[5 * SCREEN_LINE_STRIDE] = 0;
                dest[6 * SCREEN_LINE_STRIDE] = 0;
                dest[7 * SCREEN_LINE_STRIDE] = 0;
            }
            dest += SCREEN_PLANE_STRIDE;
        }
        dest -= SCREEN_LINE_STRIDE;
        dest += 1;
    }
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
}

void plat_vzapper_render(int16_t x, int16_t y, uint8_t state)
{
}

