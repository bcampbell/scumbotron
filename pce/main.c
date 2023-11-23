#define PCE_CONFIG_IMPLEMENTATION
#include <pce.h>
// Try and map the whole game ROM in at once.
// TODO: don't think this works as expected...
// first rom bank always appears at top?
PCE_ROM_FIXED_BANK_SIZE(6);

#include "../plat.h"
#include "../misc.h"

volatile uint8_t tick = 0;

__attribute__((interrupt)) void irq_vdc(void) {
  // This also acknowledges the interrupt.
  uint8_t status = *IO_VDC_STATUS;
  if (status & VDC_FLAG_VBLANK) {
    ++tick;
  }
}

static void init_bg();
static void render_start();
static void render_finish();

extern const unsigned char export_palette_bin[];
extern const unsigned int export_palette_bin_len;
extern const unsigned char export_spr16_bin[];
extern const unsigned int export_spr16_bin_len;
extern const unsigned char export_spr32_bin[];
extern const unsigned int export_spr32_bin_len;
//extern const unsigned char export_spr64x8_bin[];
//extern const unsigned int export_spr64x8_bin_len;
extern const unsigned char export_chars_bin[];
extern const unsigned int export_chars_bin_len;


void waitvbl()
{
    uint8_t tmp = tick;
    while (tmp == tick) {}
}

int main(void) {
    // Configure the VDC screen resolution.
    pce_vdc_set_resolution(SCREEN_W, SCREEN_H, 0);  //256x240

    init_bg();

    // Set up vblank interrupt
    pce_vdc_irq_vblank_enable();
    pce_irq_enable(IRQ_VDC);
    pce_cpu_irq_enable();

    game_init();
    while(1) {
        waitvbl();
        render_start();
        game_render();
   //     if (mouse_watchdog > 0) {
    //        sprout16(plat_mouse_x, plat_mouse_y, 0);
    //    }

        //debug_gamepad();
        //debug_getin();
        render_finish();
        //cx16_k_joystick_scan();
        //update_inp_mouse();
        game_tick();

    }
}

#define NUM_SPR16 128
#define NUM_SPR32 3

#define BYTESIZE_SPR16 (16*8)   // 16x16 4bpp
#define BYTESIZE_SPR32 (32*16)   // 32x32 4bpp
#define BYTESIZE_SPR64x8 (64*4)   // 64x8 4bpp

// bytes per tile
#define TILE_BYTESIZE (8*4)   // 8x8 4bpp


#define VRAM_TILES (0xE000/2)   // top 8KB of VRAM
#define TILE_BASEIDX (VRAM_TILES/16)

static void init_bg()
{
    pce_vdc_bg_enable();
    pce_vdc_bg_set_size(VDC_BG_SIZE_32_32);

    // bg palette in first block
    uint16_t idx = 0x200;
    const uint8_t* src = export_palette_bin;
    for(uint8_t i=0; i<16; ++i) {
        for(uint8_t j=0; j<16; ++j) {
            *IO_VCE_COLOR_INDEX = idx++;
            *IO_VCE_COLOR_DATA = VCE_COLOR(src[0]>>2, src[1]>>2, src[2]>>2);
        }
        src += 4;
    }

    // Load chars into vram as bg tiles
    pce_vdc_copy_to_vram(VRAM_TILES, (const void *)export_chars_bin, (uint16_t)export_chars_bin_len/2);
}

static void render_start()
{
}

static void render_finish()
{
}

void plat_clr()
{
}

void plat_textn(uint8_t cx, uint8_t cy, const char* txt, uint8_t len, uint8_t colour)
{
    uint16_t buf[64];
    uint16_t vdest = (cy*2*32) + (cx*2);
    for (uint8_t i=0; i<len; ++i) {
        buf[i] = (colour<<12) | (TILE_BASEIDX + *txt);
        ++txt;
    }
    pce_vdc_set_copy_word();
    pce_vdc_copy_to_vram(vdest/2, (const void *)buf, len*2);
}

void plat_text(uint8_t cx, uint8_t cy, const char* txt, uint8_t colour)
{
    uint8_t len = 0;
    while(txt[len] != '\0') {
        ++len;
    }
    plat_textn(cx, cy, txt, len, colour);
}

void plat_hud(uint8_t level, uint8_t lives, uint32_t score)
{
}


void plat_mono4x2(uint8_t cx, int8_t cy, const uint8_t* src, uint8_t cw, uint8_t ch, uint8_t basecol)
{
}

void plat_drawbox(int8_t x, int8_t y, uint8_t w, uint8_t h, uint8_t ch, uint8_t colour)
{
}

void plat_gatso(uint8_t t)
{
}

// sfx

void plat_psg(uint8_t chan, uint16_t freq, uint8_t vol, uint8_t waveform, uint8_t pulsewidth)
{
}

static void sprout16(int16_t x, int16_t y, uint8_t img) {}
static void sprout16_highlight(int16_t x, int16_t y, uint8_t img) {}
static void sprout32(int16_t x, int16_t y, uint8_t img) {}
static void sprout32_highlight(int16_t x, int16_t y, uint8_t img) {}
static void sprout64x8(int16_t x, int16_t y, uint8_t img) {}

#include "../spr_common_inc.h"

void plat_hzapper_render(int16_t x, int16_t y, uint8_t state)
{
}

void plat_vzapper_render(int16_t x, int16_t y, uint8_t state)
{
}

// input

// start PLAT_HAS_TEXTENTRY
void plat_textentry_start() {}
void plat_textentry_stop() {}
extern char plat_textentry_getchar() {return 0;}
// end PLAT_HAS_TEXTENTRY
 
uint8_t plat_raw_dualstick()
{
    return 0;
}

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



