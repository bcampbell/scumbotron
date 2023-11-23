#include <pce.h>
#include "../plat.h"

// sprites

extern const unsigned char export_spr16_bin[];
extern const unsigned int export_spr16_bin_len;
extern const unsigned char export_spr32_bin[];
extern const unsigned int export_spr32_bin_len;
//extern const unsigned char export_spr64x8_bin[];
//extern const unsigned int export_spr64x8_bin_len;

extern const unsigned char export_palette_bin[];
extern const unsigned int export_palette_bin_len;

// sprite attribute table in vram (for DMAing into internal VDC SATB)
#define VRAM_SATB (0x8000/2)

#define VRAM_SPR16 (0x4000/2)

#define SPR16_BASE_IDX ((VRAM_SPR16*2)/128)

void sprites_init()
{
    //pce_vdc_sprite_enable();

    // sprite palette in first block
    uint16_t idx = 0x200;
    for(uint8_t i=0; i<16; ++i) {
        const uint8_t* src = export_palette_bin;
        for(uint8_t j=0; j<16; ++j) {
            *IO_VCE_COLOR_INDEX = idx++;
            *IO_VCE_COLOR_DATA = VCE_COLOR(src[0]>>2, src[1]>>2, src[2]>>2);
            src += 4;
        }
    }

    // Load chars into vram as bg tiles
    pce_vdc_copy_to_vram(VRAM_SPR16, (const void *)export_spr16_bin, (uint16_t)export_spr16_bin_len/2);
}



static uint8_t nsprites;

void sprites_start()
{
    nsprites = 0;
}

void sprites_finish()
{

    //pce_vdc_dma_start(VDC_DMA_REPEAT_SATB, VRAM_SATB, 0, (nsprites*8)/2);
}


void sprout16(int16_t x, int16_t y, uint8_t img)
{
    // Word 0 : ------aaaaaaaaaa
    // Word 1 : ------bbbbbbbbbb
    // Word 2 : -----ccccccccccd
    // Word 3 : e-ffg--hi---jjjj
    vdc_sprite_t s;
    s.x = (x>>FX) - 32;
    s.y = (y>>FX) - 64;
    s.pattern = SPR16_BASE_IDX + img;
    s.attr = VDC_SPRITE_COLOR(0) | VDC_SPRITE_FG |VDC_SPRITE_WIDTH_16 | VDC_SPRITE_HEIGHT_16;

    pce_vdc_set_copy_word();
    uint16_t dest = VRAM_SATB + ((nsprites*8)/2);
    pce_vdc_copy_to_vram(dest, (const void *)&s, (uint16_t)sizeof(s));
    ++nsprites;
}

void sprout16_highlight(int16_t x, int16_t y, uint8_t img)
{
    sprout16(x, y, img);
}

void sprout32(int16_t x, int16_t y, uint8_t img)
{
    sprout16(x, y, img);
}

void sprout32_highlight(int16_t x, int16_t y, uint8_t img)
{
    sprout32(x, y, img);
}

void sprout64x8(int16_t x, int16_t y, uint8_t img)
{
}

