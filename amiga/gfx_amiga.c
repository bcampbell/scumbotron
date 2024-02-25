#include "support/gcc8_c_support.h"
#include <proto/exec.h>
//#include <proto/dos.h>
//#include <proto/graphics.h>
//#include <graphics/gfxbase.h>
//#include <graphics/view.h>
#include <exec/execbase.h>
//#include <graphics/gfxmacros.h>
#include <hardware/custom.h>
#include <hardware/dmabits.h>
#include <hardware/intbits.h>
#include <proto/graphics.h> // for WaitBlit()
#include <hardware/blit.h>  // for blitter defines

#include "../plat.h"
#include "../misc.h"
#include "gfx_amiga.h"

#include <stddef.h>

/***
 */

typedef struct DamageEntry
{
    size_t offset;   // screen offset (with bit0 clear)
    UWORD w_words;   // width in words
    UWORD h_lines;        // hight in lines
} DamageEntry;

#define MAX_DAMAGEENTRIES 128

typedef struct DamageList
{
    uint16_t nentries;
    DamageEntry entries[MAX_DAMAGEENTRIES];
} DamageList;


static void init_sprite_data();
static void init_copperlist();

// Some chipmem allocations.
static uint8_t *screenbuf[2] = {NULL, NULL};
static uint8_t *textbuf = NULL;
static uint8_t *spr16_mem = NULL;
static uint8_t *spr32_mem = NULL;
static uint8_t *spr64x8_mem = NULL;
static UWORD *copperlist_mem = NULL;

static UWORD* cop_bpl_bookmark;
static UWORD* cop_color15_bookmark;
static UWORD palette[16];

static uint8_t *backbuf = NULL;
static uint8_t *frontbuf = NULL;

static DamageList damagelists[2] = {0};
DamageList *front_damage = NULL;
DamageList *back_damage = NULL;


static bool grab_chipmem()
{
    copperlist_mem = (UWORD*)AllocMem(COPPERLIST_MEMSIZE, MEMF_CHIP);
    if (!copperlist_mem) {
        return false;
    }
    for (int i=0; i<2; ++i) {
        screenbuf[i] = (uint8_t*)AllocMem(SCREEN_MEMSIZE, MEMF_CHIP);
        if (!screenbuf[i]) {
            return false;
        }
    }
    frontbuf = screenbuf[0];
    front_damage = &damagelists[0];
    backbuf = screenbuf[1];
    back_damage = &damagelists[1];



    textbuf = (uint8_t*)AllocMem(SCREEN_MEMSIZE, MEMF_CHIP);
    if (!textbuf) {
        return false;
    }

    // *2 to account for mask!
    spr16_mem = (uint8_t*)AllocMem(SPR16_SIZE * SPR16_NUM * 2, MEMF_CHIP);
    if (!spr16_mem) {
        return false;
    }
    spr32_mem = (uint8_t*)AllocMem(SPR32_SIZE * SPR32_NUM * 2, MEMF_CHIP);
    if (!spr32_mem) {
        return false;
    }
    spr64x8_mem = (uint8_t*)AllocMem(SPR64x8_SIZE * SPR64x8_NUM * 2, MEMF_CHIP);
    if (!spr64x8_mem) {
        return false;
    }
    return true;
}

static void free_chipmem()
{
    if (copperlist_mem) {
        FreeMem(copperlist_mem, COPPERLIST_MEMSIZE);
        copperlist_mem = NULL;
    }
    for (int i=0; i<2; ++i) {
        if (screenbuf[i]) {
            FreeMem(screenbuf[i], SCREEN_MEMSIZE);
            screenbuf[i] = NULL;
        }
    }
    if (textbuf) {
        FreeMem(textbuf, SCREEN_MEMSIZE);
        textbuf = NULL;
    }
    if (spr16_mem) {
        FreeMem(spr16_mem, SPR16_SIZE * SPR16_NUM * 2);
        spr16_mem = NULL;
    }
    if (spr32_mem) {
        FreeMem(spr32_mem, SPR32_SIZE * SPR32_NUM * 2);
        spr32_mem = NULL;
    }
    if (spr64x8_mem) {
        FreeMem(spr64x8_mem, SPR64x8_SIZE * SPR64x8_NUM * 2);
        spr64x8_mem = NULL;
    }
}



static UWORD *copper_emit_bitplanes(UWORD* cl, uint8_t* screen)
{
    uint8_t *plane = screen;
    for (int i = 0; i < 4; ++i) {
        *cl++ = CUSTOM_OFFSET(bplpt[i]);
        *cl++ = (UWORD)((ULONG)plane >> 16);
        *cl++ = CUSTOM_OFFSET(bplpt[i]) + 2;
        *cl++ = (UWORD)((ULONG)plane & 0xffff);
        plane += SCREEN_PLANE_STRIDE;
    }
    return cl;
}


static UWORD *copper_emit_color15(UWORD* cl, UWORD colour)
{
    *cl++ = CUSTOM_OFFSET(color[15]);
    *cl++ = colour;
    return cl;
}


static void init_copperlist()
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

        cop_bpl_bookmark = cl;
        cl = copper_emit_bitplanes(cl, frontbuf);
        cop_color15_bookmark = cl;
        cl = copper_emit_color15(cl, palette[15]);
#if 0
//        *cl++ = CUSTOM_OFFSET(color[0]);
//        *cl++ = 0x0000;
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
#endif

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

        // start running.
        custom->cop1lc = (ULONG)copperlist_mem;
        custom->copjmp1 = 0x7fff;
    }
}


// exported spritedata has no mask, and may not be in chipmem.
static void init_sprite_data()
{
    {
        // Exported data is image only, and we'll generate a mask here on the fly.
        // Images are 4 bitplanes interleaved.
        const UWORD *src = (const UWORD*)export_spr16_bin;
        UWORD *dest = (UWORD*)(spr16_mem);
        // For each 16x16 sprite...
        for (int n = 0; n < SPR16_NUM; ++n) {
            UWORD *mask = dest + (SPR16_SIZE/2);    // mask follows bitplanes
            for (int y=0; y<16; ++y) {
                // Build the mask on the fly.
                UWORD bits = 0x0000;
                for(int i=0; i<4; ++i) {
                    bits |= *src;
                    *dest++ = *src++;
                }
                // same mask for all bitplanes
                for(int i=0; i<4; ++i) {
                    *mask++ = bits;
                }
            }
            dest += (SPR16_SIZE/2); // skip mask
        }
    }
    {
        const UWORD *src = (const UWORD*)export_spr32_bin;
        UWORD *dest = (UWORD*)(spr32_mem);
        // For each 32x32 sprite...
        for (int n = 0; n < SPR32_NUM; ++n) {
            UWORD *mask = dest + (SPR32_SIZE/2);    // mask follows bitplanes
            for (int y = 0; y < 32; ++y) {
                for( int x = 0; x < 32; x+= 16) {
                    // Build the mask on the fly.
                    UWORD bits = 0x0000;
                    for(int i=0; i<4; ++i) {
                        bits |= *src;
                        *dest++ = *src++;
                    }
                    // same mask for all bitplanes
                    for(int i=0; i<4; ++i) {
                        *mask++ = bits;
                    }
                }
            }
            dest += (SPR32_SIZE/2); // skip mask
        }
    }
}

bool gfx_init()
{
    if (!grab_chipmem()) {
        return false;
    }
    // convert palette to amiga format 
    for (int i = 0; i < 16; ++i) {
        const uint8_t *c = &export_palette_bin[i*4]; // rgba
        palette[i] = (c[0]>>4)<<8 | (c[1]>>4)<<4 | (c[2]>>4);
        custom->color[i] = palette[i];
    }


    init_sprite_data();
    init_copperlist();
    return true;
}


void gfx_shutdown()
{
    free_chipmem();
}


/******************************
 * blitqueue stuff.
 */

#define BLITOP_WORDCOPY 0
#define BLITOP_DRAWSPR 1

struct blitop_drawspr
{
    uint16_t kind;
    uint8_t* srcimg;
    uint8_t* srcmask;
    UWORD srcmod;
    uint8_t* dest;
    UWORD destmod;
    UWORD xshift;
    UWORD bltsize;
};

// copy word-aligned data
struct blitop_wordcopy
{
    uint16_t kind;
    APTR bltapt;
    UWORD bltamod;
    APTR bltdpt;
    UWORD bltdmod;
    UWORD bltsize;
};

union blitop
{
    uint16_t kind;
    struct blitop_drawspr drawspr;
    struct blitop_wordcopy wordcopy;
};


#define MAX_BLITOPS 128
int blitq_head = 0;
int blitq_tail = 0;

union blitop blitq[MAX_BLITOPS];

void blitq_do(union blitop *op) {
    switch (op->kind) {
        case BLITOP_WORDCOPY:
            {
                custom->bltcon0 = SRCA | DEST | 0xF0;
                custom->bltcon1 = 0;

                custom->bltapt = op->wordcopy.bltapt;
                custom->bltamod = op->wordcopy.bltamod;

                custom->bltdpt = op->wordcopy.bltdpt;
                custom->bltdmod = op->wordcopy.bltdmod;

                custom->bltafwm = 0xffff;
                custom->bltalwm = 0xffff;

                custom->bltsize = op->wordcopy.bltsize;
            }
            break;

        case BLITOP_DRAWSPR:
            {
                // blit a sprite image with mask
                UWORD xshift = op->drawspr.xshift;

                // a: srcmask b: srcimg c: background
                // function: d = (a & b) | c   => 0xCA
                custom->bltcon0 = SRCA | SRCB | SRCC | DEST | 0xCA | (xshift << ASHIFTSHIFT);
                custom->bltcon1 = xshift << BSHIFTSHIFT;
                custom->bltapt = op->drawspr.srcmask;
                custom->bltamod = op->drawspr.srcmod;

                custom->bltbpt = op->drawspr.srcimg;
                custom->bltbmod = op->drawspr.srcmod;

                custom->bltcpt = op->drawspr.dest;
                custom->bltcmod = op->drawspr.destmod;

                custom->bltdpt = op->drawspr.dest;
                custom->bltdmod = op->drawspr.destmod;

                custom->bltafwm = 0xffff;
                custom->bltalwm = 0x0000;

                // hhhhhhhh hhwwwwww
                // h: lines
                // w: words
                custom->bltsize = op->drawspr.bltsize;
            }
            break;
    }
}

// Draw len number of chars
void plat_textn(uint8_t cx, uint8_t cy, const char* txt, uint8_t len, uint8_t colour)
{
    size_t offset = (cy * 8 * SCREEN_LINE_STRIDE) + cx;

    uint8_t* dest = textbuf + offset;

    // Record the damage.
    if(back_damage->nentries<MAX_DAMAGEENTRIES) {
        DamageEntry* d = &back_damage->entries[back_damage->nentries++];
        d->offset = offset;
        d->w_words = (len+1)/2;
        d->h_lines = 8;
    }
    if(front_damage->nentries<MAX_DAMAGEENTRIES) {
        DamageEntry* d = &front_damage->entries[front_damage->nentries++];
        d->offset = offset;
        d->w_words = (len+1)/2;
        d->h_lines = 8;
    }

    for( ; len>0; --len) {
        uint8_t chr = (uint8_t)*txt++;
        const uint8_t* src = export_chars_bin + (chr * 8);
#if 0
        // add to queue
        struct blitop_drawchar *op = &(blitq[blitq_head].drawchar);
        op->kind = BLITOP_DRAWCHAR;
        op->xshift = (cx & 0x01) ? 8 : 0;
        op->dest = dest;    // & 0xffffffe;
        op->srcimg = (uint8_t *)(spr16_mem);
        op->srcmask = op->srcimg;
        ++blitq_head;
        custom->intreq = INTF_SETCLR | INTF_BLIT; // go!
#endif
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

// Clear text
void plat_clr()
{
    memset(textbuf, 0, SCREEN_MEMSIZE);
}

// Render bonkers encoding where each byte encodes a 4x2 block of pixels,
// to be rendered via charset.
void plat_mono4x2(uint8_t cx, int8_t cy, const uint8_t* src, uint8_t cw, uint8_t ch, uint8_t basecol)
{
}

void gfx_startrender()
{
    {
        char buf[8];
        buf[0] = hexdigits[blitq_head >> 4];
        buf[1] = hexdigits[blitq_head & 0x0f];
        buf[2] = hexdigits[blitq_tail >> 4];
        buf[3] = hexdigits[blitq_tail & 0x0f];
        plat_textn(0,20,buf,4,15);
    }
    // reset blit queue
    blitq_head = 0;
    blitq_tail = 0;
    // clear screen.
    for (int i = 0; i < back_damage->nentries; ++i) {
        DamageEntry *ent = &back_damage->entries[i];

        struct blitop_wordcopy *op = &(blitq[blitq_head].wordcopy);
        op->kind = BLITOP_WORDCOPY;
        op->bltapt = textbuf + ent->offset;
        op->bltamod = SCREEN_PLANE_STRIDE - (ent->w_words*2);
        op->bltdpt = backbuf + ent->offset;
        op->bltdmod = SCREEN_PLANE_STRIDE - (ent->w_words*2);
        op->bltsize = ((ent->h_lines*4) << 6) | ent->w_words;
        ++blitq_head;
    }
    custom->intreq = INTF_SETCLR | INTF_BLIT; // go!
    back_damage->nentries = 0;
}


bool is_blitter_busy() {
    // On early Agnus chips, the blitter-busy flag might not be set
    // immediately upon the write to BLTSIZ.
    // HW ref manual suggests that reading it twice will do the trick.
    // Silly name. BLTDONE is set if busy (aka 'BBUSY').
    UWORD tst=*(volatile UWORD*)&custom->dmaconr;
    (void)tst;
    return (*(volatile UWORD*)&custom->dmaconr&(1<<14));// ? true:false;
}

void gfx_present_frame()
{
    // Wait until all the blitting is done
//    while (blitq_tail < blitq_head) {
//        if (!is_blitter_busy()) {
//            custom->intreq = INTF_SETCLR | INTF_BLIT; // kick!!!
//       }
//    }

    // swap buffers and damagelists
    {
        uint8_t *btmp = backbuf;
        backbuf = frontbuf;
        frontbuf = btmp;
        DamageList* dtmp = back_damage;
        back_damage = front_damage;
        front_damage = dtmp;
    }

    // show the front buffer
    copper_emit_bitplanes(cop_bpl_bookmark, frontbuf);
    // update color15
    copper_emit_color15(cop_color15_bookmark, palette[tick & 0x0f]);
}

void gfx_blit_irq_handler()
{
    if (is_blitter_busy()) {
        // still busy. wait for next interrupt.
        return;
    }

    if (blitq_tail < blitq_head) {
        int i = blitq_tail;
        ++blitq_tail;
        blitq_do(&blitq[i]);
    }
}

void sprout16(int16_t x, int16_t y, uint8_t img)
{
    x = x >> FX;
    y = y >> FX;
    if (x<0 || y<0 || x >= (SCREEN_W-16) || y >= (SCREEN_H-16)) {
        return;
    }
    if (blitq_head == MAX_BLITOPS) {
        return; // TOO MANY!
    }

    size_t offset = (y * SCREEN_LINE_STRIDE) + (2 * (x / 16));

    // Add to blit queue.
    if (blitq_head < MAX_BLITOPS) {
        const UWORD w_words = 2;  // 2 words to account for shifting.
        struct blitop_drawspr *op = &(blitq[blitq_head].drawspr);
        op->kind = BLITOP_DRAWSPR;
        op->xshift = x & 0xf;
        op->dest = backbuf + offset;
        op->destmod = SCREEN_PLANE_STRIDE - (w_words*2);
        op->srcimg = (uint8_t *)(spr16_mem + (img * SPR16_SIZE * 2));
        op->srcmask = op->srcimg + SPR16_SIZE;
        op->srcmod = 2 - (w_words*2);
        op->bltsize = ((16*4)<<6) | w_words;

        ++blitq_head;
        custom->intreq = INTF_SETCLR | INTF_BLIT; // go!
    }
    // Record the damage.
    if(back_damage->nentries<MAX_DAMAGEENTRIES) {
        DamageEntry* d = &back_damage->entries[back_damage->nentries++];
        d->offset = offset;
        d->w_words = (x & 0xF) ? 2 : 1;
        d->h_lines = 16;
    }
}

void sprout16_highlight(int16_t x, int16_t y, uint8_t img)
{
}

void sprout32(int16_t x, int16_t y, uint8_t img)
{
    x = x >> FX;
    y = y >> FX;
    if (x<0 || y<0 || x >= (SCREEN_W-32) || y >= (SCREEN_H-32)) {
        return;
    }
    if (blitq_head == MAX_BLITOPS) {
        return; // TOO MANY!
    }

    size_t offset = (y * SCREEN_LINE_STRIDE) + (2 * (x / 16));

    // Add to blit queue.
    if (blitq_head < MAX_BLITOPS) {
        const UWORD w_words = 3;  // 3 words to account for shifting.
        struct blitop_drawspr *op = &(blitq[blitq_head].drawspr);
        op->kind = BLITOP_DRAWSPR;
        op->xshift = x & 0xf;
        op->dest = backbuf + offset;
        op->destmod = SCREEN_PLANE_STRIDE - (w_words*2);
        op->srcimg = (uint8_t *)(spr32_mem + (img * SPR32_SIZE * 2));
        op->srcmask = op->srcimg + SPR32_SIZE;
        op->srcmod = 4 - (w_words*2);
        op->bltsize = ((32*4)<<6) | w_words;

        ++blitq_head;
        custom->intreq = INTF_SETCLR | INTF_BLIT; // go!
    }
    // Record the damage.
    if(back_damage->nentries<MAX_DAMAGEENTRIES) {
        DamageEntry* d = &back_damage->entries[back_damage->nentries++];
        d->offset = offset;
        d->w_words = (x & 0xF) ? 3 : 2;
        d->h_lines = 32;
    }
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


// Draw horizontal line of chars, range [cx_begin, cx_end).
void plat_hline_noclip(uint8_t cx_begin, uint8_t cx_end, uint8_t cy, uint8_t chr, uint8_t colour)
{
}


// Draw vertical line of chars, range [cy_begin, cy_end).
void plat_vline_noclip(uint8_t cx, uint8_t cy_begin, uint8_t cy_end, uint8_t chr, uint8_t colour)
{
}

