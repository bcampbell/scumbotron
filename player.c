#include "player.h"
#include "sys_cx16.h"
#include "gob.h"

uint8_t player_lives;
uint32_t player_score;

// player vars
int16_t plrx[MAX_PLAYERS];
int16_t plry[MAX_PLAYERS];
uint8_t plrtimer[MAX_PLAYERS];
uint8_t plrfacing[MAX_PLAYERS]; // 0xff = dead, else DIR_ bits

// shot vars
int16_t shotx[MAX_SHOTS];
int16_t shoty[MAX_SHOTS];
uint8_t shotdir[MAX_SHOTS];
uint8_t shottimer[MAX_SHOTS];

static uint8_t shot_alloc();

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


void player_create(uint8_t p, int16_t x, int16_t y) {
    plrx[p] = x;
    plry[p] = y;
    plrfacing[p] = 0;
    plrtimer[p] = 0;
}

void player_renderall()
{
    for (uint8_t p = 0; p < MAX_PLAYERS; ++p) {
        if (plrfacing[p] == 0xff) {
            // dead.
            continue;
        }
        sprout(plrx[p], plry[p], 0);
    }

    for (uint8_t s = 0; s < MAX_SHOTS; ++s) {
        if (!shotdir[s]) {
            // inactive.
            continue;
        }
        sprout(shotx[s], shoty[s], shot_spr[shotdir[s]]);
    }
}

void player_tickall()
{
    for (uint8_t p = 0; p < MAX_PLAYERS; ++p) {
        if (plrfacing[p] == 0xff) {
            // dead.
            continue;
        }
        player_tick(p);
    }
    for (uint8_t s = 0; s < MAX_SHOTS; ++s) {
        if (!shotdir[s]) {
            // inactive.
            continue;
        }
        shot_tick(s);
    }
}


static inline bool overlap(int16_t amin, int16_t amax, int16_t bmin, int16_t bmax)
{
    return (amin <= bmax) && (amax >= bmin);
}


// returns true if player killed.
bool player_collisions()
{
    for (uint8_t p = 0; p < MAX_PLAYERS; ++p) {
        int16_t px0 = plrx[p] + (4 << FX);
        int16_t py0 = plry[p] + (4 << FX);
        int16_t px1 = plrx[p] + (12 << FX);
        int16_t py1 = plry[p] + (12 << FX);
        for (uint8_t d = 0; d < MAX_GOBS; ++d) {
            if (gobkind[d]==GK_NONE) {
                continue;
            }
            if (overlap(px0, px1, gobx[d], gobx[d] + gob_size(d)) &&
                overlap(py0, py1, goby[d], goby[d] + gob_size(d))) {
                plrfacing[p] = 0xff;    // dead
                sys_addeffect(gobx[d], goby[d], EK_KABOOM);
                // boom
                return true;
            }
        }
    }
    return false;
}



void player_tick(uint8_t d) {
    ++plrtimer[d];

    uint8_t dir = 0;
    if ((inp_joystate & JOY_UP_MASK) ==0) {
        dir |= DIR_UP;
        plry[d] -= PLAYER_SPD;
    } else if ((inp_joystate & JOY_DOWN_MASK) ==0) {
        dir |= DIR_DOWN;
        plry[d] += PLAYER_SPD;
    }
    if ((inp_joystate & JOY_LEFT_MASK) ==0) {
        dir |= DIR_LEFT;
        plrx[d] -= PLAYER_SPD;
    } else if ((inp_joystate & JOY_RIGHT_MASK) ==0) {
        dir |= DIR_RIGHT;
        plrx[d] += PLAYER_SPD;
    }

    if ((inp_joystate & JOY_BTN_1_MASK) == 0) {
        if (!plrfacing[d]) {
            plrfacing[d] = dir;
        }

        if (plrtimer[d]>8) {
            // FIRE!
            plrtimer[d] = 0;
            uint8_t shot = shot_alloc();
            if (shot < MAX_SHOTS) {
                shotx[shot] = plrx[d];
                shoty[shot] = plry[d];
                shotdir[shot] = plrfacing[d];   // direction
                shottimer[shot] = 16;
            }
        }
    } else {
        if (dir) {
            plrfacing[d] = dir;
        }
    }

    // keep player on screen
    const int16_t xmax = (SCREEN_W - 16) << FX;
    if (plrx[d] < 0<<FX) {
        plrx[d] = 0;
    } else if (plrx[d] > xmax) {
        plrx[d] = xmax;
    }
    const int16_t ymax = (SCREEN_H - 16) << FX;
    if (plry[d] < 0<<FX) {
        plry[d] = 0;
    } else if (plry[d] > ymax) {
        plry[d] = ymax;
    }
}


// returns 0 if none free.
static uint8_t shot_alloc()
{
    for(uint8_t s = 0; s < MAX_SHOTS; ++s) {
        if(shotdir[s] == 0) {
            return s;
        }
    }
    return MAX_SHOTS;
}

// shot
// dat: dir
// timer: dies at 0
void shot_tick(uint8_t s)
{
    if (--shottimer[s] == 0) {
        shotdir[s] = 0; // inactive.
        return;
    }
    shotx[s] += shot_xvel(s);
    shoty[s] += shot_yvel(s);
}

void shot_collisions()
{
    for (uint8_t s = 0; s < MAX_SHOTS; ++s) {
        if (shotdir[s] == 0) {
            continue;   // inactive
        }
        // Take centre point of shot.
        int16_t sx = shotx[s] + (8<<FX);
        int16_t sy = shoty[s] + (8<<FX);

        for (uint8_t d = 0; d < MAX_GOBS; ++d) {
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
                            amoeba_shot(d, s);
                            break;
                        case GK_TANK:
                            tank_shot(d, s);
                            break;
                        default:
                            sys_addeffect(gobx[d], goby[d], EK_KABOOM);
                            gobkind[d] = GK_NONE;
                            break;
                    }

                    shotdir[s] = 0; // turn off shot.
                    break;  // next shot.
                }
            }
        }
    }
}
