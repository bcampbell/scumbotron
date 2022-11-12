#include "gob.h"
#include "sys_cx16.h"
#include "player.h"

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


// gob-specific functions
void grunt_tick(uint8_t d);
void baiter_tick(uint8_t d);
void amoeba_tick(uint8_t d);
void amoeba_shot(uint8_t d);

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
        if (gobflags[i] & GF_SPAWNING) {
            ++gobs_spawncnt;
            ++gobs_lockcnt;
            if( gobtimer[i] == 8) {
                sys_addeffect(gobx[i], goby[i], EK_SPAWN);
            } else if( gobtimer[i] == 0) {
                // done spawning.
                gobflags[i] &= ~GF_SPAWNING;
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
            case GK_AMOEBA_BIG:
            case GK_AMOEBA_MED:
            case GK_AMOEBA_SMALL:
                ++gobs_lockcnt;
                amoeba_tick(i);
                break;
            default:
                break;
        }
    }
}

void gobs_render()
{
    for (uint8_t d = 0; d < MAX_GOBS; ++d) {
        if (gobflags[d] & GF_SPAWNING) {
            continue;
        }
        switch(gobkind[d]) {
            case GK_NONE:
                sproff();
                break;
            case GK_SHOT:
                sprout(gobx[d], goby[d], shot_spr[gobdat[d]]);
                break;
            case GK_BLOCK:
                sprout(gobx[d], goby[d], 2);
                break;
            case GK_GRUNT:
                sprout(gobx[d], goby[d],  SPR16_GRUNT + ((tick >> 5) & 0x01));
                break;
            case GK_BAITER:
                sprout(gobx[d], goby[d],  SPR16_BAITER + ((tick >> 2) & 0x03));
                break;
            case GK_AMOEBA_BIG:
                sys_spr32(gobx[d], goby[d],  SPR32_AMOEBA_BIG + ((tick >> 3) & 0x01));
                break;
            case GK_AMOEBA_MED:
                sprout(gobx[d], goby[d],  SPR16_AMOEBA_MED + ((tick >> 3) & 0x03));
                break;
            case GK_AMOEBA_SMALL:
                sprout(gobx[d], goby[d],  SPR16_AMOEBA_SMALL + ((tick >> 3) & 0x03));
                break;
            default:
                sproff();
                break;
        }
    }
}

void dudes_spawn(uint8_t kind, uint8_t n)
{
    while(n--) {
        uint8_t d = dude_alloc();
        if (!d) {
            return;
        }
        switch (kind) {
            case GK_BLOCK:
                block_init(d);
                break;
            case GK_GRUNT:
                grunt_init(d);
                break;
            case GK_BAITER:
                block_init(d);
                break;
            case GK_AMOEBA_BIG:
            case GK_AMOEBA_MED:
            case GK_AMOEBA_SMALL:
                amoeba_init(d);
                break;
            default:
                break;
        }
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
        gobflags[g] |= GF_SPAWNING;
        gobtimer[g] = t + 8;
        t += 2;
    }
}

// returns 0 if none free.
uint8_t dude_alloc() {
    for(uint8_t d = FIRST_DUDE; d < FIRST_DUDE + MAX_DUDES; ++d) {
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


// block

void block_init(uint8_t d)
{
    gobkind[d] = GK_BLOCK;
    gobflags[d] = 0;
    gobx[d] = 0;
    goby[d] = 0;
    gobvx[d] = 0;
    gobvy[d] = 0;
    gobdat[d] = 0;
    gobtimer[d] = 0;
}


// grunt

void grunt_init(uint8_t d)
{
    gobkind[d] = GK_GRUNT;
    gobflags[d] = 0;
    gobx[d] = 0;
    goby[d] = 0;
    gobvx[d] = (2 << FX) + (rnd() & 0x7f);
    gobvy[d] = (2 << FX) + (rnd() & 0x7f);
    gobdat[d] = 0;
    gobtimer[d] = 0;
}


void grunt_tick(uint8_t d)
{
    // update every 16 frames
    if (((tick+d) & 0x0f) != 0x00) {
       return;
    }
    int16_t px = plrx[0];
    if (px < gobx[d]) {
        gobx[d] -= gobvx[d];
    } else if (px > gobx[d]) {
        gobx[d] += gobvx[d];
    }
    int16_t py = plry[0];
    if (py < goby[d]) {
        goby[d] -= gobvy[d];
    } else if (py > goby[d]) {
        goby[d] += gobvy[d];
    }
}


// baiter
void baiter_init(uint8_t d)
{
    gobkind[d] = GK_BAITER;
    gobflags[d] = 0;
    gobx[d] = 0;
    goby[d] = 0;
    gobvx[d] = 0;
    gobvy[d] = 0;
    gobdat[d] = 0;
    gobtimer[d] = 0;
}

void baiter_tick(uint8_t d)
{
    const int16_t BAITER_MAX_SPD = 4 << FX;
    const int16_t BAITER_ACCEL = 2;
    int16_t px = plrx[0];
    if (px < gobx[d] && gobvx[d] > -BAITER_MAX_SPD) {
        gobvx[d] -= BAITER_ACCEL;
    } else if (px > gobx[d] && gobvx[d] < BAITER_MAX_SPD) {
        gobvx[d] += BAITER_ACCEL;
    }
    gobx[d] += gobvx[d];

    int16_t py = plry[0];
    if (py < goby[d] && gobvy[d] > -BAITER_MAX_SPD) {
        gobvy[d] -= BAITER_ACCEL;
    } else if (py > goby[d] && gobvy[d] < BAITER_MAX_SPD) {
        gobvy[d] += BAITER_ACCEL;
    }
    goby[d] += gobvy[d];

}

// amoeba
void amoeba_init(uint8_t d)
{
    gobkind[d] = GK_AMOEBA_BIG;
    gobflags[d] = 0;
    gobx[d] = 0;
    goby[d] = 0;
    gobvx[d] = 0;
    gobvy[d] = 0;
    gobdat[d] = 0;
    gobtimer[d] = 0;
}

void amoeba_tick(uint8_t d)
{
    // jitter acceleration
    gobvx[d] += (rnd() - 128)>>2;
    gobvy[d] += (rnd() - 128)>>2;


    // apply drag
    int16_t vx = gobvx[d];
    gobvx[d] = (vx >> 1) + (vx >> 2);
    int16_t vy = gobvy[d];
    gobvy[d] = (vy >> 1) + (vy >> 2);
    
    const int16_t AMOEBA_MAX_SPD = (4<<FX)/3;
    const int16_t AMOEBA_ACCEL = 2 * FX_ONE / 2;

    gobx[d] += gobvx[d];
    goby[d] += gobvy[d];

    // update homing every 16 frames
    if (((tick+d) & 0x0f) != 0x00) {
       return;
    }
    int16_t px = plrx[0];
    if (px < gobx[d] && gobvx[d] > -AMOEBA_MAX_SPD) {
        gobvx[d] -= AMOEBA_ACCEL;
    } else if (px > gobx[d] && gobvx[d] < AMOEBA_MAX_SPD) {
        gobvx[d] += AMOEBA_ACCEL;
    }

    int16_t py = plry[0];
    if (py < goby[d] && gobvy[d] > -AMOEBA_MAX_SPD) {
        gobvy[d] -= AMOEBA_ACCEL;
    } else if (py > goby[d] && gobvy[d] < AMOEBA_MAX_SPD) {
        gobvy[d] += AMOEBA_ACCEL;
    }



}

void amoeba_spawn(uint8_t kind, int16_t x, int16_t y, int16_t vx, int16_t vy) {
    uint8_t d = dude_alloc();
    if (!d) {
        return;
    }
    gobkind[d] = kind;
    gobx[d] = x;
    goby[d] = y;
    gobvx[d] = vx;
    gobvy[d] = vy;
    gobdat[d] = 0;
    gobtimer[d] = 0;
}

void amoeba_shot(uint8_t d)
{
    if (gobkind[d] == GK_AMOEBA_SMALL) {
        gobkind[d] = GK_NONE;
        return;
    }

    if (gobkind[d] == GK_AMOEBA_MED) {
        const int16_t v = FX<<5;
        gobkind[d] = GK_NONE;
        amoeba_spawn(GK_AMOEBA_SMALL, gobx[d], goby[d], -v, v);
        amoeba_spawn(GK_AMOEBA_SMALL, gobx[d], goby[d], v, v);
        amoeba_spawn(GK_AMOEBA_SMALL, gobx[d], goby[d], 0, -v);
        return;
    }

    if (gobkind[d] == GK_AMOEBA_BIG) {
        const int16_t v = FX<<5;
        gobkind[d] = GK_NONE;
        amoeba_spawn(GK_AMOEBA_MED, gobx[d], goby[d], -v, v);
        amoeba_spawn(GK_AMOEBA_MED, gobx[d], goby[d], v, v);
        amoeba_spawn(GK_AMOEBA_MED, gobx[d], goby[d], 0, -v);
        return;
    }
}
