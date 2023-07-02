#include "gob.h"
#include "plat.h"
#include "player.h"
#include "misc.h"
#include "bub.h"

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
uint8_t gobs_lockcnt;   // num gobs holding level open.
uint8_t gobs_spawncnt;  // num gobs spawning.


// Force direction into valid bits (no up+down, for example)
const uint8_t dir_fix[16] = {
    0,              // 0000
    DIR_RIGHT,      // 0001 right
    DIR_LEFT,       // 0010 left
    0,              // 0011 left+right
    DIR_DOWN,       // 0100 down
    DIR_DOWN|DIR_RIGHT,   // 0101 down+right           
    DIR_DOWN|DIR_LEFT,   // 0110 down+left           
    0,              // 0111 down+left+right

    DIR_UP,         // 1000 up
    DIR_UP|DIR_RIGHT,   // 1001 up+right
    DIR_UP|DIR_LEFT,   // 1010 up+left
    0,              // 1011 up+left+right
    0,              // 1100 up+down
    0,              // 1101 up+down+right           
    0,              // 1110 up+down+left           
    0,              // 1111 all
};

#define ANGLE_DUD 0 // Shouldn't be used, but want to show invalid values.

// Convert DIR_ bits into angle (0..23)
// invalid directions return 0xff
const uint8_t dir_to_angle24[16] = {
    ANGLE_DUD,   // 0000
    6,         // 0001 right
    18,        // 0010 left
    ANGLE_DUD, // 0011 left+right
    12,        // 0100 down
    6 + 3,     // 0101 down+right           
    12 + 3,    // 0110 down+left           
    ANGLE_DUD, // 0111 down+left+right

    0,         // 1000 up
    0 + 3,     // 1001 up+right
    18 + 3,    // 1010 up+left
    ANGLE_DUD, // 1011 up+left+right
    ANGLE_DUD, // 1100 up+down
    ANGLE_DUD, // 1101 up+down+right           
    ANGLE_DUD, // 1110 up+down+left           
    ANGLE_DUD, // 1111 all
};

void gobs_clear()
{
    uint8_t i;
    for (i = 0; i < MAX_GOBS; ++i) {
        gobkind[i] = GK_NONE;
    }
    gobs_lockcnt = 0;
    gobs_spawncnt = 0;
    bub_clear();
}

void gobs_tick(bool spawnphase)
{
    uint8_t i;
    gobs_lockcnt = 0;
    gobs_spawncnt = 0;
    for (i = 0; i < MAX_GOBS; ++i) {
        if (gobkind[i] == GK_NONE) {
            continue;
        }
        // Is gob playing its spawning-in effect?
        if (gobflags[i] & GF_SPAWNING) {
            ++gobs_spawncnt;
            if( gobtimer[i] == 8) {
                if (gobkind[i] != GK_BOSSTAIL) {
                    plat_addeffect(gobx[i]+(8<<FX), goby[i]+(8<<FX), EK_SPAWN);
                }
            } else if( gobtimer[i] == 0) {
                // done spawning.
                gobflags[i] &= ~GF_SPAWNING;
                gobtimer[i] = 1;
            }
            --gobtimer[i];
            continue;
        }
        if (gobflags[i] & GF_LOCKS_LEVEL) {
            ++gobs_lockcnt;
        }
        // Decrease highlight timer (lower 3 bits) if non-zero.
        if (gobflags[i] & GF_HIGHLIGHT_MASK) {
            --gobflags[i];
        }

        // If in spawn phase, keep everything frozen until we finish.
        if (spawnphase) {
            continue;
        }
        switch(gobkind[i]) {
            case GK_BLOCK: break;
            case GK_GRUNT:   grunt_tick(i); break;
            case GK_BAITER:  baiter_tick(i); break;
            case GK_AMOEBA_BIG:
            case GK_AMOEBA_MED:
            case GK_AMOEBA_SMALL:
                amoeba_tick(i);
                break;
            case GK_TANK:    tank_tick(i); break;
            case GK_HZAPPER:
            case GK_VZAPPER:
                zapper_tick(i);
                break;
            case GK_FRAGGER: fragger_tick(i); break;
            case GK_FRAG:    frag_tick(i); break;
            case GK_POWERUP: powerup_tick(i); break;
            case GK_VULGON:  vulgon_tick(i); break;
            case GK_POOMERANG: poomerang_tick(i); break;
            case GK_HAPPYSLAPPER:  happyslapper_tick(i); break;
            case GK_MARINE:  marine_tick(i); break;
            case GK_BRAIN:  brain_tick(i); break;
            case GK_ZOMBIE:  zombie_tick(i); break;
            case GK_MISSILE:  missile_tick(i); break;
            case GK_BOSS:  boss_tick(i); break;
            case GK_BOSSTAIL:  bosstail_tick(i); break;
            default:
                break;
        }
    }
    bub_tick();
}

void gobs_render()
{
    uint8_t d = MAX_GOBS-1;
    do {
        if (gobflags[d] & GF_SPAWNING) {
            continue;
        }
        switch(gobkind[d]) {
            case GK_NONE:
                break;
            case GK_BLOCK:
                plat_block_render(gobx[d], goby[d]);
                break;
            case GK_GRUNT:
                plat_grunt_render(gobx[d], goby[d]);
                break;
            case GK_BAITER:
                plat_baiter_render(gobx[d], goby[d]);
                break;
            case GK_AMOEBA_BIG:
                plat_amoeba_big_render(gobx[d], goby[d]);
                break;
            case GK_AMOEBA_MED:
                plat_amoeba_med_render(gobx[d], goby[d]);
                break;
            case GK_AMOEBA_SMALL:
                plat_amoeba_small_render(gobx[d], goby[d]);
                break;
            case GK_TANK:
                plat_tank_render(gobx[d], goby[d], gobtimer[d] > 0);
                break;
            case GK_HZAPPER:
                plat_hzapper_render(gobx[d], goby[d], zapper_state(d));
                break;
            case GK_VZAPPER:
                plat_vzapper_render(gobx[d], goby[d], zapper_state(d));
                break;
            case GK_FRAGGER:
                plat_fragger_render(gobx[d], goby[d]);
                break;
            case GK_FRAG:
                plat_frag_render(gobx[d], goby[d], gobdat[d]);
                break;
            case GK_POWERUP:
                plat_powerup_render(gobx[d], goby[d], gobdat[d]);
                break;
            case GK_VULGON:
                plat_vulgon_render(gobx[d], goby[d], gobtimer[d] > 0, gobdat[d]);
                break;
            case GK_POOMERANG:
                plat_poomerang_render(gobx[d], goby[d]);
                break;
            case GK_HAPPYSLAPPER:
                plat_happyslapper_render(gobx[d], goby[d], gobtimer[d] < HAPPYSLAPPER_SLEEP_TIME);
                break;
            case GK_MARINE:
                plat_marine_render(gobx[d], goby[d]);
                break;
            case GK_BRAIN:
                plat_brain_render(gobx[d], goby[d]);
                break;
            case GK_ZOMBIE:
                plat_zombie_render(gobx[d], goby[d]);
                break;
            case GK_MISSILE:
                plat_missile_render(gobx[d], goby[d], gobdat[d]);
                break;
            case GK_BOSS:
                plat_boss_render(gobx[d], goby[d], gobflags[d] & GF_HIGHLIGHT_MASK);
                break;
            case GK_BOSSTAIL:
                plat_bosstail_render(gobx[d], goby[d], d);
                break;
            default:
                break;
        }
    } while(d-- > 0);
    bub_render();
}


// Handle a gob being shot.
// d: gob index
// s: shot index
void gob_shot(uint8_t d, uint8_t s)
{
    switch (gobkind[d]) {
        case GK_BLOCK:        block_shot(d, s); break;
        case GK_GRUNT:        grunt_shot(d, s); break;
        case GK_BAITER:       baiter_shot(d, s); break;
        case GK_AMOEBA_BIG:   amoeba_shot(d, s); break;
        case GK_AMOEBA_MED:   amoeba_shot(d, s); break;
        case GK_AMOEBA_SMALL: amoeba_shot(d, s); break;
        case GK_TANK:         tank_shot(d, s); break;
        case GK_HZAPPER:      zapper_shot(d, s); break;
        case GK_VZAPPER:      zapper_shot(d, s); break;
        case GK_FRAGGER:      fragger_shot(d, s); break;
        case GK_FRAG:         frag_shot(d, s); break;
        case GK_VULGON:       vulgon_shot(d, s); break;
        case GK_POOMERANG:    poomerang_shot(d, s); break;
        case GK_HAPPYSLAPPER: happyslapper_shot(d, s); break;
        case GK_BRAIN:        brain_shot(d, s); break;
        case GK_ZOMBIE:       zombie_shot(d, s); break;
        case GK_MISSILE:      missile_shot(d, s); break;
        case GK_BOSS:         boss_shot(d, s); break;
        case GK_BOSSTAIL:     bosstail_shot(d, s); break;
        default:
            break;
    }
}

// Handle a gob colliding with a player.
// d: gob index
// plr: player index
bool gob_playercollide(uint8_t g, uint8_t plr)
{
    switch (gobkind[g]) {
        case GK_POWERUP:  return powerup_playercollide(g, plr);
        case GK_MARINE:   return marine_playercollide(g, plr);
        default:          return true;    // Kill player.
    }
    return false;
}

void gobs_create(uint8_t kind, uint8_t n)
{
    while(n--) {
        uint8_t d = gob_alloc();
        if (d >= MAX_GOBS) {
            return;
        }
        switch (kind) {
            case GK_BLOCK: block_create(d); break;
            case GK_GRUNT: grunt_create(d); break;
            case GK_BAITER: baiter_create(d); break;
            case GK_AMOEBA_BIG:
            case GK_AMOEBA_MED:
            case GK_AMOEBA_SMALL:
                amoeba_create(d);
                break;
            case GK_TANK: tank_create(d); break;
            case GK_HZAPPER: hzapper_create(d); break;
            case GK_VZAPPER: vzapper_create(d); break;
            case GK_FRAGGER: fragger_create(d); break;
            case GK_VULGON: vulgon_create(d); break;
            case GK_HAPPYSLAPPER: happyslapper_create(d); break;
            case GK_MARINE: marine_create(d); break;
            case GK_BRAIN: brain_create(d); break;
            case GK_ZOMBIE: zombie_create(d); break;
            case GK_MISSILE: missile_create(d); break;
            case GK_BOSS: boss_create(d); break;
            // Not all kinds can be created. Some are spawned by others.
            case GK_BOSSTAIL:
            default:
                break;
        }
    }
}

// for all gobs:
// - cull any non-persistant gobs
// - set to new random position.
// - put into spawning mode.
void gobs_reset() {
    uint8_t g;
    bub_clear();
    for (g = 0; g < MAX_GOBS; ++g) {
        if (gobkind[g] == GK_NONE) {
            continue;
        }
        if (!(gobflags[g] & GF_PERSIST)) {
            gobkind[g] = GK_NONE;
            continue;
        }
        switch(gobkind[g]) {
            case GK_BLOCK:        block_reset(g); break;
            case GK_GRUNT:        grunt_reset(g); break;
            case GK_AMOEBA_SMALL: amoeba_reset(g); break;
            case GK_AMOEBA_MED:   amoeba_reset(g); break;
            case GK_AMOEBA_BIG:   amoeba_reset(g); break;
            case GK_TANK:         tank_reset(g); break;
            case GK_HZAPPER:      zapper_reset(g); break;
            case GK_VZAPPER:      zapper_reset(g); break;
            case GK_FRAGGER:      fragger_reset(g); break;
            case GK_VULGON:       vulgon_reset(g); break;
            case GK_HAPPYSLAPPER: happyslapper_reset(g); break;
            case GK_MARINE:       marine_reset(g); break;
            case GK_BRAIN:        brain_reset(g); break;
            case GK_ZOMBIE:       zombie_reset(g); break;
            case GK_MISSILE:      missile_reset(g); break;
            case GK_BOSS:         boss_reset(g); break;
            case GK_BOSSTAIL:     bosstail_reset(g); break;
            default:
                gob_randompos(g);
                break;
        }
    }
}


// Set all the active gobs into spawning-in mode.
void gobs_spawning() {
    uint8_t g;
    uint8_t t = 0;
    for (g = 0; g < MAX_GOBS; ++g) {
        if (gobkind[g] == GK_NONE) {
            continue;
        }
        gobflags[g] |= GF_SPAWNING;
        gobtimer[g] = t + 8;
        t += 2;
    }
}

// returns 0 if none free.
uint8_t gob_alloc() {
    uint8_t d;
    for(d = 0; d < MAX_GOBS; ++d) {
        if(gobkind[d] == GK_NONE) {
            return d;
        }
    }
    return MAX_GOBS;
}

void gob_randompos(uint8_t d) {
    const int16_t xmid = (SCREEN_W/2)-8;
    const int16_t ymid = (SCREEN_H/2)-8;

    int16_t x;
    int16_t y;
    while (1) {
        int16_t lsq; 
        x = -128 + rnd();
        y = -128 + rnd();

        lsq = x*x + y*y;
        if (lsq > 80*80 && lsq <100*100 ) {
            break;
        }
    }

    gobx[d] = (xmid + x) << FX;
    goby[d] = (ymid + y) << FX;
}

// duration must be <= 7 
void gob_highlight(uint8_t g, uint8_t duration)
{
    uint8_t f = gobflags[g];
    f = f & ~GF_HIGHLIGHT_MASK;
    f |= duration;
    gobflags[g] = f;
}

// move according to velocity, bouncing off walls.
void gob_move_bounce_x(uint8_t d)
{
    int16_t x = gobx[d];
    const int16_t xmax = (SCREEN_W - 16)<<FX;
    x += gobvx[d];
    if (x < 0) {
        x = -x;
        gobvx[d] = -gobvx[d];
    } else if (x >= xmax) {
        x = xmax - (x - xmax);
        gobvx[d] = -gobvx[d];
    }
    gobx[d] = x;
}

void gob_move_bounce_y(uint8_t d)
{
    int16_t y = goby[d];
    const int16_t ymax = (SCREEN_H - 16)<<FX;
    y += gobvy[d];
    if (y < 0) {
        y = -y;
        gobvy[d] = -gobvy[d];
    } else if (y >= ymax) {
        y = ymax - (y - ymax);
        gobvy[d] = -gobvy[d];
    }
    goby[d] = y;
}

// return the Manhattan distance between gob a and gob b.
int16_t gob_manhattan_dist(uint8_t a, uint8_t b)
{
    int16_t dx = gobx[a] - gobx[b];
    if (dx < 0) {
        dx = -dx;
    }
    int16_t dy = goby[a] - goby[b];
    if (dy < 0) {
        dy = -dy;
    }
    return dx + dy;
}


int16_t clip(int16_t val, int16_t minimum, int16_t maximum) {
    if (val < minimum) {
        val = minimum;
    } else if (val > maximum) {
        val =  maximum;
    }
    return val;
}

// accelerate toward target x
void gob_seek_x(uint8_t d, int16_t target, int16_t accel, int16_t maxspd)
{
    if (gobx[d] < target) {
        gobvx[d] = clip(gobvx[d] + accel, -maxspd, maxspd);
    } else if (gobx[d] > target) {
        gobvx[d] = clip(gobvx[d] - accel, -maxspd, maxspd);
    }
}

// accelerate toward target y
void gob_seek_y(uint8_t d, int16_t target, int16_t accel, int16_t maxspd)
{
    if (goby[d] < target) {
        gobvy[d] = clip(gobvy[d] + accel, -maxspd, maxspd);
    } else if (goby[d] > target) {
        gobvy[d] = clip(gobvy[d] - accel, -maxspd, maxspd);
    }
}


// no shot if 0xff
void gob_standard_kaboom(uint8_t d, uint8_t shot, uint8_t points)
{
    player_add_score(points);
    plat_addeffect(gobx[d]+(8<<FX), goby[d]+(8<<FX), EK_KABOOM);
    plat_sfx_play(SFX_KABOOM);
    gobkind[d] = GK_NONE;

    if (rnd() > 250) {
        // transform into powerup
        powerup_create(d, gobx[d], goby[d], 0);
    }
}


/*
 * block
 */
void block_create(uint8_t d)
{
    gobkind[d] = GK_BLOCK;
    gobflags[d] = GF_PERSIST | GF_COLLIDES_SHOT | GF_COLLIDES_PLAYER;
    block_reset(d);
}

void block_reset(uint8_t d)
{
    gob_randompos(d);
}

void block_shot(uint8_t d, uint8_t shot)
{
    gob_standard_kaboom(d, shot, 10);
}



/*
 * grunt
 */
void grunt_create(uint8_t g)
{
    gobkind[g] = GK_GRUNT;
    gobflags[g] = GF_PERSIST | GF_LOCKS_LEVEL | GF_COLLIDES_SHOT | GF_COLLIDES_PLAYER;
    gobdat[g] = 0;
    gobtimer[g] = 0;
    grunt_reset(g);
}

void grunt_reset(uint8_t g)
{
    gob_randompos(g);
    gobvx[g] = (2 << FX) + (rnd() & 0x7f);
    gobvy[g] = (2 << FX) + (rnd() & 0x7f);
}

void grunt_spawn(uint8_t g, int16_t x, int16_t y)
{
    gobkind[g] = GK_GRUNT;
    gobflags[g] = GF_PERSIST | GF_LOCKS_LEVEL | GF_COLLIDES_SHOT | GF_COLLIDES_PLAYER;
    gobdat[g] = 0;
    gobtimer[g] = 0;
    gobx[g] = x;
    goby[g] = y;
    gobvx[g] = (2 << FX) + (rnd() & 0x7f);
    gobvy[g] = (2 << FX) + (rnd() & 0x7f);
}


void grunt_tick(uint8_t g)
{
    int16_t px,py;
    // update every 16 frames
    if (((tick+g) & 0x0f) != 0x00) {
       return;
    }
    px = plrx[0];
    if (px < gobx[g]) {
        gobx[g] -= gobvx[g];
    } else if (px > gobx[g]) {
        gobx[g] += gobvx[g];
    }
    py = plry[0];
    if (py < goby[g]) {
        goby[g] -= gobvy[g];
    } else if (py > goby[g]) {
        goby[g] += gobvy[g];
    }
}

void grunt_shot(uint8_t g, uint8_t shot)
{
    gob_standard_kaboom(g, shot, 50);
}


/*
 * baiter
 */
void baiter_create(uint8_t d)
{
    gobkind[d] = GK_BAITER;
    gobflags[d] = GF_LOCKS_LEVEL | GF_COLLIDES_SHOT | GF_COLLIDES_PLAYER;
    gobx[d] = 0;
    goby[d] = 0;
    gobvx[d] = 0;
    gobvy[d] = 0;
    gobdat[d] = 0;
    gobtimer[d] = 0;
}

void baiter_tick(uint8_t d)
{
    const int16_t BAITER_MAX_SPD = 2 << FX;
    const int16_t BAITER_ACCEL = 3;

    gob_seek_x(d, plrx[0], BAITER_ACCEL, BAITER_MAX_SPD);
    gob_move_bounce_x(d);
    gob_seek_y(d, plry[0], BAITER_ACCEL, BAITER_MAX_SPD);
    gob_move_bounce_y(d);
}

void baiter_shot(uint8_t d, uint8_t shot)
{
    gob_standard_kaboom(d, shot, 75);
}


/*
 * amoeba
 */
void amoeba_create(uint8_t d)
{
    gobkind[d] = GK_AMOEBA_BIG;
    gobflags[d] = GF_PERSIST | GF_LOCKS_LEVEL | GF_COLLIDES_SHOT | GF_COLLIDES_PLAYER;
    gobdat[d] = 0;
    gobtimer[d] = 0;
    amoeba_reset(d);
}

void amoeba_reset(uint8_t d)
{
    gob_randompos(d);
    gobvx[d] = 0;
    gobvy[d] = 0;
}

void amoeba_tick(uint8_t d)
{
    const int16_t AMOEBA_MAX_SPD = (4<<FX)/3;
    const int16_t AMOEBA_ACCEL = 2 * FX_ONE / 2;
    int16_t px, py, vx, vy;
    // jitter acceleration
    gobvx[d] += (rnd() - 128)>>2;
    gobvy[d] += (rnd() - 128)>>2;


    // apply drag
    vx = gobvx[d];
    gobvx[d] = (vx >> 1) + (vx >> 2);
    vy = gobvy[d];
    gobvy[d] = (vy >> 1) + (vy >> 2);

    gobx[d] += gobvx[d];
    goby[d] += gobvy[d];

    // update homing every 16 frames
    if (((tick+d) & 0x0f) != 0x00) {
       return;
    }
    px = plrx[0];
    if (px < gobx[d] && gobvx[d] > -AMOEBA_MAX_SPD) {
        gobvx[d] -= AMOEBA_ACCEL;
    } else if (px > gobx[d] && gobvx[d] < AMOEBA_MAX_SPD) {
        gobvx[d] += AMOEBA_ACCEL;
    }

    py = plry[0];
    if (py < goby[d] && gobvy[d] > -AMOEBA_MAX_SPD) {
        gobvy[d] -= AMOEBA_ACCEL;
    } else if (py > goby[d] && gobvy[d] < AMOEBA_MAX_SPD) {
        gobvy[d] += AMOEBA_ACCEL;
    }
}

void amoeba_spawn(uint8_t kind, int16_t x, int16_t y, int16_t vx, int16_t vy) {
    uint8_t d = gob_alloc();
    if (d >= MAX_GOBS) {
        return;
    }
    gobkind[d] = kind;
    gobflags[d] = GF_PERSIST | GF_LOCKS_LEVEL | GF_COLLIDES_SHOT | GF_COLLIDES_PLAYER;
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
        gob_standard_kaboom(d, shot, 20);
        return;
    }

    if (gobkind[d] == GK_AMOEBA_MED) {
        int16_t x = gobx[d];
        int16_t y = goby[d];
        const int16_t v = FX<<5;
        gob_standard_kaboom(d, shot, 50);
        amoeba_spawn(GK_AMOEBA_SMALL, x, y, -v, v);
        amoeba_spawn(GK_AMOEBA_SMALL, x, y, v, v);
        amoeba_spawn(GK_AMOEBA_SMALL, x, y, 0, -v);
        return;
    }

    if (gobkind[d] == GK_AMOEBA_BIG) {
        int16_t x = gobx[d];
        int16_t y = goby[d];
        const int16_t v = FX<<5;
        gob_standard_kaboom(d, shot, 75);
        amoeba_spawn(GK_AMOEBA_MED, x, y, -v, v);
        amoeba_spawn(GK_AMOEBA_MED, x, y, v, v);
        amoeba_spawn(GK_AMOEBA_MED, x, y, 0, -v);
        return;
    }
}


// tank

void tank_create(uint8_t g)
{
    gobkind[g] = GK_TANK;
    gobflags[g] = GF_PERSIST | GF_LOCKS_LEVEL | GF_COLLIDES_SHOT | GF_COLLIDES_PLAYER;
    gobdat[g] = 12;  // life
    tank_reset(g);
}

void tank_reset(uint8_t g)
{
    gob_randompos(g);
    gobvx[g] = 0;
    gobvy[g] = 0;
    gobtimer[g] = 0;    // highlight timer
}


void tank_tick(uint8_t g)
{
    const int16_t vx = (5<<FX);
    const int16_t vy = (5<<FX);
    int16_t px = plrx[0];
    int16_t py = plry[0];

    if (gobtimer[g] > 0) {
        --gobtimer[g];
    }

    // shoot at regular intervals
    if(((tick + (g<<4)) & 0x3f) == 0 ) {
      uint8_t m = gob_alloc();
      if (m >= MAX_GOBS) {
        return;
      }
      int16_t x = gobx[g] + ((8-4)<<FX);
      int16_t y = goby[g] + ((8-4)<<FX);
      int16_t dx = plrx[0] - x;
      int16_t dy = plry[0] - y;
      uint8_t theta = arctan8(dx, dy);
      missile_spawn(m, x, y, theta);
    }

    // update every 32 frames
    if (((tick+g) & 0x1f) != 0x00) {
       return;
    }
    if (px < gobx[g]) {
        gobx[g] -= vx;
    } else if (px > gobx[g]) {
        gobx[g] += vx;
    }
    if (py < goby[g]) {
        goby[g] -= vy;
    } else if (py > goby[g]) {
        goby[g] += vy;
    }
}

void tank_shot(uint8_t g, uint8_t shot)
{
    --gobdat[g];
    if (gobdat[g]==0) {
        // boom.
        gob_standard_kaboom(g, shot, 100);
    } else {
        // knockback
        gobx[g] += shotvx[shot]/2;
        goby[g] += shotvy[shot]/2;
        gobtimer[g] = 8;
    }
}

/*
 * zapper
 */
void hzapper_create(uint8_t d)
{
    gobkind[d] = GK_HZAPPER;
    gobflags[d] = GF_PERSIST | GF_COLLIDES_SHOT | GF_COLLIDES_PLAYER;
    zapper_reset(d);
}

void vzapper_create(uint8_t d)
{
    gobkind[d] = GK_VZAPPER;
    gobflags[d] = GF_PERSIST | GF_COLLIDES_SHOT | GF_COLLIDES_PLAYER;
    zapper_reset(d);
}

// used for both h and v zappers
void zapper_reset(uint8_t d)
{
    uint8_t foo = rnd();
    gob_randompos(d);
    gobvx[d] = (1<<FX)/4;
    gobvy[d] = (1<<FX)/4;
    if (foo & 1) {
        gobvx[d] = -gobvx[d]; 
    }
    if (foo & 2) {
        gobvy[d] = -gobvy[d]; 
    }
    gobtimer[d] = 0;
}

// used for both h and v zappers
void zapper_tick(uint8_t d)
{
    gob_move_bounce_x(d);
    gob_move_bounce_y(d);
    // Firing state governed by timer (ticks/8)
    if(tick & 0x04) {
        gobtimer[d]++;
    }
}

// return zapper state based on timer
uint8_t zapper_state(uint8_t d)
{
    if (gobtimer[d] < 150) {
        return ZAPPER_OFF;
    }
    if (gobtimer[d] < 180) {
        return ZAPPER_WARMING_UP;
    }
    return ZAPPER_ON;
}

void zapper_shot(uint8_t d, uint8_t s)
{
    // knockback
    gobvx[d] += (shotvx[s] >> FX);
    gobvy[d] += (shotvy[s] >> FX);
}

/*
 * Fragger
 */

void fragger_create(uint8_t d)
{
    gobkind[d] = GK_FRAGGER;
    gobflags[d] = GF_PERSIST | GF_LOCKS_LEVEL | GF_COLLIDES_SHOT | GF_COLLIDES_PLAYER;
    fragger_reset(d);
}

void fragger_reset(uint8_t d)
{
    gob_randompos(d);
    gobvx[d] = 1<<FX;
    gobvy[d] = 1<<FX;
    gobdat[d] = 0;
    gobtimer[d] = 0;
}

void fragger_tick(uint8_t d)
{
    gob_move_bounce_x(d);
    gob_move_bounce_y(d);
}



void fragger_shot(uint8_t d, uint8_t shot)
{
    // boom.
    int16_t x = gobx[d];
    int16_t y = goby[d];
    uint8_t i;
    gob_standard_kaboom(d, shot, 100);
    for (i = 0; i < 4; ++i) {
        uint8_t f = gob_alloc();
        if (f >=MAX_GOBS) {
            return;
        }
        frag_spawn(f, x, y, i);
    }
}

/*
 * Frag
 */

static const int16_t frag_vx[4] = {-2 * FX_ONE, 2 * FX_ONE, -2 * FX_ONE, 2 * FX_ONE};
static const int16_t frag_vy[4] = {-2 * FX_ONE, -2 * FX_ONE, 2 * FX_ONE, 2 * FX_ONE};

void frag_spawn(uint8_t f, int16_t x, int16_t y, uint8_t dir)
{
    gobkind[f] = GK_FRAG;
    gobflags[f] = GF_LOCKS_LEVEL | GF_COLLIDES_SHOT | GF_COLLIDES_PLAYER;
    gobx[f] = x;
    goby[f] = y;
    gobdat[f] = dir;  // direction
    gobvx[f] = frag_vx[dir];
    gobvy[f] = frag_vy[dir];
    gobtimer[f] = 50;
}


void frag_tick(uint8_t d)
{
    gobx[d] += gobvx[d];
    goby[d] += gobvy[d];

    if (--gobtimer[d] == 0) {
        gobkind[d] = GK_NONE;
    }
}

void frag_shot(uint8_t d, uint8_t shot)
{
    // boom.
    gob_standard_kaboom(d, shot, 20);
}

/*
 * Powerup
 */

void powerup_create(uint8_t d, int16_t x, int16_t y, uint8_t kind)
{
    gobkind[d] = GK_POWERUP;
    gobflags[d] = GF_LOCKS_LEVEL | GF_COLLIDES_PLAYER;
    gobx[d] = x;
    goby[d] = y;
    gobvx[d] = (uint16_t)rnd() - 128;
    gobvy[d] = (uint16_t)rnd() - 128;
    gobdat[d] = kind;  // 0 = extra life
    gobtimer[d] = 0;
}

void powerup_tick(uint8_t d)
{
    gob_move_bounce_x(d);
    gob_move_bounce_y(d);
}

bool powerup_playercollide(uint8_t g, uint8_t plr)
{
    player_lives++;
    gobkind[g] = GK_NONE;
    return false;   // Don't kill player.
}


/*
 * Vulgon
 */

void vulgon_create(uint8_t d)
{
    gobkind[d] = GK_VULGON;
    gobflags[d] = GF_PERSIST | GF_LOCKS_LEVEL | GF_COLLIDES_SHOT | GF_COLLIDES_PLAYER;
    vulgon_reset(d);
}

void vulgon_reset(uint8_t d)
{
    gob_randompos(d);
    gobtimer[d] = 0;    // highlight timer
    gobdat[d] = 0;  // anger level
    gobvx[d] = 0;
    gobvy[d] = 0;
}

void vulgon_tick(uint8_t d)
{
    if(gobtimer[d]>0) {
        --gobtimer[d];  // decay highlight timer
    }
    gob_move_bounce_x(d);
    gob_move_bounce_y(d);
    // update accleration and anger every 16 frames
    if (((tick+d) & 0x0f) != 0x00) {
       return;
    }

    // check anger
    if (gobdat[d] == 0) {
        // not angry
        gob_seek_x(d, plrx[0], 8, 1<<FX);
        gob_seek_y(d, plry[0], 8, 1<<FX);
    } else {
        // angry! higher speed.
        gob_seek_x(d, plrx[0], 12, 4<<FX);
        gob_seek_y(d, plry[0], 12, 4<<FX);
        if (((tick+d) & 0x3f) == 0x00) {
            // shoot
            uint8_t f = gob_alloc();
            if (f >= MAX_GOBS) {
                return;
            }
            // always shoot poomerangs on diagonals
            int16_t vx = (gobvx[d] < 0) ? -POOMERANG_VEL : POOMERANG_VEL;
            int16_t vy = (gobvy[d] < 0) ? -POOMERANG_VEL : POOMERANG_VEL;
            poomerang_spawn(f, gobx[d], goby[d], vx, vy);
        }
    }
}

void vulgon_shot(uint8_t d, uint8_t shot)
{
    gobdat[d]++; // add to anger
    if (gobdat[d] >= 3) {
        // boom.
        gob_standard_kaboom(d, shot, 100);
    } else {
        // knockback
        gobvx[d] += (shotvx[shot] >> 4);
        gobvy[d] += (shotvy[shot] >> 4);
        gobtimer[d] = 4;
    }
}

/*
 * Poomerang
 */

void poomerang_spawn(uint8_t d, int16_t x, int16_t y, int16_t vx, int16_t vy)
{
    gobkind[d] = GK_POOMERANG;
    gobflags[d] = GF_LOCKS_LEVEL | GF_COLLIDES_SHOT | GF_COLLIDES_PLAYER;
    gobx[d] = x;
    goby[d] = y;
    gobvx[d] = vx;
    gobvy[d] = vy;
}

void poomerang_tick(uint8_t d)
{
    gob_move_bounce_x(d);
    gob_move_bounce_y(d);
}

void poomerang_shot(uint8_t d, uint8_t shot)
{
    gob_standard_kaboom(d, shot, 20);
}

/*
 * HappySlapper
 */

void happyslapper_create(uint8_t d)
{
    gobkind[d] = GK_HAPPYSLAPPER;
    gobflags[d] = GF_PERSIST | GF_LOCKS_LEVEL | GF_COLLIDES_SHOT | GF_COLLIDES_PLAYER;
    happyslapper_reset(d);
}

void happyslapper_reset(uint8_t d)
{
    gob_randompos(d);
    gobtimer[d] = 0;
    gobvx[d] = 0;
    gobvy[d] = 0;
    // reset time - add a little random variation to disperse them.
    gobdat[d] = HAPPYSLAPPER_SLEEP_TIME + HAPPYSLAPPER_RUN_TIME + (rnd() % 0x0f);
}

void happyslapper_tick(uint8_t d)
{
    ++gobtimer[d];
    if (gobtimer[d] < HAPPYSLAPPER_SLEEP_TIME) {
        return;
    }
    if (gobtimer[d] > gobdat[d]) {
        // Go back to sleep.
        gobtimer[d] = 0;
        gobvx[d] = 0;
        gobvy[d] = 0;
        return;
    }
    // Charge! (but only every 4 frames)
    if (((tick+d) & 0x03) != 0x00) {
       return;
    }
    // home in on player
    gob_seek_x(d, plrx[0], 1<<FX, 8<<FX);
    gob_seek_y(d, plry[0], 1<<FX, 8<<FX);
    gob_move_bounce_x(d);
    gob_move_bounce_y(d);
}

void happyslapper_shot(uint8_t d, uint8_t shot)
{
    gob_standard_kaboom(d, shot, 75);
}

/*
 * Heavily Armoured Space Marine
 */

void marine_create(uint8_t g)
{
    gobkind[g] = GK_MARINE;
    gobflags[g] = GF_PERSIST | GF_COLLIDES_PLAYER;  //| GF_LOCKS_LEVEL 
    gobdat[g] = 0;
    marine_reset(g);
}

static int16_t rndspd(int16_t s)
{
    if (rnd() >= 0x80) {
        return -s;
    } else {
        return s;
    }
}

// helper - return true if marine has been collected and is trailing after player.
static bool marine_is_trailing(uint8_t g)
{
    if (gobflags[g] & GF_COLLIDES_PLAYER) {
        return false;
    } else {
        return true;
    }
}

// turn marine into zombie
static void marine_zombify(uint8_t g)
{
    gobkind[g] = GK_ZOMBIE;
    gobflags[g] = GF_PERSIST | GF_LOCKS_LEVEL | GF_COLLIDES_SHOT | GF_COLLIDES_PLAYER;
}

void marine_reset(uint8_t g)
{
    gob_randompos(g);
    gobvx[g] = rndspd(1<<(FX/2));
    gobvy[g] = rndspd(1<<(FX/2));
}

void marine_tick(uint8_t g)
{
    if (marine_is_trailing(g)) {
        uint8_t p = 0;  // Player to follow.
        uint8_t idx = plrhistidx[p] - gobdat[g];
        gobx[g] = plrhistx[p][idx & (PLR_HIST_LEN-1)];
        goby[g] = plrhisty[p][idx & (PLR_HIST_LEN-1)];
    } else {
        // Random walking.
        if ((tick & 0x3f) == 0) {
            // New direction.
            gobvx[g] = rndspd(1<<(FX/2));
            gobvy[g] = rndspd(1<<(FX/2));
        }
        gob_move_bounce_x(g);
        gob_move_bounce_y(g);
    }
}

bool marine_playercollide(uint8_t g, uint8_t plr)
{
    gobflags[g] = 0;    // Turn off further collisions etc.
    // Add this marine to the end of the queue already following the player.
    uint8_t i;
    uint8_t idx = 0;
    for(i = 0; i < MAX_GOBS; ++i) {
        if (gobkind[i] != GK_MARINE) {
            continue;
        }
        if (gobdat[i] > idx) {
            idx = gobdat[i];
        }
    }
    gobdat[g] = idx + 3;
    return false;   // Don't kill player.
}
/*
 * Brain
 */

void brain_create(uint8_t d)
{
    gobkind[d] = GK_BRAIN;
    gobflags[d] = GF_PERSIST | GF_LOCKS_LEVEL | GF_COLLIDES_SHOT | GF_COLLIDES_PLAYER;
    brain_reset(d);
}

void brain_reset(uint8_t d)
{
    gob_randompos(d);
    gobtimer[d] = 0;
    gobvx[d] = 0;
    gobvy[d] = 0;
    // reset time - add a little random variation to disperse them.
    gobdat[d] = HAPPYSLAPPER_SLEEP_TIME + HAPPYSLAPPER_RUN_TIME + (rnd() % 0x0f);
}



void brain_tick(uint8_t g)
{
    int16_t tx;
    int16_t ty;


    // update every 8th frame
    if (((tick + g) & 0x07) != 0x00) {
        return;
    }

    int16_t bestd = INT16_MAX;
    uint8_t targ = MAX_GOBS;
    for (uint8_t i = 0; i < MAX_GOBS; ++i) {
        if (gobkind[i] == GK_MARINE && !marine_is_trailing(i)) {
            int16_t dist = gob_manhattan_dist(g, i);
            if (dist < bestd) {
                targ = i;
                bestd = dist;
            }
        }
    }
    if (targ == MAX_GOBS) {
        // No marines. Target player instead
        tx = plrx[0];
        ty = plry[0];
    } else {
        // Within zombification distance?
        if (bestd<(8<<FX)) {
            //gobkind[g] = GK_NONE;
            // convert marine to zombie
            marine_zombify(targ);
            plat_addeffect(gobx[g]+(8<<FX), goby[g]+(8<<FX), EK_ZOMBIFY);
            return;
        }

        tx = gobx[targ];
        ty = goby[targ];
    }  

    int16_t x = gobx[g];
    int16_t y = goby[g];
    if (x < tx) {
        x += 2<<FX;
    } else if (x > tx) {
        x -= 2<<FX;
    }
    if (y < ty) {
        y += 2<<FX;
    } else if (y > ty) {
        y -= 2<<FX;
    }
    gobx[g] = x;
    goby[g] = y;
}

void brain_shot(uint8_t d, uint8_t shot)
{
    gob_standard_kaboom(d, shot, 75);
}

/*
 * Zombie
 */

void zombie_create(uint8_t d)
{
    gobkind[d] = GK_ZOMBIE;
    gobflags[d] = GF_PERSIST | GF_LOCKS_LEVEL | GF_COLLIDES_SHOT | GF_COLLIDES_PLAYER;
    zombie_reset(d);
}

void zombie_reset(uint8_t d)
{
    gob_randompos(d);
    gobtimer[d] = 0;
    gobvx[d] = 0;
    gobvy[d] = 0;
}

void zombie_tick(uint8_t d)
{
    // Charge! (but only every 4 frames)
    if (((tick+d) & 0x03) != 0x00) {
       return;
    }
    // home in on player
    gob_seek_x(d, plrx[0], 2<<FX, 4<<FX);
    gob_seek_y(d, plry[0], 2<<FX, 4<<FX);
    gob_move_bounce_x(d);
    gob_move_bounce_y(d);
}

void zombie_shot(uint8_t d, uint8_t shot)
{
    gob_standard_kaboom(d, shot, 75);
}

/*
 * Missile
 *
 * gobdat: current heading (0..7)
 */

static int8_t missile_vx[8] = {
    0,3,4,3, 0,-3,-4,-3,
};
static int8_t missile_vy[8] = {
    -4,-3,0,3, 4,3,0,-3,
};
// helper
static void missile_setdir(uint8_t g, uint8_t dir) {
    gobdat[g] = dir;
    gobvx[g] = missile_vx[dir]<<4;
    gobvy[g] = missile_vy[dir]<<4;
}

void missile_spawn(uint8_t g, int16_t x, int16_t y, uint8_t direction)
{
    gobkind[g] = GK_MISSILE;
    gobflags[g] = GF_LOCKS_LEVEL | GF_COLLIDES_SHOT | GF_COLLIDES_PLAYER;
    gobx[g] = x;
    goby[g] = y;
    missile_setdir(g, direction);
}

void missile_create(uint8_t g)
{
    gobkind[g] = GK_MISSILE;
    gobflags[g] = GF_LOCKS_LEVEL | GF_COLLIDES_SHOT | GF_COLLIDES_PLAYER;
    missile_reset(g);
}

void missile_reset(uint8_t g)
{
    gob_randompos(g);
    gobtimer[g] = 0;
    missile_setdir(g, 0);
}

void missile_tick(uint8_t g)
{
    // home in on player (but not every frame)
    if (((tick+g) & 0x1f) == 0x00) {
        int16_t dx = plrx[0] - gobx[g];
        int16_t dy = plry[0] - goby[g];
        uint8_t theta = arctan8(dx, dy);
        int8_t delta = theta - gobdat[g];
        // find shortest route
        if (delta < -4) {
            delta += 8;
        } else if (delta > 4){
            delta -= 8;
        }
        // turn toward target
        if (delta > 0) {
            missile_setdir(g, (gobdat[g] + 1) & 7);
        } else if (delta < 0) {
            missile_setdir(g, (gobdat[g] - 1) & 7);
        }

        gobvx[g] = missile_vx[gobdat[g]]<<4;
        gobvy[g] = missile_vy[gobdat[g]]<<4;
    }

    gobx[g] += gobvx[g];
    goby[g] += gobvy[g];
}

void missile_shot(uint8_t g, uint8_t shot)
{
    gob_standard_kaboom(g, shot, 20);
}


/*
 * Boss
 *
 * gobdat: LLLLLLTT
 *         L = life (6 bits)
 *         T = target point (2bits)
 * gobtimer: time until next target
 */

void boss_create(uint8_t g)
{
    uint8_t i;
    gobkind[g] = GK_BOSS;
    gobflags[g] = GF_PERSIST | GF_COLLIDES_PLAYER | GF_COLLIDES_SHOT | GF_LOCKS_LEVEL;
    gobdat[g] = (5 << 2) | (rnd() & 3);
    gobtimer[g] = 0;
    boss_reset(g);
  
    uint8_t parent = g; 
    for (i = 0; i < 9; ++i) {
        uint8_t c = gob_alloc();
        if (c >= MAX_GOBS) {
            return;
        }
        bosstail_spawn(c, parent);
        parent = c;
    }
}

void boss_reset(uint8_t g)
{
    gob_randompos(g);
    gobvx[g] = 0;
    gobvy[g] = 0;
    gobtimer[g] = 0;
}

void boss_shot(uint8_t g, uint8_t shot)
{
    uint8_t life = gobdat[g] >> 2;
    if (life == 0) {
        // tell any children to go kaboom.
        bosstail_chainreact(g);
        gob_standard_kaboom(g, shot, 250);
        return;
    }
    --life;
    gobdat[g] = (life << 2) & (gobdat[g] & 0xFC);
    uint8_t child;
    // make the first child the new head.
    //for( child=0; child<MAX_GOBS; ++child) {
    //    if(gobkind[child] == GK_BOSSTAIL && gobdat[child] == g) {
    //        break;
    //    }
    //}
    gobvx[g] += shotvx[shot]<<2;
    gobvy[g] += shotvy[shot]<<2;
    gob_highlight(g, 4);
}


void boss_tick(uint8_t g)
{
    const int16_t WIBBLER_MAX_SPD = 2 << FX;
    const int16_t WIBBLER_ACCEL = 3;

    uint8_t targ = gobdat[g] & 0x03;
    if (++gobtimer[g] >= 230) {
        gobtimer[g] = 0;
        // next target
        targ = (targ + 1 ) & 0x03;
        gobdat[g] = (gobdat[g] & 0xfc) | targ;
    }
   
    const int16_t x0 = 0;
    const int16_t y0 = 0;
    const int16_t x1 = (SCREEN_W-32)<<FX;
    const int16_t y1 = (SCREEN_H-32)<<FX;
    int16_t targx;
    int16_t targy;
    switch (targ) {
        case 0: targx = x0; targy = y0; break;
        case 1: targx = x1; targy = y0; break;
        case 2: targx = x1; targy = y1; break;
        case 3: targx = x0; targy = y1; break;
    }

        /*
        if (((g+tick) & 0x31) == 0) {
            //gobvx[g] >>= 1;
            //gobvy[g] >>= 1;
            gobvx[g] += (rnd() - 128)>>1;
            gobvy[g] += (rnd() - 128)>>1;
        }
        */
    gob_seek_x(g, targx, (3*WIBBLER_ACCEL)/2, WIBBLER_MAX_SPD);
    gob_seek_y(g, targy, (3*WIBBLER_ACCEL)/2, WIBBLER_MAX_SPD);
    gob_move_bounce_x(g);
    gob_move_bounce_y(g);

    // shoot off a burst at regular intervals
    if(gobtimer[g] < 25 && ((tick & 0x7) == 0 )) {
      uint8_t m = gob_alloc();
      if (m >= MAX_GOBS) {
        return;
      }

      int16_t x = gobx[g] + (4<<FX);
      int16_t y = goby[g] + (4<<FX);
      grunt_spawn(m, x, y);
    //  int16_t dx = plrx[0] - x;
    //  int16_t dy = plry[0] - y;
    //  uint8_t theta = arctan8(dx, dy);
//      missile_spawn(m, x, y, theta);
    }

}


/*
 * Boss tail segment
 */


void bosstail_spawn(uint8_t g, uint8_t parent)
{
    gobkind[g] = GK_BOSSTAIL;
    gobflags[g] = GF_PERSIST | GF_COLLIDES_PLAYER | GF_COLLIDES_SHOT | GF_LOCKS_LEVEL; 
    gobdat[g] = parent;
    gobx[g] = gobx[parent];
    goby[g] = goby[parent];
    gobvx[g] = 0;
    gobvx[g] = 0;
    //boss_reset(i);
}

//
void bosstail_chainreact(uint8_t parent)
{
    uint8_t g;
    for (g=0; g<MAX_GOBS; ++g) {
        if (gobkind[g]== GK_BOSSTAIL && gobdat[g]==parent) {
            gobdat[g] = 0xff;
            gobtimer[g] = 8;
        }
    }
}

void bosstail_reset(uint8_t g)
{
    gobx[g] = gobx[gobdat[g]];
    goby[g] = goby[gobdat[g]];
    gobvx[g] = 0;
    gobvy[g] = 0;
}

void bosstail_shot(uint8_t g, uint8_t shot)
{
    gobvx[g] += shotvx[shot]<<2;
    gobvy[g] += shotvy[shot]<<2;
}

void bosstail_tick(uint8_t g)
{
    uint8_t parent = gobdat[g];
    if (parent == 0xff) {
        // parent gone - we're counting down to destruction.
        if (gobtimer[g] == 0) {
            bosstail_chainreact(g);
            gob_standard_kaboom(g, 0xff, 50);
        } else {
            --gobtimer[g];
        }
        return;
    }
    const int16_t WIBBLER_MAX_SPD = 2 << FX;
    const int16_t WIBBLER_ACCEL = 3;

    if ((tick & 0x3) == 0) {
        gobvx[g] >>= 1;
        gobvy[g] >>= 1;
    }
        gob_seek_x(g, gobx[parent], WIBBLER_ACCEL*6, WIBBLER_MAX_SPD*4);
        gob_seek_y(g, goby[parent], WIBBLER_ACCEL*6, WIBBLER_MAX_SPD*4);
    gob_move_bounce_x(g);
    gob_move_bounce_y(g);
}

