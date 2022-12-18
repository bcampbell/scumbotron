#include "gob.h"
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
void tank_tick(uint8_t d);

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
            case GK_TANK:
                ++gobs_lockcnt;
                tank_tick(i);
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
                break;
            case GK_BLOCK:
                sys_block_render(gobx[d], goby[d]);
                break;
            case GK_GRUNT:
                sys_grunt_render(gobx[d], goby[d]);
                break;
            case GK_BAITER:
                sys_baiter_render(gobx[d], goby[d]);
                break;
            case GK_AMOEBA_BIG:
                sys_amoeba_big_render(gobx[d], goby[d]);
                break;
            case GK_AMOEBA_MED:
                sys_amoeba_med_render(gobx[d], goby[d]);
                break;
            case GK_AMOEBA_SMALL:
                sys_amoeba_small_render(gobx[d], goby[d]);
                break;
            case GK_TANK:
                sys_tank_render(gobx[d], goby[d], gobtimer[d] > 0);
                break;
            default:
                break;
        }
    }
}

void dudes_spawn(uint8_t kind, uint8_t n)
{
    while(n--) {
        uint8_t d = dude_alloc();
        if (d >= MAX_GOBS) {
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
                baiter_init(d);
                break;
            case GK_AMOEBA_BIG:
            case GK_AMOEBA_MED:
            case GK_AMOEBA_SMALL:
                amoeba_init(d);
                break;
            case GK_TANK:
                tank_init(d);
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
    for (uint8_t g = 0; g < MAX_GOBS; ++g) {
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
    for(uint8_t d = 0; d < MAX_GOBS; ++d) {
        if(gobkind[d] == GK_NONE) {
            return d;
        }
    }
    return MAX_GOBS;
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
    if (d >= MAX_GOBS) {
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

void amoeba_shot(uint8_t d, uint8_t shot)
{
    if (gobkind[d] == GK_AMOEBA_SMALL) {
        gobkind[d] = GK_NONE;
        sys_sfx_play(SFX_KABOOM);
        sys_addeffect(gobx[d], goby[d], EK_KABOOM);
        return;
    }

    if (gobkind[d] == GK_AMOEBA_MED) {
        const int16_t v = FX<<5;
        gobkind[d] = GK_NONE;
        sys_sfx_play(SFX_KABOOM);
        amoeba_spawn(GK_AMOEBA_SMALL, gobx[d], goby[d], -v, v);
        amoeba_spawn(GK_AMOEBA_SMALL, gobx[d], goby[d], v, v);
        amoeba_spawn(GK_AMOEBA_SMALL, gobx[d], goby[d], 0, -v);
        return;
    }

    if (gobkind[d] == GK_AMOEBA_BIG) {
        const int16_t v = FX<<5;
        gobkind[d] = GK_NONE;
        sys_sfx_play(SFX_KABOOM);
        amoeba_spawn(GK_AMOEBA_MED, gobx[d], goby[d], -v, v);
        amoeba_spawn(GK_AMOEBA_MED, gobx[d], goby[d], v, v);
        amoeba_spawn(GK_AMOEBA_MED, gobx[d], goby[d], 0, -v);
        return;
    }
}


// tank

void tank_init(uint8_t d)
{
    gobkind[d] = GK_TANK;
    gobflags[d] = 0;
    gobx[d] = 0;
    goby[d] = 0;
    gobvx[d] = 0;
    gobvy[d] = 0;
    gobdat[d] = 8;  // life
    gobtimer[d] = 0;    // highlight timer
}


void tank_tick(uint8_t d)
{
    if (gobtimer[d] > 0) {
        --gobtimer[d];
    }
    // update every 32 frames
    if (((tick+d) & 0x1f) != 0x00) {
       return;
    }
    const int16_t vx = (5<<FX);
    const int16_t vy = (5<<FX);
    int16_t px = plrx[0];
    if (px < gobx[d]) {
        gobx[d] -= vx;
    } else if (px > gobx[d]) {
        gobx[d] += vx;
    }
    int16_t py = plry[0];
    if (py < goby[d]) {
        goby[d] -= vy;
    } else if (py > goby[d]) {
        goby[d] += vy;
    }
}

void tank_shot(uint8_t d, uint8_t s)
{
    --gobdat[d];
    if (gobdat[d]==0) {
        // boom.
        gobkind[d] = GK_NONE;
        sys_sfx_play(SFX_KABOOM);
        sys_addeffect(gobx[d], goby[d], EK_KABOOM);
    } else {
        // knockback
        gobx[d] += (shot_xvel(s) >> 0);
        goby[d] += (shot_yvel(s) >> 0);
        gobtimer[d] = 8;
    }

}

