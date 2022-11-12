#include "player.h"
#include "sys_cx16.h"
#include "gob.h"

uint8_t player_lives;
uint32_t player_score;

// player vars
int16_t plrx[MAX_PLAYERS];
int16_t plry[MAX_PLAYERS];
uint8_t plrtimer[MAX_PLAYERS];
uint8_t plrfacing[MAX_PLAYERS];

#if 0
// shot vars
int16_t shotx[MAX_SHOTS];
int16_t shoty[MAX_SHOTS];
uint8_t shotdir[MAX_SHOTS];
uint8_t shottimer[MAX_SHOTS];
#endif

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
    for (uint8_t p=0; p<MAX_PLAYERS; ++p) {
        if (plrfacing[p] == 0xff) {
            // dead.
            continue;
        }
        sprout(plrx[p], plry[p], 0);
    }
}

void player_tickall()
{
    for (uint8_t p=0; p<MAX_PLAYERS; ++p) {
        if (plrfacing[p] == 0xff) {
            // dead.
            continue;
        }
        player_tick(p);
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
        for (uint8_t d = FIRST_DUDE; d < (FIRST_DUDE + MAX_DUDES); ++d) {
            if (gobkind[d]==GK_NONE) {
                continue;
            }
            if (overlap(px0, px1, gobx[d], gobx[d] + gob_size(d)) &&
                overlap(py0, py1, goby[d], goby[d] + gob_size(d))) {
                plrfacing[p] = 0xff;    // dead
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
            plrtimer[d] = 0;
            uint8_t shot = shot_alloc();
            if (shot) {
                gobkind[shot] = GK_SHOT;
                gobx[shot] = plrx[d];
                goby[shot] = plry[d];
                gobdat[shot] = plrfacing[d];   // direction
                gobtimer[shot] = 16;
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
    for(uint8_t d = MAX_PLAYERS; d < MAX_PLAYERS + MAX_SHOTS; ++d) {
        if(gobkind[d] == GK_NONE) {
            return d;
        }
    }
    return 0;
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


