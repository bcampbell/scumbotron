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
 *
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
static void build_copperlist(UWORD *cl);

// Some chipmem allocations.
static uint8_t *screenbuf[2] = {NULL, NULL};    // Main screen buffers
static uint8_t *bgbuf = NULL;                   // Background buffer
static uint8_t *spr16_mem = NULL;
static uint8_t *spr32_mem = NULL;
static uint8_t *spr64x8_mem = NULL;
static UWORD *copperlist_mem = NULL;

static UWORD* cop_bpl_bookmark;     // Where copperlist sets bitplane addrs
static UWORD* cop_color15_bookmark; // Where copperlist sets colour 15
static UWORD palette[16];           // Master palette in amiga xrgb format

// We also need a damage list for each buffer, for erasing.
// To erase, we blit chunks of the bgbuf over the damaged area.
static DamageList damagelists[2] = {0};

// front is the screen we're displaying, back is the one we're
// drawing into. These will be swapped each frame.
static uint8_t *backbuf = NULL;
static DamageList *front_damage = NULL;
static uint8_t *frontbuf = NULL;
static DamageList *back_damage = NULL;


// Allocate all the chipmem we need.
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

    bgbuf = (uint8_t*)AllocMem(SCREEN_MEMSIZE, MEMF_CHIP);
    if (!bgbuf) {
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
    if (bgbuf) {
        FreeMem(bgbuf, SCREEN_MEMSIZE);
        bgbuf = NULL;
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



// Output copper instructions for setting bitplane addrs for screen.
// (used to edit the copperlist each frame for double buffering)
static UWORD *copper_emit_bitplanes(UWORD* cl, uint8_t* screen)
{
    // Show (SCREEN_XPAD, SCREEN_YPAD) at top left.
    uint8_t *plane = screen + (SCREEN_YPAD * SCREEN_LINE_STRIDE) + (SCREEN_XPAD/8);
    for (int i = 0; i < 4; ++i) {
        *cl++ = CUSTOM_OFFSET(bplpt[i]);
        *cl++ = (UWORD)((ULONG)plane >> 16);
        *cl++ = CUSTOM_OFFSET(bplpt[i]) + 2;
        *cl++ = (UWORD)((ULONG)plane & 0xffff);
        plane += SCREEN_PLANE_STRIDE;
    }
    return cl;
}


// Output copper instruction for setting color 15.
// Used for flashing colour.
static UWORD *copper_emit_color15(UWORD* cl, UWORD colour)
{
    *cl++ = CUSTOM_OFFSET(color[15]);
    *cl++ = colour;
    return cl;
}


// Assumes there is enough chipmem at cl to fit it!
static void build_copperlist(UWORD *cl)
{
    // set up display window
    *cl++ = CUSTOM_OFFSET(diwstrt);
    *cl++ = (0x2c << 8) | 0x81;    // recommended topleft yyyyyyyyxxxxxxxx 

    *cl++ = CUSTOM_OFFSET(diwstop);
    *cl++ = ((0x2c + SCREEN_H - 256) << 8) | (0x81 + SCREEN_W - 256);
    *cl++ = CUSTOM_OFFSET(diwhigh);
    *cl++ = (1 << 13) | (1 << 8);

    // Set data fetch for lowres, as per hw reference manual
    *cl++ = CUSTOM_OFFSET(ddfstrt);
    *cl++ = 0x0038;
    *cl++ = CUSTOM_OFFSET(ddfstop);
    *cl++ = 0x00D0;

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
    *cl++ = (SCREEN_PLANE_STRIDE * 3) + ((2 * SCREEN_XPAD) / 8);
    *cl++ = CUSTOM_OFFSET(bpl2mod);
    *cl++ = (SCREEN_PLANE_STRIDE * 3) + ((2 * SCREEN_XPAD) / 8);

    cop_bpl_bookmark = cl;
    cl = copper_emit_bitplanes(cl, frontbuf);
    cop_color15_bookmark = cl;
    cl = copper_emit_color15(cl, palette[15]);

    // end.
    *cl++ = 0xFFFF;
    *cl++ = 0xFFFE;
}


// exported spritedata has no mask, and may not be in chipmem.
// TODO: just export proper data, and compress it into the exe?
// Or just link it directly into chipmem for startup.
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
    // convert 32bit rgba palette to 16bit xrgb amiga format 
    for (int i = 0; i < 16; ++i) {
        const uint8_t *c = &export_palette_bin[i*4]; // byte per component
        palette[i] = (c[0]>>4)<<8 | (c[1]>>4)<<4 | (c[2]>>4);
        custom->color[i] = palette[i];
    }


    init_sprite_data();
    // Create and start the copperlist.
    build_copperlist(copperlist_mem);
    custom->cop1lc = (ULONG)(copperlist_mem);
    custom->copjmp1 = 0x7fff;
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
#define BLITOP_HZAPPER 2
#define BLITOP_VZAPPER 3
#define BLITOP_EFFECT 4

// sprite with mask
struct blitop_drawspr
{
    uint16_t kind;  // BLITOP_DRAWSPR
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
    uint16_t kind;  // BLITOP_WORDCOPY
    APTR bltapt;
    UWORD bltamod;
    APTR bltdpt;
    UWORD bltdmod;
    UWORD bltsize;
};

struct blitop_hzapper
{
    uint16_t kind;  // BLITOP_HZAPPER
    APTR bltdpt;
    UWORD bltdmod;
};

struct blitop_vzapper
{
    uint16_t kind;  // BLITOP_VZAPPER
    APTR bltdpt;    // first dest word
    UWORD xshift;   // 0-15, pixel within word
};

struct blitop_effect
{
    uint16_t kind;  // BLITOP_EFFECT
    UWORD bltadat;
    UWORD bltafwm;
    UWORD bltalwm;
    APTR bltdpt;
    UWORD bltdmod;
    UWORD bltsize;
};

union blitop
{
    uint16_t kind;
    struct blitop_drawspr drawspr;
    struct blitop_wordcopy wordcopy;
    struct blitop_hzapper hzapper;
    struct blitop_vzapper vzapper;
    struct blitop_effect effect;
};


#define MAX_BLITOPS 128
volatile int blitq_head = 0;
volatile int blitq_tail = 0;

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
        case BLITOP_HZAPPER:
            {
                // A constant FFFF
                // D dest
                custom->bltcon0 = DEST | 0xF0;
                custom->bltcon1 = 0;

                custom->bltadat = 0xFFFF;

                custom->bltdpt = op->hzapper.bltdpt;
                custom->bltdmod = op->hzapper.bltdmod;

                custom->bltafwm = 0xffff;
                custom->bltalwm = 0xffff;

                // hhhhhhhh hhwwwwww
                // 4 bitplanes high
                custom->bltsize = (4<<6) | (SCREEN_W/16);
            }
            break;
        case BLITOP_VZAPPER:
            {
                // A existing bg
                // B constant, the pattern we want to blit
                // D dest
                custom->bltcon0 = SRCA | DEST | 0b11111100;
                custom->bltcon1 = 0;

                custom->bltapt = op->vzapper.bltdpt;
                custom->bltamod = SCREEN_PLANE_STRIDE - 2;

                // the constant word image
                custom->bltbdat = 0x8000 >> op->vzapper.xshift;

                custom->bltdpt = op->vzapper.bltdpt;
                custom->bltdmod = SCREEN_PLANE_STRIDE - 2;

                custom->bltafwm = 0xffff;
                custom->bltalwm = 0xffff;

                // hhhhhhhh hhwwwwww
                // full screen height, 1 word wide
                // FUDGE to avoid bltsize height overflow!!!
                custom->bltsize = (((SCREEN_H-1)*4)<<6) | 1;
            }
            break;
        case BLITOP_EFFECT:
            {
                // A constant (masked by fwm/lwm)
                // B existing screen
                // D dest

                custom->bltcon0 = SRCB | DEST | 0b11111100;
                custom->bltcon1 = 0;

                custom->bltadat = op->effect.bltadat;
                custom->bltafwm = op->effect.bltafwm;
                custom->bltalwm = op->effect.bltalwm;

                custom->bltbpt = op->effect.bltdpt;
                custom->bltbmod = op->effect.bltdmod;

                custom->bltdpt = op->effect.bltdpt;
                custom->bltdmod = op->effect.bltdmod;

                custom->bltsize = op->effect.bltsize;
            }
            break;

    }
}

// Draw len number of chars
void plat_textn(uint8_t cx, uint8_t cy, const char* txt, uint8_t len, uint8_t colour)
{
    cx += (SCREEN_XPAD / 8);
    cy += (SCREEN_YPAD / 8);
    size_t offset = (cy * 8 * SCREEN_LINE_STRIDE) + cx;
    uint8_t* dest = bgbuf + offset;

    size_t wordstart = offset / 2;
    size_t wordend = (offset + len + 1) / 2;

    // Record the damage.
    if(back_damage->nentries<MAX_DAMAGEENTRIES) {
        DamageEntry* d = &back_damage->entries[back_damage->nentries++];
        d->offset = wordstart * 2;
        d->w_words = wordend - wordstart;
        d->h_lines = 8;
    }
    if(front_damage->nentries<MAX_DAMAGEENTRIES) {
        DamageEntry* d = &front_damage->entries[front_damage->nentries++];
        d->offset = wordstart * 2;
        d->w_words = wordend - wordstart;
        d->h_lines = 8;
    }

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

// Clear text
void plat_clr()
{
    // Don't need to set offscreen area, but hey.
    memset(bgbuf, 0, SCREEN_MEMSIZE);
}

// Render bonkers encoding where each byte encodes a 4x2 block of pixels,
// to be rendered via charset.
void plat_mono4x2(uint8_t cx, int8_t cy, const uint8_t* src, uint8_t cw, uint8_t ch, uint8_t basecol)
{
}

void gfx_startrender()
{
#if 0
    {
        char buf[8];
        buf[0] = hexdigits[blitq_head >> 4];
        buf[1] = hexdigits[blitq_head & 0x0f];
        buf[2] = hexdigits[blitq_tail >> 4];
        buf[3] = hexdigits[blitq_tail & 0x0f];
        plat_textn(0,20,buf,4,15);
    }
#endif
    // reset blit queue
    blitq_head = 0;
    blitq_tail = 0;
    // clear screen.
    for (int i = 0; i < back_damage->nentries; ++i) {
        DamageEntry *ent = &back_damage->entries[i];

        struct blitop_wordcopy *op = &(blitq[blitq_head].wordcopy);
        op->kind = BLITOP_WORDCOPY;
        op->bltapt = bgbuf + ent->offset;
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

void gfx_wait_for_blitting()
{
    UWORD c = 0;
        while (is_blitter_busy()) {
            //custom->color[0] = c++;
        }

    // Wait until all the blitting is done
//        if (!is_blitter_busy()) {
//            custom->intreq = INTF_SETCLR | INTF_BLIT; // kick!!!
//       }
//    }

}

void gfx_present_frame()
{
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
        //custom->color[0] = 0x004;
        return;
    }

    if (blitq_tail < blitq_head) {
        int i = blitq_tail;
        ++blitq_tail;
        //custom->color[0] = 0x00f;
        blitq_do(&blitq[i]);
        //custom->color[0] = 0x008;
    } else {
        //custom->color[0] = 0x000;
    }
}

static void generic_sprout(int16_t x, int16_t y,
    const uint8_t* srcimg, const uint8_t* srcmask,
    int16_t w_words, uint16_t h_lines) {

    if (blitq_head == MAX_BLITOPS) {
        return; // TOO MANY!
    }
    x = (x >> FX) + SCREEN_XPAD;
    y = (y >> FX) + SCREEN_YPAD;
    if (x < 0 || y < 0 || x >= (SCREEN_XPAD + SCREEN_W) || y >= (SCREEN_YPAD + SCREEN_H)) {
        return; // Totally offscreen
    }

    size_t offset = (y * SCREEN_LINE_STRIDE) + (2 * (x / 16));

    // Add to blit queue.
    if (blitq_head < MAX_BLITOPS) {
        struct blitop_drawspr *op = &(blitq[blitq_head].drawspr);
        op->kind = BLITOP_DRAWSPR;
        op->xshift = x & 0xf;
        op->dest = backbuf + offset;
        op->destmod = SCREEN_PLANE_STRIDE - ((w_words+1)*2);
        op->srcimg = (uint8_t*)srcimg;
        op->srcmask = (uint8_t*)srcmask;
        op->srcmod = -2;
        op->bltsize = ((h_lines*4)<<6) | (w_words+1);

        ++blitq_head;
        custom->intreq = INTF_SETCLR | INTF_BLIT; // go!
    }
    // Record the damage.
    if(back_damage->nentries<MAX_DAMAGEENTRIES) {
        DamageEntry* d = &back_damage->entries[back_damage->nentries++];
        d->offset = offset;
        d->w_words = (x & 0xF) ? w_words + 1 : w_words;
        d->h_lines = h_lines;
    }
}

void sprout16(int16_t x, int16_t y, uint8_t img)
{
    const uint8_t *srcimg = (uint8_t *)(spr16_mem + (img * SPR16_SIZE * 2));
    const uint8_t *srcmask = srcimg + SPR16_SIZE;
    generic_sprout(x, y, srcimg, srcmask, 1, 16);
}

void sprout16_highlight(int16_t x, int16_t y, uint8_t img)
{
    const uint8_t *srcimg = (uint8_t *)(spr16_mem + (img * SPR16_SIZE * 2));
    const uint8_t *srcmask = srcimg + SPR16_SIZE;
    generic_sprout(x, y, srcmask, srcmask, 1, 16);
}

void sprout32(int16_t x, int16_t y, uint8_t img)
{
    const uint8_t *srcimg = (uint8_t *)(spr32_mem + (img * SPR32_SIZE * 2));
    const uint8_t *srcmask = srcimg + SPR32_SIZE;
    generic_sprout(x, y, srcimg, srcmask, 2, 32);
}

void sprout32_highlight(int16_t x, int16_t y, uint8_t img)
{
    const uint8_t *srcimg = (uint8_t *)(spr32_mem + (img * SPR32_SIZE * 2));
    const uint8_t *srcmask = srcimg + SPR32_SIZE;
    generic_sprout(x, y, srcmask, srcmask, 2, 32);
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
            break;
        case ZAPPER_ON:
            {
                uint16_t pixy = 8 + (uint16_t)(y>>FX) + SCREEN_YPAD;
                size_t offset = (pixy * SCREEN_LINE_STRIDE) + (SCREEN_XPAD/8);
                // Add to blit queue.
                if (blitq_head < MAX_BLITOPS) {
                    struct blitop_hzapper *op = &(blitq[blitq_head].hzapper);
                    op->kind = BLITOP_HZAPPER;
                    op->bltdpt = backbuf + offset;
                    op->bltdmod = (2*SCREEN_XPAD)/8;
                    ++blitq_head;
                    custom->intreq = INTF_SETCLR | INTF_BLIT; // go!
                }
                // Record the damage.
                if(back_damage->nentries<MAX_DAMAGEENTRIES) {
                    DamageEntry* d = &back_damage->entries[back_damage->nentries++];
                    d->offset = offset;
                    d->w_words = SCREEN_W/16;
                    d->h_lines = 1;
                }

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
            break;
        case ZAPPER_ON:
            {
                uint16_t xpix = 8 + (x>>FX) + SCREEN_XPAD;
                size_t offset = (SCREEN_YPAD * SCREEN_LINE_STRIDE) + (xpix/8);
                // Add to blit queue.
                if (blitq_head < MAX_BLITOPS) {
                    struct blitop_vzapper *op = &(blitq[blitq_head].vzapper);
                    op->kind = BLITOP_VZAPPER;
                    op->bltdpt = backbuf + offset;
                    op->xshift = xpix & 0x0f;
                    ++blitq_head;
                    custom->intreq = INTF_SETCLR | INTF_BLIT; // go!
                }
                // Record the damage.
                if(back_damage->nentries<MAX_DAMAGEENTRIES) {
                    DamageEntry* d = &back_damage->entries[back_damage->nentries++];
                    d->offset = offset;
                    d->w_words = 1;
                    d->h_lines = SCREEN_H*4;
                }

                sprout16(x, y, SPR16_VZAPPER_ON);
            }
            break;
    }

}


// return offset of word at x,y on screen (0,0 is topleft visible).
size_t offset_addr_word(int x, int y) {
    return ((SCREEN_YPAD + y) * SCREEN_LINE_STRIDE) +
        ((SCREEN_XPAD + x) / 16) * 2;
}


static void effect_fill(uint8_t cx_begin, uint8_t cx_end, uint8_t cy_begin, uint8_t cy_end, uint8_t colour) {

    UWORD w_begin = cx_begin/2;
    UWORD w_end = (cx_end+1)/2;

    UWORD w_words = (w_end - w_begin);
    UWORD h = (cy_end - cy_begin)*4;    // we skip odd lines
    if (!w_words || !h) {
        return;
    }
    // use masks to handle start/end on non-word boundaries
    UWORD bltafwm = (cx_begin & 1) ? 0x00FF : 0xFFFF;
    UWORD bltalwm = (cx_end & 1) ? 0xFF00 : 0xFFFF;

    size_t offset = offset_addr_word(cx_begin * 8, cy_begin * 8);

    // add a blit per bitplane
    size_t plane_offset = offset;
    for (int i = 0; i < 4; ++i) {
        if (blitq_head < MAX_BLITOPS) {
            struct blitop_effect *op = &(blitq[blitq_head].effect);
            op->bltadat = (colour & (1 << i)) ? 0xffff : 0x0000;
            op->bltafwm = bltafwm;
            op->bltalwm = bltalwm;
            op->kind = BLITOP_EFFECT;
            op->bltdpt = backbuf + plane_offset;
            op->bltdmod = (SCREEN_LINE_STRIDE*2) - (w_words*2);
            op->bltsize = (h << 6) | w_words;
            ++blitq_head;
            custom->intreq = INTF_SETCLR | INTF_BLIT; // go!
        }
        plane_offset += SCREEN_PLANE_STRIDE;
    }
    // Record the damage for whole area.
    if(back_damage->nentries<MAX_DAMAGEENTRIES) {
        DamageEntry* d = &back_damage->entries[back_damage->nentries++];
        d->offset = offset;
        d->w_words = w_words;
        d->h_lines = h*2;   // no skip facility on damage blits...
    }
}

// Draw horizontal line of chars, range [cx_begin, cx_end).
void plat_hline_noclip(uint8_t cx_begin, uint8_t cx_end, uint8_t cy, uint8_t chr, uint8_t colour)
{
    effect_fill(cx_begin, cx_end, cy, cy+1, colour);
}


// Draw vertical line of chars, range [cy_begin, cy_end).
void plat_vline_noclip(uint8_t cx, uint8_t cy_begin, uint8_t cy_end, uint8_t chr, uint8_t colour)
{
    effect_fill(cx, cx+1, cy_begin, cy_end, colour);
}

