#include "gob.h"
#include "sys_cx16.h"

// gob tables
uint8_t gobkind[MAX_GOBS];
uint8_t gobflags[MAX_GOBS];
int16_t gobx[MAX_GOBS];
int16_t goby[MAX_GOBS];
int16_t gobvx[MAX_GOBS];   // SHOULD BE BYTES?
int16_t gobvy[MAX_GOBS];
uint8_t gobdat[MAX_GOBS];
uint8_t gobtimer[MAX_GOBS];

// these two set by gobs_tick()
uint8_t gobs_lockcnt;   // num dudes holding level open.
uint8_t gobs_spawncnt;  // num dudes spawning.


uint8_t player_lives;
uint32_t player_score;


// by darsie,
// https://www.avrfreaks.net/forum/tiny-fast-prng
uint8_t rnd() {
        static uint8_t s=0xaa,a=0;
        s^=s<<3;
        s^=s>>5;
        s^=a++>>2;
        return s;
}

void gobs_init()
{
    for (uint8_t i = 0; i < MAX_GOBS; ++i) {
        gobkind[i] = GK_NONE;
    }
    gobs_lockcnt = 0;
    gobs_spawncnt = 0;
}

void gobs_tick(bool spawnphase)
{
    gobs_lockcnt = 0;
    gobs_spawncnt = 0;
    for (uint8_t i = 0; i < MAX_GOBS; ++i) {
        // Is gob playing its spawning-in effect?
        if (gobkind[i] & GK_SPAWNFLAG) {
            ++gobs_spawncnt;
            ++gobs_lockcnt;
            if( gobtimer[i] == 8) {
                sys_addeffect(gobx[i], goby[i], EK_SPAWN);
            } else if( gobtimer[i] == 0) {
                // done spawning.
                gobkind[i] &= ~GK_SPAWNFLAG;
                gobtimer[i] = 1;
            }
            --gobtimer[i];
            continue;
        }
        // If in spawn phase, keep everything frozen until we finish.
        if (spawnphase) {
            continue;
        }
        switch(gobkind[i]) {
            case GK_NONE:
                break;
            case GK_PLAYER:
                player_tick(i);
                break;
            case GK_SHOT:
                shot_tick(i);
                break;
            case GK_BLOCK:
                break;
            case GK_GRUNT:
                ++gobs_lockcnt;
                grunt_tick(i);
                break;
            case GK_BAITER:
                ++gobs_lockcnt;
                baiter_tick(i);
                break;
            default:
                break;
        }
    }
}

void gobs_render()
{
    for (uint8_t d = 0; d < MAX_GOBS; ++d) {
        switch(gobkind[d]) {
            case GK_NONE:
                sproff();
                break;
            case GK_PLAYER:
                sprout(gobx[d], goby[d], 0);
                break;
            case GK_SHOT:
                sprout(gobx[d], goby[d], 1);
                break;
            case GK_BLOCK:
                sprout(gobx[d], goby[d], 2);
                break;
            case GK_GRUNT:
                sprout(gobx[d], goby[d],  3 + ((tick >> 5) & 0x01));
                break;
            case GK_BAITER:
                sprout(gobx[d], goby[d],  2 + ((tick >> 5) & 0x03));
                break;
            default: // includes spawning GK_SPAWNFLAG dudes
                sproff();
                break;
        }
    }
}

void dudes_spawn(uint8_t kind, uint8_t n)
{
    while(n--) {
        uint8_t g = dude_alloc();
        if (!g) {
            return;
        }
        // dude creation
        gobkind[g] = kind;
        gobdat[g] = 0;
        gobtimer[g] = 0;
        // position set by respawn
    }
}



// for all dudes:
// - set to new random position.
// - put into spawning mode.
void dudes_reset() {
    uint8_t t=0;
    for (uint8_t g = FIRST_DUDE; g < FIRST_DUDE+MAX_DUDES; ++g) {
        if (gobkind[g] == GK_NONE) {
            continue;
        }
        dude_randompos(g);
        gobvx[g] = 0;
        gobvy[g] = 0;
        gobkind[g] |= GK_SPAWNFLAG;
        gobtimer[g] = t + 8;
        t += 2;
    }
}


// returns 0 if none free.
uint8_t shot_alloc() {
    for(uint8_t d = MAX_PLAYERS; d < MAX_PLAYERS + MAX_SHOTS; ++d) {
        if(gobkind[d] == GK_NONE) {
            return d;
        }
    }
    return 0;
}

// returns 0 if none free.
uint8_t dude_alloc() {
    for(uint8_t d = MAX_PLAYERS+MAX_SHOTS; d < MAX_GOBS; ++d) {
        if(gobkind[d] == GK_NONE) {
            return d;
        }
    }
    return 0;
}

void dude_randompos(uint8_t d) {
    const int16_t xmid = (SCREEN_W/2)-8;
    const int16_t ymid = (SCREEN_H/2)-8;

    int16_t x;
    int16_t y;
    while (1) {
        x = -128 + rnd();
        y = -128 + rnd();

        int16_t lsq = x*x + y*y;
        if (lsq > 80*80 && lsq <100*100 ) {
            break;
        }
    }

    gobx[d] = (xmid + x) << FX;
    goby[d] = (ymid + y) << FX;
}

// hackhackhack (from cx16.h)
#define JOY_UP_MASK 0x08
#define JOY_DOWN_MASK 0x04
#define JOY_LEFT_MASK 0x02
#define JOY_RIGHT_MASK 0x01
#define JOY_BTN_1_MASK 0x80


// player
#define PLAYER_SPD FX_ONE


void player_create(uint8_t d, int x, int y) {
    gobkind[d] = GK_PLAYER;
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
            int16_t dx = gobx[d];
            int16_t dy = goby[d];
            if (sx >= dx && sx < (dx + (16<<FX)) && sy >= dy && sy < (dy + (16<<FX))) {
                // boom.
                gobkind[d] = GK_NONE;
                gobkind[s] = GK_NONE;
                break;  // next shot.
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



// block has no tick fn

// grunt
void grunt_tick(uint8_t d)
{
    const int16_t GRUNT_SPD = 2 << FX;
    gobdat[d]++;
    if (gobdat[d]>13) {
        gobdat[d] = 0;
        int16_t px = gobx[0];
        if (px < gobx[d]) {
            gobx[d] -= GRUNT_SPD;
        } else if (px > gobx[d]) {
            gobx[d] += GRUNT_SPD;
        }
        int16_t py = goby[0];
        if (py < goby[d]) {
            goby[d] -= GRUNT_SPD;
        } else if (py > goby[d]) {
            goby[d] += GRUNT_SPD;
        }
    }
}


// baiter
void baiter_tick(uint8_t d)
{
    const int16_t BAITER_MAX_SPD = 4 << FX;
    const int16_t BAITER_ACCEL = 2;
    int16_t px = gobx[0];
    if (px < gobx[d] && gobvx[d] > -BAITER_MAX_SPD) {
        gobvx[d] -= BAITER_ACCEL;
    } else if (px > gobx[d] && gobvx[d] < BAITER_MAX_SPD) {
        gobvx[d] += BAITER_ACCEL;
    }
    gobx[d] += gobvx[d];

    int16_t py = goby[0];
    if (py < goby[d] && gobvy[d] > -BAITER_MAX_SPD) {
        gobvy[d] -= BAITER_ACCEL;
    } else if (py > goby[d] && gobvy[d] < BAITER_MAX_SPD) {
        gobvy[d] += BAITER_ACCEL;
    }
    goby[d] += gobvy[d];

}

