#include <neo/api.h>
#include <stdint.h>
#include <stdbool.h>
#include "../plat.h"
#include "../sfx.h"
//#include <stdio.h>

/* exported gfx */
#define SPR16_SIZE (8*16)   // 16x16, 4 bpp
//#define SPR16_SIZE (16*16)   // 16x16, 8 bpp
#define SPR16_NUM 128
#define SPR32_SIZE (16*32)   // 32x32, 4 bpp
//#define SPR32_SIZE (32*32)   // 32x32, 8 bpp
#define SPR32_NUM 3
//#define SPR64x8_SIZE (32*8) // 64x8, 4bpp
//#define SPR64x8_NUM 4
extern const unsigned char export_palette_bin[];
extern unsigned int export_palette_bin_len;
extern const unsigned char export_spr16_bin[];
extern unsigned int export_spr16_bin_len;
extern const unsigned char export_spr32_bin[];
extern unsigned int export_spr32_bin_len;

/* missing api */

#include <string.h>
#include <neo6502.h>
#include <kernel.h>
//#include <api-internal.h>

#define API_GROUP_BLITTER 12
#define API_FN_BLITTER_BUSY 1
#define API_FN_BLITTER_COPY 2

void neo_blitter_copy(uint8_t srckind, const void *src, uint8_t destkind, void *dest, uint16_t nbytes) {
    ControlPort.params[0] = srckind;
    ControlPort.params[1] = ((uint16_t)src) & 0xff;
    ControlPort.params[2] = ((uint16_t)src) >> 8;
    ControlPort.params[3] = destkind;
    ControlPort.params[4] = ((uint16_t)dest) & 0xff;
    ControlPort.params[5] = ((uint16_t)dest) >> 8;
    ControlPort.params[6] = nbytes & 0xff;
    ControlPort.params[7] = nbytes >> 8;
    KSendMessage(12,2); //API_GROUP_BLITTER, API_FN_BLITTER_COPY);
}

void neo_gfxdraw4bpp(const void* src, uint8_t w, uint8_t h, int16_t destx, int16_t desty) {
    ControlPort.params[0] = ((uint16_t)src) & 0xff;
    ControlPort.params[1] = ((uint16_t)src) >> 8;
    ControlPort.params[2] = w;
    ControlPort.params[3] = h;
    ControlPort.params[4] = ((uint16_t)destx) & 0xff;
    ControlPort.params[5] = ((uint16_t)destx) >> 8;
    ControlPort.params[6] = ((uint16_t)desty) & 0xff;
    ControlPort.params[7] = ((uint16_t)desty) >> 8;
    KSendMessage(API_GROUP_GRAPHICS,42);
}

void neo_blitter_complex(uint8_t action, const void *src, const void* dest) {
    ControlPort.params[0] = action;
    ControlPort.params[1] = ((uint16_t)src) & 0xff;
    ControlPort.params[2] = ((uint16_t)src) >> 8;
    ControlPort.params[3] = ((uint16_t)dest) & 0xff;
    ControlPort.params[4] = ((uint16_t)dest) >> 8;
    KSendMessage(12,3); //API_GROUP_BLITTER, complexblit);
}

void neo_blit_image(uint8_t action, const void *sourceArea, int16_t x, int16_t y, uint8_t destFmt) {
    ControlPort.params[0] = action;
    ControlPort.params[1] = ((uint16_t)sourceArea) & 0xff;
    ControlPort.params[2] = ((uint16_t)sourceArea) >> 8;
    ControlPort.params[3] = x & 0xff;
    ControlPort.params[4] = x >> 8;
    ControlPort.params[5] = y & 0xff;
    ControlPort.params[6] = y >> 8;
    ControlPort.params[7] = destFmt;
    KSendMessage(12,4); //API_GROUP_BLITTER, blitimage);
}

/* end missing api */

struct BlitterArea {
	uint16_t 	address;
	uint8_t 	page;
    uint8_t     padding;
	int16_t  	stride;
	uint8_t 	format;
	uint8_t 	transparent;
	uint8_t 	solid;
	uint8_t  	height;
	uint16_t  	width;
};
#define BLTFMT_BYTE 0		// whole byte
#define BLTFMT_PAIR 1		// 2 4-bit values (nibbles) (src only)
#define BLTFMT_BITS 2		// 8 1-bit values (src only)
#define BLTFMT_HIGH 3		// High nibble (target only)
#define BLTFMT_LOW  4		// Low nibble (target only)

#define BLTACT_COPY 0		// Straight rectangle copy
#define BLTACT_MASK 1		// Copy, but only where src != srcarea.v (transparent value)
#define BLTACT_SOLID 2		// Fill with destarea.k (solid value), but only where src != srcarea.k (transparent value)


volatile uint8_t tick=0;

void plat_quit()
{
}

// Noddy profiling (rasterbars)
void plat_gatso(uint8_t t)
{
}

/*
 * TEXT FUNCTIONS
 */

// Clear text
void plat_clr()
{
//    neo_graphics_set_color(0);
//    neo_graphics_draw_rectangle(0,0,SCREEN_W, SCREEN_H);
}

// Draw len number of chars
void plat_textn(uint8_t cx, uint8_t cy, const char* txt, uint8_t len, uint8_t colour)
{
    char buf[40];
    buf[0] = len;
    for(uint8_t i = 0; i < len; ++i) {
        buf[i+1] = txt[i];
    }
    buf[len+2] = '\0';
    neo_graphics_set_color(colour);
    neo_graphics_draw_text_p(cx*8, cy*8, (const neo_pstring_t*)buf);
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


/*
 * SPRITES
 */
 
void sprout16(int16_t x, int16_t y, uint8_t img)
{
    x = x >> FX;
    y = y >> FX;
    const void* srcdat = export_spr16_bin + (img*SPR16_SIZE);

    struct BlitterArea src = {
        .address = (uint16_t)srcdat,
        .page = 0x00,
        .padding = 0,
        .stride = 16/2,
        .format = BLTFMT_PAIR,  // pairs
        .transparent = 0, // mask colour
        .solid = 1,
        .height = 16,
        .width = 16, // line size in units
    };
    neo_blit_image(BLTACT_MASK, &src,  x, y, BLTFMT_BYTE);
}

void sprout16_highlight(int16_t x, int16_t y, uint8_t img)
{
    x = x >> FX;
    y = y >> FX;
    const void* srcdat = export_spr16_bin + (img*SPR16_SIZE);

    struct BlitterArea src = {
        .address = (uint16_t)srcdat,
        .page = 0x00,
        .padding = 0,
        .stride = 16/2,
        .format = BLTFMT_PAIR,  // pairs
        .transparent = 0, // mask colour
        .solid = 1,
        .height = 16,
        .width = 16, // line size in units
    };
    neo_blit_image(BLTACT_SOLID, &src, x, y, BLTFMT_BYTE);
}

void sprout32(int16_t x, int16_t y, uint8_t img)
{
    x = x >> FX;
    y = y >> FX;
    const void* srcdat = export_spr32_bin + (img * SPR32_SIZE);

    struct BlitterArea src = {
        .address = (uint16_t)srcdat,
        .page = 0x00,
        .padding = 0,
        .stride = 32/2,
        .format = BLTFMT_PAIR,  // pairs
        .transparent = 0, // mask colour
        .solid = 1,
        .height = 32,
        .width = 32, // line size in units
    };
    neo_blit_image(BLTACT_MASK, &src, x, y, BLTFMT_BYTE);
}

void sprout32_highlight(int16_t x, int16_t y, uint8_t img)
{
    x = x >> FX;
    y = y >> FX;
    const void* srcdat = export_spr32_bin + (img * SPR32_SIZE);

    struct BlitterArea src = {
        .address = (uint16_t)srcdat,
        .page = 0x00,
        .padding = 0,
        .stride = 32/2,
        .format = BLTFMT_PAIR,  // pairs
        .transparent = 0, // mask colour
        .solid = 1,
        .height = 32,
        .width = 32, // line size in units
    };
    neo_blit_image(BLTACT_SOLID, &src, x, y, BLTFMT_BYTE);
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

// Returns direction + PAD_ bits.
uint8_t plat_raw_gamepad()
{
    uint8_t out = 0;
    uint8_t raw = neo_controller_read();
    // bits are: YXBADURL
    if (raw & 0x01) {
        out |= INP_LEFT;
    }
    if (raw & 0x02) {
        out |= INP_RIGHT;
    }
    if (raw & 0x04) {
        out |= INP_UP;
    }
    if (raw & 0x08) {
        out |= INP_DOWN;
    }

    if (raw & 0x10) {
        out |= INP_PAD_A;
    }

    if (raw & 0x20) {
        out |= INP_PAD_START;
    }
    return out;
}



// Returns direction + KEY_ bits.
uint8_t plat_raw_keys()
{
    return 0;
}

uint8_t plat_raw_cheatkeys()
{
    return 0;
}

#ifdef PLAT_HAS_MOUSE
int16_t plat_mouse_x;
int16_t plat_mouse_y;
uint8_t plat_mouse_buttons;   // left:0x01 right:0x02 middle:0x04
#endif // PLAT_HAS_MOUSE


#ifdef PLAT_HAS_TEXTENTRY
// start PLAT_HAS_TEXTENTRY (fns should be no-ops if not supported)
void plat_textentry_start()
{
}

void plat_textentry_stop()
{
}

// Returns printable ascii or DEL (0x7f) or LF (0x0A), or 0 for no input.
char plat_textentry_getchar()
{
    return '\0';
}

#endif // PLAT_HAS_TEXTENTRY


/*
 * HIGHSCORE PERSISTENCE
 */

#ifdef PLAT_HAS_SCORESAVE

bool plat_savescores(const void* begin, int nbytes)
{
    return false;
}

bool plat_loadscores(void* begin, int nbytes)
{
    return false;
}
#endif // PLAT_HAS_SCORESAVE



static void waitvbl() {
    uint8_t start = neo_graphics_frame_count() & 0xff;
    while (1) {
        uint8_t now = neo_graphics_frame_count() & 0xff;
        if(now != start) {
            break;
        }
    }
}

// 128 16x16
// 3 32x32
// 256 8x8 tiles

//(128*8*16) + (3*16*32) = 17920

static uint8_t gfxheader[256] = {
    //
    1,  // magic cookie
    0,  // ntiles 16x16
    SPR16_NUM,  // nsprites 16x16
    SPR32_NUM,  // nsprites 32x32
    // padding
    0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,

    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
};


const uint8_t testspr[8*16] = {
    // sprite 0
    0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xBC, 0xEF,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
    0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
    0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
    0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,

    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
    0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A,
    0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B,
    0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C,
    0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D,
    0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E,
    0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
};

static void gfx_init() {
    neo_graphics_set_defaults(0xff, 0x00, 1, 0, 0);
    neo_graphics_set_color(1);
    neo_graphics_set_draw_size(1);
    neo_graphics_set_solid_flag(1);

    // low nybble colours
    for(int i = 0; i < 16; ++i) {
        const uint8_t *c = export_palette_bin + (i<<2);
        neo_graphics_set_palette(i, c[0], c[1], c[2]);
    }
    // high nybble colours (sprites)
    for(int i=16; i<256; ++i) {
        const uint8_t *c = export_palette_bin + ((i>>4)<<2);
        neo_graphics_set_palette(i, c[0], c[1], c[2]);
    }

    // Upload graphics data to vram.
#if 0
    uint16_t dest = 0x0000;
    neo_blitter_copy(0x00, gfxheader, 0x90, (void*)dest, 256);
    dest += 256;
    neo_blitter_copy(0x00, export_spr16_bin, 0x90, (void*)dest, export_spr16_bin_len);
    //neo_blitter_copy(0x00, testspr, 0x90, (void*)dest, 8*16);
    dest += export_spr16_bin_len;
    neo_blitter_copy(0x00, export_spr32_bin, 0x90, (void*)dest, export_spr32_bin_len);
    dest += export_spr32_bin_len;
#endif
}


int main(int argc, char* argv[])
{
    gfx_init();

    game_init();
    while(1) {
        waitvbl();
        ++tick;
        neo_graphics_set_color(0);
        neo_graphics_draw_rectangle(0,0,SCREEN_W, SCREEN_H);
//        neo_graphics_set_palette(0,0,tick,0);
        //for (uint8_t i=0; i<16; ++i) {
        //    neo_graphics_set_color(i);
        //    neo_graphics_draw_rectangle(0,i*4,8,(i+1)*4);
        // }
        game_render();
        //char buf[16];
        //sprintf(buf, "%d %d\n", plrx[0]>>FX, plry[0]>>FX);
        //plat_text(0,0,buf,1);
        game_tick();
    }
}


