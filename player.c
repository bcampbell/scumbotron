#include "player.h"
#include "effects.h"
#include "gob.h"
#include "input.h"
#include "misc.h"
#include "plat.h"

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
uint8_t plrweapon[MAX_PLAYERS];
uint8_t plrweaponage[MAX_PLAYERS];

int16_t plrhistx[MAX_PLAYERS][PLR_HIST_LEN];
int16_t plrhisty[MAX_PLAYERS][PLR_HIST_LEN];
uint8_t plrhistidx[MAX_PLAYERS];

// shot vars
int16_t shotx[MAX_SHOTS];
int16_t shoty[MAX_SHOTS];
int16_t shotvx[MAX_SHOTS];
int16_t shotvy[MAX_SHOTS];
uint8_t shotdir[MAX_SHOTS];
uint8_t shottimer[MAX_SHOTS];
uint8_t shotkind[MAX_SHOTS];

static uint8_t shot_alloc();

#define NUM_WEAPONS 3
//static uint8_t weapon_fire_delay[NUM_WEAPONS] = {8, 7, 6};
static uint8_t weapon_fire_delay[NUM_WEAPONS] = {5,5,5};

// player

//#define PLAYER_ACCEL (FX_ONE/4)
#define PLAYER_MAXSPD (FX_ONE + (FX_ONE/2))

// Dampen the player velocity
static inline int16_t dampvel(int16_t v) {
    int16_t f = (v>>3); // subtract 1/8th of speed
    if (f == 0) {
        v = 0;
    } else {
        v -= f;
    }
    return v;
}

void player_add_score(uint8_t points)
{
    player_score += points;
}

void player_create(uint8_t p) {
    player_reset(p);
    // One-time init stuff (things that persists between levels).
    plrweapon[p] = 0;
    plrweaponage[p] = 0;
}

void player_reset(uint8_t p) {

    const int16_t x = ((SCREEN_W / 2) - 8) << FX;
    const int16_t y = ((SCREEN_H / 2) - 8) << FX;
    uint8_t i;
    plrx[p] = x;
    plry[p] = y;
    plrvx[p] = 0;
    plrvy[p] = 0;
    plrfacing[p] = 0;
    plrtimer[p] = 0;
    plralive[p] = 1;

    // reset the history list
    for (i = 0; i < PLR_HIST_LEN; ++i) {
        plrhistx[p][i] = x;
        plrhisty[p][i] = y;
    }
    plrhistidx[p] = 0;
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
        plat_player_render(plrx[p], plry[p], plrfacing[p], moving);
    }

    for (s = 0; s < MAX_SHOTS; ++s) {
        if (!shottimer[s]) {
            // inactive.
            continue;
        }
        plat_shot_render(shotx[s], shoty[s], shotdir[s], shotkind[s]);
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
        if (!shottimer[s]) {
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
                if (gob_playercollide(d, p)) {
                    // boom
                    norwegian_blue = true;
                    break;
                }
            }
            // Zapper beams are a special case.
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
            effects_add(px0+(8<<FX), py0+(8<<FX), EK_KABOOM);
        }
    }
    return norwegian_blue;
}

// give player an extra life
void player_extra_life(uint8_t plr)
{
    ++player_lives;
}

// increase player weapon
void player_powerup(uint8_t plr)
{
    if (plrweapon[plr] < (NUM_WEAPONS-1)) {
        plrweapon[plr]++;
        plrweaponage[plr] = 0;
    }
}




// Control schemes:
// 1. keyboard/digital gamepad
// 2. keyboard+mouse
// 3. analog twin stick
//

static void plr_digital_move(uint8_t d, uint8_t move)
{
    // move
    {
        int16_t vy = 0;
        if (move & DIR_UP) {
            vy = -PLAYER_MAXSPD;
        }
        if (move & DIR_DOWN) {
            vy = PLAYER_MAXSPD;
        }
        plry[d] += vy;
        plrvy[d] = vy;
    }

    {
        int16_t vx = 0;
        if (move & DIR_LEFT) {
            vx = -PLAYER_MAXSPD;
        }
        if (move & DIR_RIGHT) {
            vx = PLAYER_MAXSPD;
        }
        plrx[d] += vx;
        plrvx[d] = vx;
    }
    // end of non-intertial movement


    if (move) {
        plrfacing[d] =  move;
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


static void plr_shoot(uint8_t p, uint8_t theta) {
    uint8_t s = shot_alloc();
    // FIRE!
    plat_sfx_play(SFX_LASER);
    plrtimer[p] = 0;
    if (s >= MAX_SHOTS) {
        return;
    }
    shottimer[s] = 24;
    shotx[s] = plrx[p];
    shoty[s] = plry[p];
    shotvx[s] = sin24(theta) * 4;
    shotvy[s] = -cos24(theta) * 4;
    shotdir[s] = theta;
    shotkind[s] = plrweapon[p];
}

void player_tick(uint8_t p) {
    uint8_t sticks = inp_dualstick;
    uint8_t move = sticks & 0x0F;
    uint8_t fire = (sticks>>4) & 0x0F;

    plr_digital_move(p, move);

    // downgrade weapon (update every 4th frame)
    if((tick & 0x03) == 0 && plrweapon[p] > 0) {
      if (++plrweaponage[p] >= 200) {
        // downgrade.
        --plrweapon[p];
        plrweaponage[p] = 0;
      }
    }


    // check firing
    if (plrtimer[p] >= weapon_fire_delay[plrweapon[p]]) {
        if (fire) {
            uint8_t theta = dir_to_angle24[fire];
            plr_shoot(p, theta);
            plrtimer[p] = 0;
        }
        #ifdef PLAT_HAS_MOUSE
        if (!fire && plat_mouse_buttons) {
            int16_t dx = plat_mouse_x - plrx[p];
            int16_t dy = plat_mouse_y - plry[p];
            uint8_t theta = arctan24(dx, dy);
            plr_shoot(p, theta);
            plrtimer[p] = 0;
        }
        #endif // PLAT_HAS_MOUSE
    } else {
        ++plrtimer[p];
    }

    // Update position history array (every second frame)
//    if ((tick & 0x01) == 0 ) {
    {
        uint8_t idx = plrhistidx[p];
        if (plrx[p] != plrhistx[p][idx] || plry[p] != plrhisty[p][idx]) {
            // add new entry
            idx = (idx + 1) & (PLR_HIST_LEN-1);
            plrhistidx[p] = idx;
            plrhistx[p][idx] = plrx[p];
            plrhisty[p][idx] = plry[p];
        }
    }
}


void shot_clearall()
{
    uint8_t s;
    for(s = 0; s < MAX_SHOTS; ++s) {
        shottimer[s] = 0;
    }
}

// returns 0 if none free.
static uint8_t shot_alloc()
{
    uint8_t s;
    for(s = 0; s < MAX_SHOTS; ++s) {
        if(shottimer[s] == 0) {
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
    if( --shottimer[s] == 0) {
        return;
    }
    shotx[s] += shotvx[s];
    shoty[s] += shotvy[s];
}

void shot_collisions()
{
    uint8_t s;
    for (s = 0; s < MAX_SHOTS; ++s) {
        // Take centre point of shot.
        int16_t sx = shotx[s] + (8<<FX);
        int16_t sy = shoty[s] + (8<<FX);
        uint8_t d;
        if (shottimer[s] == 0) {
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
                    shottimer[s] = 0; // turn off shot.
                    break;  // next shot.
                }
            }
        }
    }
}

