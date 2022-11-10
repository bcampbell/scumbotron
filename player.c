#include "player.h"
#include "sys_cx16.h"
#include "gob.h"

uint8_t player_lives;
uint32_t player_score;

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

// hackhackhack (from cx16.h)
#define JOY_UP_MASK 0x08
#define JOY_DOWN_MASK 0x04
#define JOY_LEFT_MASK 0x02
#define JOY_RIGHT_MASK 0x01
#define JOY_BTN_1_MASK 0x80


// player
#define PLAYER_SPD FX_ONE


void player_create(uint8_t d, int16_t x, int16_t y) {
    gobkind[d] = GK_PLAYER;
    gobflags[d] = 0;
    gobx[d] = x;
    goby[d] = y;
    gobdat[d] = 0;
    gobtimer[d] = 0;
}

void player_tick(uint8_t d) {
    uint8_t dir = 0;
    if ((inp_joystate & JOY_UP_MASK) ==0) {
        dir |= DIR_UP;
        goby[d] -= PLAYER_SPD;
    } else if ((inp_joystate & JOY_DOWN_MASK) ==0) {
        dir |= DIR_DOWN;
        goby[d] += PLAYER_SPD;
    }
    if ((inp_joystate & JOY_LEFT_MASK) ==0) {
        dir |= DIR_LEFT;
        gobx[d] -= PLAYER_SPD;
    } else if ((inp_joystate & JOY_RIGHT_MASK) ==0) {
        dir |= DIR_RIGHT;
        gobx[d] += PLAYER_SPD;
    }

    if ((inp_joystate & JOY_BTN_1_MASK) == 0) {
        if (!gobdat[d]) {
            gobdat[d] = dir;
        }

        uint8_t shot = shot_alloc();
        if (shot) {
            gobkind[shot] = GK_SHOT;
            gobx[shot] = gobx[d];
            goby[shot] = goby[d];
            gobdat[shot] = gobdat[d];   // direction
            gobtimer[shot] = 16;
        }
    } else {
        if (dir) {
            gobdat[d] = dir;
        }
    }

    // keep player on screen
    const int16_t xmax = (SCREEN_W - 16) << FX;
    if (gobx[d] < 0<<FX) {
        gobx[d] = 0;
    } else if (gobx[d] > xmax) {
        gobx[d] = xmax;
    }
    const int16_t ymax = (SCREEN_H - 16) << FX;
    if (goby[d] < 0<<FX) {
        goby[d] = 0;
    } else if (goby[d] > ymax) {
        goby[d] = ymax;
    }
}

// shot
// dat: dir
// timer: dies at 0
#define SHOT_SPD (8<<FX)
void shot_tick(uint8_t s)
{
    if (--gobtimer[s] == 0) {
        gobkind[s] = GK_NONE;
        return;
    }
    uint8_t dir = gobdat[s];
    if (dir & DIR_UP) {
        goby[s] -= SHOT_SPD;
    } else if (dir & DIR_DOWN) {
        goby[s] += SHOT_SPD;
    }
    if (dir & DIR_LEFT) {
        gobx[s] -= SHOT_SPD;
    } else if (dir & DIR_RIGHT) {
        gobx[s] += SHOT_SPD;
    }
}


void shot_collisions()
{
    for (uint8_t s = FIRST_SHOT; s < (FIRST_SHOT + MAX_SHOTS); ++s) {
        if (gobkind[s] == GK_NONE) {
            continue;
        }
        // Take centre point of shot.
        int16_t sx = gobx[s] + (8<<FX);
        int16_t sy = goby[s] + (8<<FX);

        for (uint8_t d = FIRST_DUDE; d < (FIRST_DUDE + MAX_DUDES); d=d+1) {
            if (gobkind[d]==GK_NONE) {
                continue;
            }
            int16_t dy0 = goby[d];
            int16_t dy1 = goby[d] + gob_size(d);
            if (sy >= dy0 && sy < dy1) {
                int16_t dx0 = gobx[d];
                int16_t dx1 = gobx[d] + gob_size(d);
                if (sx >= dx0 && sx < dx1) {
                    // boom.
                    switch (gobkind[d]) {
                        case GK_AMOEBA_BIG:
                        case GK_AMOEBA_MED:
                        case GK_AMOEBA_SMALL:
                            amoeba_shot(d);
                            break;
                        default:
                            gobkind[d] = GK_NONE;
                            break;
                    }

                    gobkind[s] = GK_NONE;
                    break;  // next shot.
                }
            }
        }
    }
}


static inline bool overlap(uint16_t amin, uint16_t amax, uint16_t bmin, uint16_t bmax)
{
    return (amin <= bmax) && (amax >= bmin);
}

bool player_collisions()
{
    for (uint8_t p = FIRST_PLAYER; p < (FIRST_PLAYER + MAX_PLAYERS); ++p) {
        if (gobkind[p] != GK_PLAYER) {
            continue;
        }
        for (uint8_t d = FIRST_DUDE; d < (FIRST_DUDE + MAX_DUDES); d=d+1) {
            if (gobkind[d]==GK_NONE) {
                continue;
            }
            if (overlap(gobx[p] + (4 << FX), gobx[p] + (12 << FX), gobx[d], gobx[d] + (16 << FX)) &&
                overlap(goby[p] + (4 << FX), goby[p] + (12 << FX), goby[d], goby[d] + (16 << FX))) {
                gobkind[p] = GK_NONE;
                // boom
                return true;
            }
        }
    }
    return false;
}


