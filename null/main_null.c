#include <stdint.h>
#include <stdbool.h>
#include "../plat.h"
#include "../sfx.h"


int main(int argc, char* argv[])
{
    return 0;
}

volatile uint8_t tick;

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
}

// Draw len number of chars
void plat_textn(uint8_t cx, uint8_t cy, const char* txt, uint8_t len, uint8_t colour)
{
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

