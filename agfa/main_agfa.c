#include <stdint.h>
#include <stdbool.h>
#include "../plat.h"
#include "../sfx.h"

#include "../gfx_vera.h"


// NOTEs:
// in vera.h, the layer1.hscroll and layer1.vscroll registers are 16 bits, but
// on an odd byte boundary. Fine on 6502, but likely to cause problems on 680x0...
// (or can 68020 handle odd-memory 16bit access? I forget.)


// Our exported graphics data.
extern const unsigned char export_chars_bin[];
//extern const unsigned char export_palette_bin[];      // gfx_vera handles palette
extern const unsigned char export_spr16_bin[];
extern const unsigned char export_spr32_bin[];
extern const unsigned char export_spr64x8_bin[];

static void waitvbl();
static void load_data_to_vram();

static bool quit = false;

int main(int argc, char* argv[])
{
    load_data_to_vram();
    gfx_vera_init();
    game_init();
    while(!quit) {
        waitvbl();
        gfx_vera_render_start();
        game_render();
        gfx_vera_render_finish();
        game_tick();
    }
}


// want to increase this once per frame.
volatile uint8_t tick;

void waitvbl()
{
}

void plat_quit()
{
    quit = true;
}

/*
 * SOUND
 *
 * vera psg sfx support.
 * sfx_verapsg.c supplies the bulk of the sound code.
 * It requires us to supply plat_psg():
 */

#define VRAM_PSG 0x1F9C0
void plat_psg(uint8_t chan, uint16_t freq, uint8_t vol, uint8_t waveform, uint8_t pulsewidth)
{
    uint32_t addr = VRAM_PSG + chan * 4;
    VERA.control = 0x00;
    VERA.address = ((addr)&0xffff);
    VERA.address_hi = (VERA_INC_1) | (((addr)>>16)&1);

    VERA.data0 = freq & 0xff;
    VERA.data0 = freq >> 8;
    VERA.data0 = (3 << 6) | vol;  // lrvvvvvv
    VERA.data0 = (waveform << 6) | pulsewidth;         // wwpppppp
}


/*
 * INPUT
 *
 * These will need filling in. See other platforms for examples!
 */

// Returns direction + FIRE_ bits.
uint8_t plat_raw_dualstick()
{
    return 0;
}

// Returns direction + PAD_ bits.
uint8_t plat_raw_gamepad()
{
    return 0;
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


static void load_data_to_vram()
{

    // Install the 16x16 sprite data
    {
        const uint8_t *src = (const uint8_t*)export_spr16_bin;
        uint32_t dest = VRAM_SPRITES16;
        VERA.control = 0x00;
        VERA.address = ((dest)&0xffff);
        VERA.address_hi = (VERA_INC_1) | (((dest)>>16)&1);
        for (int i = 0; i < SPR16_SIZE * SPR16_NUM; ++i) {
            VERA.data0 = *src++;
        }
    }
    // Install the 32x32 sprite data
    {
        const uint8_t *src = (const uint8_t*)export_spr32_bin;
        uint32_t dest = VRAM_SPRITES32;
        VERA.control = 0x00;
        VERA.address = ((dest)&0xffff);
        VERA.address_hi = (VERA_INC_1) | (((dest)>>16)&1);
        for (int i = 0; i < SPR32_SIZE * SPR32_NUM; ++i) {
            VERA.data0 = *src++;
        }
    }
    // Install the 64x8 sprite data
    {
        const uint8_t *src = (const uint8_t*)export_spr64x8_bin;
        uint32_t dest = VRAM_SPRITES64x8;
        VERA.control = 0x00;
        VERA.address = ((dest)&0xffff);
        VERA.address_hi = (VERA_INC_1) | (((dest)>>16)&1);
        for (int i = 0; i < SPR64x8_SIZE * SPR64x8_NUM; ++i) {
            VERA.data0 = *src++;
        }
    }
    // Install the charset data
    {
        const uint8_t *src = (const uint8_t*)export_chars_bin;
        uint32_t dest = VRAM_LAYER1_TILES;
        VERA.control = 0x00;
        VERA.address = ((dest)&0xffff);
        VERA.address_hi = (VERA_INC_1) | (((dest)>>16)&1);
        for (int i = 0; i < 8*256; ++i) {
            VERA.data0 = *src++;
        }
    }
}

