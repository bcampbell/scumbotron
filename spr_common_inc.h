// Common sprite-rendering code used by most (all?) backends.
// used as #include file (for now)

// assumes existence of
// sprout16()
// sprout32()
// sprout16_highlight()
// sprout64x8()

// sprite image defs
#define SPR16_EXTRALIFE 8
#define SPR16_BAITER 16
#define SPR16_AMOEBA_MED 20
#define SPR16_AMOEBA_SMALL 24
#define SPR16_TANK 28
#define SPR16_GRUNT 30
#define SPR16_SHOT 4
#define SPR16_HZAPPER 12
#define SPR16_HZAPPER_ON 13
#define SPR16_VZAPPER 14
#define SPR16_VZAPPER_ON 15
#define SPR16_FRAGGER 32
#define SPR16_FRAG_NW 33
#define SPR16_FRAG_NE 34
#define SPR16_FRAG_SW 35
#define SPR16_FRAG_SE 36
#define SPR16_VULGON 37
#define SPR16_VULGON_ANGRY 39
#define SPR16_POOMERANG 41
#define SPR16_HAPPYSLAPPER 45
#define SPR16_HAPPYSLAPPER_SLEEP 47
#define SPR16_PLR_U 48
#define SPR16_PLR_D 50
#define SPR16_PLR_L 52
#define SPR16_PLR_R 54
#define SPR16_MARINE 56
#define SPR16_WIBBLER 58

#define SPR32_AMOEBA_BIG 0

#define SPR64x8_BUB 0   // Start of speech bubble sprites.


// SHOT+0 vert, +1 horiz, +2 nw, +3 ne
const uint8_t shot_spr[24] = {
    SPR16_SHOT+0,   // 0 up
    SPR16_SHOT+0,
    SPR16_SHOT+1,

    SPR16_SHOT+1,   // 3 up/right
    SPR16_SHOT+1,
    SPR16_SHOT+2,

    SPR16_SHOT+2,   // 6 right
    SPR16_SHOT+2,
    SPR16_SHOT+3,

    SPR16_SHOT+3,   // 9 down/right
    SPR16_SHOT+3,
    SPR16_SHOT+0,

    SPR16_SHOT+0,   // 12 down
    SPR16_SHOT+0,
    SPR16_SHOT+1,

    SPR16_SHOT+1,   // 15 down/left
    SPR16_SHOT+1,
    SPR16_SHOT+2,

    SPR16_SHOT+2,  // 18 left
    SPR16_SHOT+2,
    SPR16_SHOT+3,

    SPR16_SHOT+3,  // 21 up/left
    SPR16_SHOT+3,
    SPR16_SHOT+0,
};


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

void plat_powerup_render(int16_t x, int16_t y, uint8_t kind)
{
    sprout16(x, y, SPR16_EXTRALIFE);
}

void plat_shot_render(int16_t x, int16_t y, uint8_t direction)
{
    sprout16(x, y, shot_spr[direction]);
}

void plat_block_render(int16_t x, int16_t y)
{
    sprout16(x, y, 2);
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

void plat_wibbler_render(int16_t x, int16_t y, bool head)
{
    sprout16(x, y, head ? SPR16_WIBBLER+4 : (SPR16_WIBBLER + ((tick>>2) & 0x03)));
}

void plat_bub_render(int16_t x, int16_t y, uint8_t bubidx)
{
    sprout64x8(x, y, SPR64x8_BUB + bubidx);
}

