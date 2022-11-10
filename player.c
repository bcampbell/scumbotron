#include "player.h"
#include "sys_cx16.h"
#include "gob.h"

uint8_t player_lives;
uint32_t player_score;

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


void player_create(uint8_t d, int16_t x, int16_t y) {
    gobkind[d] = GK_PLAYER;
    gobflags[d] = 0;
    gobx[d] = x;
    goby[d] = y;
    gobdat[d] = 0;
    gobtimer[d] = 0;
}

void player_tick(uint8_t d) {
    ++gobtimer[d];

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

        if (gobtimer[d]>8) {
            gobtimer[d] = 0;
            uint8_t shot = shot_alloc();
            if (shot) {
                gobkind[shot] = GK_SHOT;
                gobx[shot] = gobx[d];
                goby[shot] = goby[d];
                gobdat[shot] = gobdat[d];   // direction
                gobtimer[shot] = 16;
            }
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


