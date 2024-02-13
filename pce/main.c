#define PCE_CONFIG_IMPLEMENTATION
#include <pce.h>
// Try and map the whole game ROM in at once.
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

// sprites.c
void sprites_init();
void sprites_start();
void sprites_finish();


extern const unsigned char export_palette_bin[];
extern const unsigned int export_palette_bin_len;
extern const unsigned char export_chars_bin[];
extern const unsigned int export_chars_bin_len;


void waitvbl()
{
    uint8_t tmp = tick;
    while (tmp == tick) {}
}


void debug_gamepad()
{
    char buf[2] = {'H','I'};
    //uint8_t b = 0x69;   //plat_raw_dualstick();
    buf[0] = 'H';   //hexdigits[b >> 4];
    buf[1] = '0';   //hexdigits[b & 0x0f];
    plat_textn(0,0,buf,2,tick&15);
}

int main(void) {
    // Configure the VDC screen resolution.
    pce_vdc_set_resolution(SCREEN_W, SCREEN_H, 0);  //256x240

    init_bg();
    //sprites_init();

    // Set up vblank interrupt
    pce_vdc_irq_vblank_enable();
    pce_irq_enable(IRQ_VDC);
    pce_cpu_irq_enable();

    game_init();
    while(1) {
        waitvbl();
        render_start();
//        *IO_VCE_COLOR_INDEX = 0x200;
//        *IO_VCE_COLOR_DATA = (rnd()<<4) | rnd();
        game_render();
   //     if (mouse_watchdog > 0) {
    //        sprout16(plat_mouse_x, plat_mouse_y, 0);
    //    }

        debug_gamepad();
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


#define VRAM_CG 0x4000
#define TILE_BASEIDX (VRAM_CG/32)

static void init_bg()
{
    pce_vdc_bg_enable();
    pce_vdc_bg_set_size(VDC_BG_SIZE_32_32);

    // bg palette in first block
    uint16_t idx = 0x0000;
    const uint8_t* src = export_palette_bin;
    for(uint8_t i=0; i<16; ++i) {
        for(uint8_t j=0; j<16; ++j) {
            *IO_VCE_COLOR_INDEX = idx++;
            *IO_VCE_COLOR_DATA = VCE_COLOR(src[0]>>5, src[1]>>5, src[2]>>5);
        }
        src += 4;
    }

    // Load chars into vram as bg tiles
    pce_vdc_set_copy_word();
    pce_vdc_copy_to_vram(VRAM_CG/2, (const void *)export_chars_bin, (uint16_t)export_chars_bin_len/2);
}

static void render_start()
{
    sprites_start();
}

static void render_finish()
{
    sprites_finish();
}

void plat_clr()
{
    const uint8_t colour = 5;
    uint16_t buf[32];
        for (uint8_t cx=0; cx<32; ++cx) {
            buf[cx] = (colour<<12) | (TILE_BASEIDX + cx);
        }
        /*
    for(uint8_t cy=0; cy<32; ++cy) {
        uint16_t vdest = (cy*2*32);
        pce_vdc_set_copy_word();
        pce_vdc_copy_to_vram(vdest/2, (const void *)buf, 32*2);
    }
    */
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

void plat_hud(uint8_t level, uint8_t lives, uint32_t score)
{
}


void plat_mono4x2(uint8_t cx, int8_t cy, const uint8_t* src, uint8_t cw, uint8_t ch, uint8_t basecol)
{
}

// Draw horizontal line of chars, range [cx_begin, cx_end).
void plat_hline_noclip(uint8_t cx_begin, uint8_t cx_end, uint8_t cy, uint8_t ch, uint8_t colour)
{
}

// Draw vertical line of chars, range [cy_begin, cy_end).
void plat_vline_noclip(uint8_t cx, uint8_t cy_begin, uint8_t cy_end, uint8_t ch, uint8_t colour)
{
}


void plat_gatso(uint8_t t)
{
}

// sfx

void plat_psg(uint8_t chan, uint16_t freq, uint8_t vol, uint8_t waveform, uint8_t pulsewidth)
{
}

// render fns

void plat_hzapper_render(int16_t x, int16_t y, uint8_t state)
{
    // TODO!
}

void plat_vzapper_render(int16_t x, int16_t y, uint8_t state)
{
    // TODO!
}

// Unsupported
void plat_quit()
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
    uint8_t out = 0;
    uint8_t b = pce_joypad_read();

    if (b & KEY_LEFT) { out |= INP_LEFT; }
    if (b & KEY_RIGHT) { out |= INP_RIGHT; }
    if (b & KEY_UP) { out |= INP_UP; }
    if (b & KEY_DOWN) { out |= INP_DOWN; }
   
    out |= (out<<4);    /// fudge fire dirs for now
    return 0;
}

uint8_t plat_raw_gamepad() {
    uint8_t out = 0;
    uint8_t b = pce_joypad_read();

    if (b & KEY_LEFT) { out |= INP_LEFT; }
    if (b & KEY_RIGHT) { out |= INP_RIGHT; }
    if (b & KEY_UP) { out |= INP_UP; }
    if (b & KEY_DOWN) { out |= INP_DOWN; }

    if (b & KEY_1) { out |= INP_PAD_A; }
    if (b & KEY_2) { out |= INP_PAD_B; }
    if (b & KEY_RUN) { out |= INP_PAD_START; }
    //if (b & KEY_SELECT) { out |= INP_PAD_???; }
    return out;
}

uint8_t plat_raw_keys()
{
    return 0;   // no keyboards here!
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



