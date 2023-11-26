#include "plat.h"
#include "misc.h"

// common implementations of plat_drawbox() and plat_text()

static inline int8_t cclip(int8_t v, int8_t low, int8_t high)
{
    if (v < low) {
        return low;
    } else if (v > high) {
        return high;
    } else {
        return v;
    }
}

// draw box in char coords, with clipping
// (note cx,cy can be negative)
void plat_drawbox(int8_t cx, int8_t cy, uint8_t w, uint8_t h, uint8_t ch, uint8_t colour)
{
    int8_t x0,y0,x1,y1;
    x0 = cclip(cx, 0, SCREEN_W / 8);
    x1 = cclip(cx + w, 0, SCREEN_W / 8);
    y0 = cclip(cy, 0, SCREEN_H / 8);
    y1 = cclip(cy + h, 0, SCREEN_H / 8);

    // top
    if (y0 == cy) {
        plat_hline_noclip((uint8_t)x0, (uint8_t)x1, (uint8_t)y0, ch, colour);
    }
    if (h<=1) {
        return;
    }

    // bottom
    if (y1 - 1 == cy + h-1) {
        plat_hline_noclip((uint8_t)x0, (uint8_t)x1, (uint8_t)y1 - 1, ch, colour);
    }
    if (h<=2) {
        return;
    }

    // left (excluding top and bottom)
    if (x0 == cx) {
        plat_vline_noclip((uint8_t)x0, (uint8_t)y0, (uint8_t)y1, ch, colour);
    }
    if (w <= 1) {
        return;
    }

    // right (excluding top and bottom)
    if (x1 - 1 == cx + w - 1) {
        plat_vline_noclip((uint8_t)x1 - 1, (uint8_t)y0, (uint8_t)y1, ch, colour);
    }
}

void plat_text(uint8_t cx, uint8_t cy, const char* txt, uint8_t colour)
{
    uint8_t len = 0;
    while(txt[len] != '\0') {
        ++len;
    }
    plat_textn(cx, cy, txt, len, colour);
}




// Common sprite-rendering code

void plat_player_render(int16_t x, int16_t y, uint8_t facing, bool moving)
{
    uint8_t img = SPR16_PLR_R;
    if (facing & DIR_UP) { img = SPR16_PLR_U; }
    if (facing & DIR_DOWN) { img = SPR16_PLR_D; }
    if (facing & DIR_LEFT) { img = SPR16_PLR_L; }
    if (facing & DIR_RIGHT) { img = SPR16_PLR_R; }

    if (moving) {
        img += (tick >> 3) & 0x01;
    }
    sprout16(x, y, img);
}

void plat_pickup_render(int16_t x, int16_t y, uint8_t kind)
{
    sprout16(x, y, SPR16_EXTRALIFE + kind);
}

void plat_shot_render(int16_t x, int16_t y, uint8_t direction, uint8_t power)
{
    uint8_t base[3] = {SPR16_SHOT, SPR16_SHOT_MED, SPR16_SHOT_HEAVY};
    uint8_t dir8 = angle24toangle8[direction];
    sprout16(x, y, base[power] + (dir8 & 0x03));
}

void plat_block_render(int16_t x, int16_t y)
{
    sprout16(x, y, SPR16_BLOCK);
}

void plat_grunt_render(int16_t x, int16_t y)
{
    sprout16(x, y,  SPR16_GRUNT + ((tick >> 5) & 0x01));
}

void plat_baiter_render(int16_t x, int16_t y)
{
    sprout16(x, y,  SPR16_BAITER + ((tick >> 2) & 0x03));
}

void plat_tank_render(int16_t x, int16_t y, bool highlight)
{
    if (highlight) {
        sprout16_highlight(x, y,  SPR16_TANK + ((tick>>5) & 0x01));
    } else {
        sprout16(x, y,  SPR16_TANK + ((tick >> 5) & 0x01));
    }
}

void plat_amoeba_big_render(int16_t x, int16_t y)
{
    sprout32(x, y,  SPR32_AMOEBA_BIG + ((tick >> 3) & 0x01));
}

void plat_amoeba_med_render(int16_t x, int16_t y)
{
    sprout16(x, y, SPR16_AMOEBA_MED + ((tick >> 3) & 0x03));
}

void plat_amoeba_small_render(int16_t x, int16_t y)
{
    sprout16(x, y,  SPR16_AMOEBA_SMALL + ((tick >> 3) & 0x03));
}

void plat_fragger_render(int16_t x, int16_t y)
{
    sprout16(x, y,  SPR16_FRAGGER);
}

static const int8_t frag_frames[4] = {SPR16_FRAG_NW, SPR16_FRAG_NE,
    SPR16_FRAG_SW, SPR16_FRAG_SE};

void plat_frag_render(int16_t x, int16_t y, uint8_t dir)
{
    sprout16(x, y,  frag_frames[dir]);
}


// seq -127 128 | shuf | head -n 16
const int8_t jitter[16] = {46, -12, -125, 73, -28, -121, 51, 61, -24, 98, -32, -108, 115, 100, -46, -62};
void plat_vulgon_render(int16_t x, int16_t y, bool highlight, uint8_t anger)
{
    uint8_t img;
    switch (anger) {
        case 0:
            img = SPR16_VULGON + ((tick>>4) & 0x01);
            break;
        case 1:
            img = SPR16_VULGON_ANGRY + ((tick>>3) & 0x01);
            x += jitter[tick&0xf]>>1;
            y += jitter[(tick+3)&0xf]>>1;
            break;
        default:
            img = SPR16_VULGON_ANGRY + ((tick>>3) & 0x01);
            x += jitter[tick&0xf];
            y += jitter[(tick+3)&0xf];
            break;
    }
    if (highlight) {
        sprout16_highlight(x, y, img);
    } else {
        sprout16(x, y, img);
    }
}

void plat_poomerang_render(int16_t x, int16_t y)
{
    sprout16(x, y, SPR16_POOMERANG + ((tick >> 3) & 0x03));
}

void plat_happyslapper_render(int16_t x, int16_t y, bool sleeping)
{
    uint8_t img;
    if (sleeping) {
        img = SPR16_HAPPYSLAPPER_SLEEP;
    } else {
        img = SPR16_HAPPYSLAPPER + ((tick >> 3) & 0x01);
    }
    sprout16(x, y, img);
}

void plat_marine_render(int16_t x, int16_t y)
{
    sprout16(x, y, SPR16_MARINE + ((tick >> 3) & 0x01));
}

void plat_boss_render(int16_t x, int16_t y, bool highlight)
{
    if (highlight) {
        sprout32_highlight(x, y, SPR32_BOSS_HEAD);
    } else {
        sprout32(x, y, SPR32_BOSS_HEAD);
    }
}

void plat_bossseg_render(int16_t x, int16_t y, uint8_t phase, bool atrest, bool highlight)
{
    if (atrest) {
        sprout16(x, y, SPR16_BOSS_SEG);
    } else {
        sprout16(x, y, SPR16_BOSS_SEG + ((phase + (tick >> 3)) & 3));
    }
}


void plat_bub_render(int16_t x, int16_t y, uint8_t bubidx)
{
    sprout64x8(x, y, SPR64x8_BUB + bubidx);
}

void plat_cursor_render(int16_t x, int16_t y)
{
    if ((tick>>4) & 0x01) {
        sprout16(x, y, SPR16_CURSOR);

    } else {
        sprout16(x, y, SPR16_CURSOR+1);
    }
}

void plat_brain_render(int16_t x, int16_t y, bool highlight)
{
    if (highlight) {
        sprout16_highlight(x, y, SPR16_BRAIN + ((tick>>4) & 0x01));
    } else {
        sprout16(x, y, SPR16_BRAIN + ((tick>>4) & 0x01));
    }
}


void plat_zombie_render(int16_t x, int16_t y)
{

    x += jitter[tick&0xf]>>3;
    y += jitter[(tick+3)&0xf]>>3;
    sprout16(x, y, SPR16_ZOMBIE + ((tick>>4) & 0x01));
}

void plat_rifashark_render(int16_t x, int16_t y, uint8_t dir8)
{
    uint8_t f = (tick & 0x10);
    sprout16(x, y, SPR16_RIFASHARK + dir8 + f);
}

void plat_rifaspawner_render(int16_t x, int16_t y)
{
    uint8_t f = ((tick>>2) & 0x07);
    if (f>=4) {
        f=7-f;
    }
    sprout16(x, y, SPR16_RIFASPAWNER + f);
}

void plat_gobber_render(int16_t x, int16_t y, uint8_t dir8, bool highlight)
{
    uint8_t f = SPR16_GOBBER + dir8;
    if (highlight) {
        sprout16_highlight(x, y, f);
    } else {
        sprout16(x, y, f);
    }
}

void plat_missile_render(int16_t x, int16_t y, uint8_t dir8)
{
    uint8_t f = SPR16_MISSILE + dir8;
    sprout16(x, y, f);
}

