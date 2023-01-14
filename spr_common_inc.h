// Common sprite-rendering code used by most (all?) backends.
// used as #include file (for now)

// assumes existence of
// sprout16()
// sprout32()
// sprout16_highlight()

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

#define SPR32_AMOEBA_BIG 0

const uint8_t shot_spr[16] = {
    0,              // 0000
    SPR16_SHOT+2,   // 0001 DIR_RIGHT
    SPR16_SHOT+2,   // 0010 DIR_LEFT
    0,              // 0011
    SPR16_SHOT+0,   // 0100 DIR_DOWN
    SPR16_SHOT+3,   // 0101 down+right           
    SPR16_SHOT+1,   // 0110 down+left           
    0,              // 0111

    SPR16_SHOT+0,   // 1000 up
    SPR16_SHOT+1,   // 1001 up+right
    SPR16_SHOT+3,   // 1010 up+left
    0,              // 1011
    0,              // 1100 up+down
    0,              // 1101 up+down+right           
    0,              // 1110 up+down+left           
    0,              // 1111
};


void sys_player_render(int16_t x, int16_t y)
{
    sprout16(x, y, 0);
}

void sys_powerup_render(int16_t x, int16_t y, uint8_t kind)
{
    sprout16(x, y, SPR16_EXTRALIFE);
}

void sys_shot_render(int16_t x, int16_t y, uint8_t direction)
{
    sprout16(x, y, shot_spr[direction]);
}

void sys_block_render(int16_t x, int16_t y)
{
    sprout16(x, y, 2);
}

void sys_grunt_render(int16_t x, int16_t y)
{
    sprout16(x, y,  SPR16_GRUNT + ((tick >> 5) & 0x01));
}

void sys_baiter_render(int16_t x, int16_t y)
{
    sprout16(x, y,  SPR16_BAITER + ((tick >> 2) & 0x03));
}

void sys_tank_render(int16_t x, int16_t y, bool highlight)
{
    if (highlight) {
        sprout16_highlight(x, y,  SPR16_TANK + ((tick>>5) & 0x01));
    } else {
        sprout16(x, y,  SPR16_TANK + ((tick >> 5) & 0x01));
    }
}

void sys_amoeba_big_render(int16_t x, int16_t y)
{
    sprout32(x, y,  SPR32_AMOEBA_BIG + ((tick >> 3) & 0x01));
}

void sys_amoeba_med_render(int16_t x, int16_t y)
{
    sprout16(x, y, SPR16_AMOEBA_MED + ((tick >> 3) & 0x03));
}

void sys_amoeba_small_render(int16_t x, int16_t y)
{
    sprout16(x, y,  SPR16_AMOEBA_SMALL + ((tick >> 3) & 0x03));
}

void sys_fragger_render(int16_t x, int16_t y)
{
    sprout16(x, y,  SPR16_FRAGGER);
}

static const int8_t frag_frames[4] = {SPR16_FRAG_NW, SPR16_FRAG_NE,
    SPR16_FRAG_SW, SPR16_FRAG_SE};

void sys_frag_render(int16_t x, int16_t y, uint8_t dir)
{
    sprout16(x, y,  frag_frames[dir]);
}

void sys_vulgon_render(int16_t x, int16_t y, bool highlight)
{
    if (highlight) {
        sprout16_highlight(x, y,  SPR16_VULGON + ((tick>>5) & 0x01));
    } else {
        sprout16(x, y,  SPR16_VULGON + ((tick >> 5) & 0x01));
    }
}

