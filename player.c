#include "player.h"
#include "sys.h"
#include "gob.h"

uint8_t player_lives;
uint32_t player_score;

// player vars
int16_t plrx[MAX_PLAYERS];
int16_t plry[MAX_PLAYERS];
int16_t plrvx[MAX_PLAYERS];
int16_t plrvy[MAX_PLAYERS];
uint8_t plrtimer[MAX_PLAYERS];
uint8_t plrfacing[MAX_PLAYERS]; // DIR_ bits
uint8_t plralive[MAX_PLAYERS]; // 0=dead

// shot vars
int16_t shotx[MAX_SHOTS];
int16_t shoty[MAX_SHOTS];
uint8_t shotdir[MAX_SHOTS];
uint8_t shottimer[MAX_SHOTS];

static uint8_t shot_alloc();

// player
#define PLAYER_SPD (2*FX_ONE);

void player_add_score(uint8_t points)
{
    player_score += points;
}

void player_create(uint8_t p, int16_t x, int16_t y) {
    plrx[p] = x;
    plry[p] = y;
    plrvx[p] = 0;
    plrvy[p] = 0;
    plrfacing[p] = 0;
    plrtimer[p] = 0;
    plralive[p] = 1;
}

void player_renderall()
{
    uint8_t p;
    uint8_t s;
    for (p = 0; p < MAX_PLAYERS; ++p) {
        if (!plralive[p]) {
            // dead.
            continue;
        }
        bool moving = (plrvx[p] != 0) || (plrvy[p] != 0);
        sys_player_render(plrx[p], plry[p], plrfacing[p], moving);
    }

    for (s = 0; s < MAX_SHOTS; ++s) {
        if (!shotdir[s]) {
            // inactive.
            continue;
        }
        sys_shot_render(shotx[s], shoty[s], shotdir[s]);
    }
}

void player_tickall()
{
    uint8_t p;
    uint8_t s;
    for (p = 0; p < MAX_PLAYERS; ++p) {
        if (!plralive[p]) {
            // dead.
            continue;
        }
        player_tick(p);
    }
    for (s = 0; s < MAX_SHOTS; ++s) {
        if (!shotdir[s]) {
            // inactive.
            continue;
        }
        shot_tick(s);
    }
}

// return true if ranges overlap
static inline bool overlap(int16_t amin, int16_t amax, int16_t bmin, int16_t bmax)
{
    return (amin <= bmax) && (amax >= bmin);
}


// returns true if player killed.
bool player_collisions()
{
    bool norwegian_blue = false;
    uint8_t p;
    for (p = 0; p < MAX_PLAYERS; ++p) {
        int16_t px0 = plrx[p] + (4 << FX);
        int16_t py0 = plry[p] + (4 << FX);
        int16_t px1 = plrx[p] + (12 << FX);
        int16_t py1 = plry[p] + (12 << FX);
        uint8_t d;
        for (d = 0; d < MAX_GOBS; ++d) {
            if (gobkind[d]==GK_NONE || (gobflags[d] & GF_COLLIDES_PLAYER) == 0) {
                continue;
            }
            if (overlap(px0, px1, gobx[d], gobx[d] + gob_size(d)) &&
                overlap(py0, py1, goby[d], goby[d] + gob_size(d))) {
                if (gobkind[d] == GK_POWERUP) {
                    player_lives++;
                    gobkind[d] = GK_NONE;
                } else {
                    // boom
                    norwegian_blue = true;
                }
                break;
            }
            // special checks for zapper beams
            if (gobkind[d] == GK_VZAPPER && zapper_state(d) == ZAPPER_ON) {
                int16_t xzap = gobx[d] + (8<<FX);
                if (px0 <= xzap && px1 >= xzap) {
                    norwegian_blue = true;
                    break;
                }
            }
            if (gobkind[d] == GK_HZAPPER && zapper_state(d) == ZAPPER_ON) {
                int16_t yzap = goby[d] + (8<<FX);
                if (py0 <= yzap && py1 >= yzap) {
                    norwegian_blue = true;
                    break;
                }
            }
        }
        if (norwegian_blue) {
            plralive[p] = 0;    // mark as dead
            sys_addeffect(px0+(8<<FX), py0+(8<<FX), EK_KABOOM);
        }
    }
    return norwegian_blue;
}



void player_tick(uint8_t d) {
    uint8_t sticks = sys_inp_dualsticks();
    uint8_t move = dir_fix[sticks & 0x0F];
    uint8_t fire = dir_fix[(sticks>>4) & 0x0F];

    ++plrtimer[d];

    // move
    plrvx[d] = 0;
    plrvy[d] = 0;
    if (move & DIR_UP) {
        plrvy[d] = -PLAYER_SPD;
    } else if (move & DIR_DOWN) {
        plrvy[d] = PLAYER_SPD;
    }
    if (move & DIR_LEFT) {
        plrvx[d] = -PLAYER_SPD;
    } else if (move & DIR_RIGHT) {
        plrvx[d] = PLAYER_SPD;
    }

    plrx[d] += plrvx[d];
    plry[d] += plrvy[d];

    if (move) {
        plrfacing[d] =  move;
    }

    // fire
    if (fire) {
        if (plrtimer[d]>8) {
            uint8_t shot = shot_alloc();
            // FIRE!
            sys_sfx_play(SFX_LASER);
            plrtimer[d] = 0;
            if (shot < MAX_SHOTS) {
                shotx[shot] = plrx[d];
                shoty[shot] = plry[d];
                shotdir[shot] = fire;
                shottimer[shot] = 16;
            }
        }
    }

    // keep player on screen
    {
        const int16_t xmax = (SCREEN_W - 16) << FX;
        if (plrx[d] < 0<<FX) {
            plrx[d] = 0;
        } else if (plrx[d] > xmax) {
            plrx[d] = xmax;
        }
    }
    {
        const int16_t ymax = (SCREEN_H - 16) << FX;
        if (plry[d] < 0<<FX) {
            plry[d] = 0;
        } else if (plry[d] > ymax) {
            plry[d] = ymax;
        }
    }
}


void shot_clearall()
{
    uint8_t s;
    for(s = 0; s < MAX_SHOTS; ++s) {
        shotdir[s] = 0;
    }
}

// returns 0 if none free.
static uint8_t shot_alloc()
{
    uint8_t s;
    for(s = 0; s < MAX_SHOTS; ++s) {
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
    uint8_t s;
    for (s = 0; s < MAX_SHOTS; ++s) {
        // Take centre point of shot.
        int16_t sx = shotx[s] + (8<<FX);
        int16_t sy = shoty[s] + (8<<FX);
        uint8_t d;
        if (shotdir[s] == 0) {
            continue;   // inactive
        }
        for (d = 0; d < MAX_GOBS; ++d) {
            int16_t dy0 = goby[d];
            int16_t dy1 = goby[d] + gob_size(d);
            if (gobkind[d] == GK_NONE || (gobflags[d] & GF_COLLIDES_SHOT) == 0) {
                continue;
            }
            if (sy >= dy0 && sy < dy1) {
                int16_t dx0 = gobx[d];
                int16_t dx1 = gobx[d] + gob_size(d);
                if (sx >= dx0 && sx < dx1) {
                    // A hit!
                    gob_shot(d, s);
                    shotdir[s] = 0; // turn off shot.
                    break;  // next shot.
                }
            }
        }
    }
}

