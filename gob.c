#include "gob.h"
#include "bub.h"
#include "misc.h"
#include "plat.h"
#include "player.h"
#include "sfx.h"
#include "vfx.h"

// gob tables
uint8_t gobkind[MAX_GOBS];
uint8_t gobflags[MAX_GOBS];
int16_t gobx[MAX_GOBS];
int16_t goby[MAX_GOBS];
int16_t gobvx[MAX_GOBS];   // SHOULD BE BYTES?
int16_t gobvy[MAX_GOBS];
uint8_t gobdat[MAX_GOBS];
uint8_t gobtimer[MAX_GOBS];

bool gobs_certainbonus; // next dude shot will yield a bonus
bool gobs_noextralivesforyou; // no more extra lives (after level 42)

// these are set by gobs_tick()
uint8_t gobs_lockcnt;   // num gobs holding level open.
uint8_t gobs_spawncnt;  // num gobs spawning.
uint8_t gobs_clearablecnt;  // num gobs remaining which are clearable.
uint8_t gobs_num_marines;  // num of unrescued marines
uint8_t gobs_num_marines_trailing;  // num of marines currently trailing player

// score stuff - use enums to shortcut formatting etc...
#define SCORE_10 0
#define SCORE_20 1
#define SCORE_50 2
#define SCORE_75 3
#define SCORE_100 4
#define SCORE_150 5
#define SCORE_200 6
#define SCORE_250 7
#define SCORE_NUM 8 // the number of score values we use.

static uint8_t score_vals[SCORE_NUM] = {10,20,50,75,100,150,200,250};
static const char* score_strings[SCORE_NUM] = {"10","20","50","75","100","150","200","250"};

void gobs_clear()
{
    uint8_t i;
    for (i = 0; i < MAX_GOBS; ++i) {
        gobkind[i] = GK_NONE;
    }
    gobs_lockcnt = 0;
    gobs_spawncnt = 0;
    gobs_clearablecnt = 0;
    gobs_num_marines = 0;
    gobs_num_marines_trailing = 0;
    bub_clear();
}

void gobs_tick(bool spawnphase)
{
    uint8_t i;
    gobs_lockcnt = 0;
    gobs_spawncnt = 0;
    gobs_clearablecnt = 0;
    gobs_num_marines = 0;
    gobs_num_marines_trailing = 0;
    for (i = 0; i < MAX_GOBS; ++i) {
        if (gobkind[i] == GK_NONE) {
            continue;
        }
        // Is gob playing its spawning-in effect?
        if (gobflags[i] & GF_SPAWNING) {
            ++gobs_spawncnt;
            if( gobtimer[i] == 8) {
                // don't play spawn effect for boss segments
                if (gobkind[i] != GK_BOSSSEG) {
                    vfx_play_spawn(gobx[i] + (8<<FX), goby[i] + (8<<FX));
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
        if (gob_is_clearable(i)) {
            ++gobs_clearablecnt;
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
            case GK_PICKUP: pickup_tick(i); break;
            case GK_VULGON:  vulgon_tick(i); break;
            case GK_POOMERANG: poomerang_tick(i); break;
            case GK_HAPPYSLAPPER:  happyslapper_tick(i); break;
            case GK_MARINE: marine_tick(i); break;
            case GK_BRAIN:  brain_tick(i); break;
            case GK_ZOMBIE:  zombie_tick(i); break;
            case GK_RIFASHARK:  rifashark_tick(i); break;
            case GK_RIFASPAWNER:  rifaspawner_tick(i); break;
            case GK_GOBBER:  gobber_tick(i); break;
            case GK_MISSILE:  missile_tick(i); break;
            case GK_BOSS:  boss_tick(i); break;
            case GK_BOSSSEG:  bossseg_tick(i); break;
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
                plat_tank_render(gobx[d], goby[d], gobflags[d] & GF_HIGHLIGHT_MASK);
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
            case GK_PICKUP:
                plat_pickup_render(gobx[d], goby[d], gobdat[d]);
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
                plat_brain_render(gobx[d], goby[d], gobflags[d] & GF_HIGHLIGHT_MASK);
                break;
            case GK_ZOMBIE:
                plat_zombie_render(gobx[d], goby[d]);
                break;
            case GK_RIFASHARK:
                plat_rifashark_render(gobx[d], goby[d], gobdat[d]);
                break;
            case GK_RIFASPAWNER:
                plat_rifaspawner_render(gobx[d], goby[d]);
                break;
            case GK_GOBBER:
                plat_gobber_render(gobx[d], goby[d], gobtimer[d], gobflags[d] & GF_HIGHLIGHT_MASK);
                break;
            case GK_MISSILE:
                plat_missile_render(gobx[d], goby[d], gobdat[d]);
                break;
            case GK_BOSS:
                plat_boss_render(gobx[d], goby[d], gobflags[d] & GF_HIGHLIGHT_MASK);
                break;
            case GK_BOSSSEG:
                if ((gobdat[d] & 0xf0) != SEGSTATE_HIDE) {
                    plat_bossseg_render(gobx[d], goby[d], d, gobtimer[d]!=0, gobflags[d] & GF_HIGHLIGHT_MASK);
                }
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
        case GK_RIFASHARK:    rifashark_shot(d, s); break;
        case GK_RIFASPAWNER:  rifaspawner_shot(d, s); break;
        case GK_GOBBER:       gobber_shot(d, s); break;
        case GK_MISSILE:      missile_shot(d, s); break;
        case GK_BOSS:         boss_shot(d, s); break;
        case GK_BOSSSEG:      bossseg_shot(d, s); break;
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
        case GK_PICKUP:  return pickup_playercollide(g, plr);
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
            case GK_RIFASHARK: rifashark_create(d); break;
            case GK_RIFASPAWNER: rifaspawner_create(d); break;
            case GK_GOBBER: gobber_create(d); break;
            case GK_MISSILE: missile_create(d); break;
            case GK_BOSS: boss_create(d); break;
            // Not all kinds can be created. Some are spawned by others.
            case GK_BOSSSEG:
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
            case GK_BAITER:       baiter_reset(g); break;
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
            case GK_RIFASHARK:    rifashark_reset(g); break;
            case GK_RIFASPAWNER:  rifaspawner_reset(g); break;
            case GK_GOBBER:       gobber_reset(g); break;
            case GK_MISSILE:      missile_reset(g); break;
            case GK_BOSS:         boss_reset(g); break;
            case GK_BOSSSEG:      bossseg_reset(g); break;
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

#if 0
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
#endif

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
        if (lsq > 80*80 &&
                x > -xmid && x < (xmid - 16) &&
                y > -ymid && y < (ymid - 16)) {
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

// generic gob_shot handler with knockback.
// assumes gobdat holds life.
void gob_knockback_shot(uint8_t g, uint8_t shot, uint8_t scoreenum)
{
    uint8_t power = shotpower[shot] + 1;
    if (gobdat[g] <= power ) {
        // boom.
        gob_standard_kaboom(g, shot, scoreenum);
    } else {
        gobdat[g] -= power;
        // knockback
        gobx[g] += shotvx[shot]/2;
        goby[g] += shotvy[shot]/2;
        gob_highlight(g,7);
        sfx_play(SFX_HIT, 1);
    }
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


// score is a SCORE_n enum
void gob_standard_kaboom(uint8_t d, uint8_t shot, uint8_t score)
{
    player_add_score(score_vals[score]);
    vfx_play_kaboom(gobx[d] + (8<<FX), goby[d] + (8<<FX));
    vfx_play_quicktext(gobx[d], goby[d]+(4<<FX), score_strings[score]);
    sfx_play(SFX_KABOOM, 1);
    gobkind[d] = GK_NONE;

    if (rnd() > 253 || gobs_certainbonus) {
        // transform into pickup
        // 0 = extra life
        // 1 = powerup
        // 2 = weapon upgrade
        uint8_t kind = rnd() & 0x03;
        if (kind == 3) {    // 3 is invalid
            kind = 1;
        }
        if (gobs_noextralivesforyou && kind==0) {
            kind = 2;
        }
        pickup_create(d, gobx[d], goby[d], kind);
        gobs_certainbonus = false;
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
    gob_standard_kaboom(d, shot, SCORE_10);
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
    gob_standard_kaboom(g, shot, SCORE_50);
}


/*
 * baiter
 */
void baiter_create(uint8_t g)
{
    gobkind[g] = GK_BAITER;
    gobflags[g] = GF_PERSIST | GF_LOCKS_LEVEL | GF_COLLIDES_SHOT | GF_COLLIDES_PLAYER;
    gobvx[g] = 0;
    gobvy[g] = 0;
    baiter_reset(g);
}

void baiter_reset(uint8_t g)
{ 
    gob_randompos(g);
    gobvx[g] = 0;
    gobvy[g] = 0;
}

void baiter_tick(uint8_t g)
{
    const int16_t BAITER_MAX_SPD = 2 << FX;
    const int16_t BAITER_ACCEL = 3;

    gob_seek_x(g, plrx[0], BAITER_ACCEL, BAITER_MAX_SPD);
    gob_move_bounce_x(g);
    gob_seek_y(g, plry[0], BAITER_ACCEL, BAITER_MAX_SPD);
    gob_move_bounce_y(g);
}

void baiter_shot(uint8_t g, uint8_t shot)
{
    gob_standard_kaboom(g, shot, SCORE_75);
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
        gob_standard_kaboom(d, shot, SCORE_20);
        return;
    }

    if (gobkind[d] == GK_AMOEBA_MED) {
        int16_t x = gobx[d];
        int16_t y = goby[d];
        const int16_t v = FX<<5;
        gob_standard_kaboom(d, shot, SCORE_50);
        amoeba_spawn(GK_AMOEBA_SMALL, x, y, -v, v);
        amoeba_spawn(GK_AMOEBA_SMALL, x, y, v, v);
        amoeba_spawn(GK_AMOEBA_SMALL, x, y, 0, -v);
        return;
    }

    if (gobkind[d] == GK_AMOEBA_BIG) {
        int16_t x = gobx[d];
        int16_t y = goby[d];
        const int16_t v = FX<<5;
        gob_standard_kaboom(d, shot, SCORE_75);
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
    gobdat[g] = 10;  // life
    tank_reset(g);
}

void tank_reset(uint8_t g)
{
    gob_randompos(g);
    gobvx[g] = 0;
    gobvy[g] = 0;

    gobtimer[g] = g*4;  // stagger firing
}


void tank_tick(uint8_t g)
{
    const int16_t vx = (5<<FX);
    const int16_t vy = (5<<FX);
    int16_t px = plrx[0];
    int16_t py = plry[0];

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
    gob_knockback_shot(g, shot, SCORE_150);
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
void zapper_tick(uint8_t g)
{
    gob_move_bounce_x(g);
    gob_move_bounce_y(g);
    // Firing state governed by timer (ticks/8)
    if(tick & 0x04) {
        gobtimer[g]++;
    }

    // Make sure appropriate sound effect is playing.
    // TODO: zappers are all in sync, so should share state/timer. But hey.
    switch (zapper_state(g)) {
        case ZAPPER_WARMING_UP:
            sfx_continuous = SFX_ZAPPER_CHARGE;
            break;
        case ZAPPER_ON:
            sfx_continuous = SFX_ZAPPING;
            break;
        default:
            break;
    }
}

// return zapper state based on timer
uint8_t zapper_state(uint8_t g)
{
    if (gobtimer[g] < 150) {
        return ZAPPER_OFF;
    }
    if (gobtimer[g] < 180) {
        return ZAPPER_WARMING_UP;
    }
    return ZAPPER_ON;
}

void zapper_shot(uint8_t d, uint8_t s)
{
    // knockback
    gobvx[d] += (shotvx[s] >> FX);
    gobvy[d] += (shotvy[s] >> FX);
    sfx_play(SFX_INEFFECTIVE_THUD, 1);
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
    gob_standard_kaboom(d, shot, SCORE_100);

    // Bigger shots don't cause fragmentation.
    uint8_t power = shotpower[shot] + 1;
    if (power <= 1) {
        int16_t x = gobx[d];
        int16_t y = goby[d];
        uint8_t i;
        for (i = 0; i < 4; ++i) {
            uint8_t f = gob_alloc();
            if (f >=MAX_GOBS) {
                return;
            }
            frag_spawn(f, x, y, i);
        }
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
    gob_standard_kaboom(d, shot, SCORE_20);
}

/*
 * Powerup
 */

void pickup_create(uint8_t g, int16_t x, int16_t y, uint8_t kind)
{
    gobkind[g] = GK_PICKUP;
    gobflags[g] = GF_LOCKS_LEVEL | GF_COLLIDES_PLAYER;
    gobx[g] = x;
    goby[g] = y;
    gobvx[g] = ((int16_t)rnd() - 128)/2;
    gobvy[g] = ((int16_t)rnd() - 128)/2;
    gobdat[g] = kind;  // 0 = extra life
    gobtimer[g] = 20;
}

void pickup_tick(uint8_t g)
{
    if(((tick+g)&0x0f) == 0) {
        gob_seek_x(g, plrx[0], FX_ONE/16, FX_ONE/2);
        gob_seek_y(g, plry[0], FX_ONE/16, FX_ONE/2);
        // slow timeout
        if (gobtimer[g]==0) {
            gobkind[g] = GK_NONE;
        } else {
            --gobtimer[g];
        }
    }
    gob_move_bounce_x(g);
    gob_move_bounce_y(g);
}

bool pickup_playercollide(uint8_t g, uint8_t plr)
{
    sfx_play(SFX_BONUS, 1);
    switch(gobdat[g]) {
        case 0: player_extra_life(plr); break;
        case 1: player_powerup(plr); break;
        case 2: player_next_weapon(plr); break;
        default: break;
    }

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
        gob_standard_kaboom(d, shot, SCORE_100);
    } else {
        // knockback
        gobvx[d] += (shotvx[shot] >> 4);
        gobvy[d] += (shotvy[shot] >> 4);
        gobtimer[d] = 4;
        sfx_play(SFX_HIT, 1);
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
    gob_standard_kaboom(d, shot, SCORE_20);
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
    gob_standard_kaboom(d, shot, SCORE_75);
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
        ++gobs_num_marines_trailing;
        uint8_t p = 0;  // Player to follow.
        uint8_t idx = plrhistidx[p] - gobdat[g];
        gobx[g] = plrhistx[p][idx & (PLR_HIST_LEN-1)];
        goby[g] = plrhisty[p][idx & (PLR_HIST_LEN-1)];
    } else {
        ++gobs_num_marines;
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

    // give points
    player_add_score(score_vals[SCORE_100]);
    vfx_play_quicktext(gobx[g], goby[g] + (4<<FX), score_strings[SCORE_100]);
    sfx_play(SFX_MARINE_SAVED, 1);
    player_extend_powerup_time(plr);
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
    gobdat[d] = 8;  // life
}

void brain_reset(uint8_t d)
{
    gob_randompos(d);
    gobtimer[d] = 0;
    gobvx[d] = 0;
    gobvy[d] = 0;
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
            vfx_play_zombify(gobx[g]+(8<<FX), goby[g]+(8<<FX));
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

void brain_shot(uint8_t g, uint8_t shot)
{
    gob_knockback_shot(g, shot, SCORE_150);
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
    gob_standard_kaboom(d, shot, SCORE_75);
}

/*
 * Rifashark
 *
 * gobdat: current heading (0..7)
 */

static int8_t rifashark_vx[8] = {
    0,3,4,3, 0,-3,-4,-3,
};
static int8_t rifashark_vy[8] = {
    -4,-3,0,3, 4,3,0,-3,
};
// helper
static void rifashark_setdir(uint8_t g, uint8_t dir) {
    gobdat[g] = dir;
    gobvx[g] = rifashark_vx[dir]<<4;
    gobvy[g] = rifashark_vy[dir]<<4;
}

void rifashark_spawn(uint8_t g, int16_t x, int16_t y, uint8_t direction)
{
    gobkind[g] = GK_RIFASHARK;
    gobflags[g] = GF_PERSIST | GF_LOCKS_LEVEL | GF_COLLIDES_SHOT | GF_COLLIDES_PLAYER;
    gobx[g] = x;
    goby[g] = y;
    gobtimer[g] = 0;
    rifashark_setdir(g, direction);
}

void rifashark_create(uint8_t g)
{
    gobkind[g] = GK_RIFASHARK;
    gobflags[g] = GF_PERSIST | GF_LOCKS_LEVEL | GF_COLLIDES_SHOT | GF_COLLIDES_PLAYER;
    rifashark_reset(g);
}

void rifashark_reset(uint8_t g)
{
    gob_randompos(g);
    gobtimer[g] = 0;
    rifashark_setdir(g, rnd() & 0x07);
}

void rifashark_tick(uint8_t g)
{
    // home in on player (but not every frame)
    if (((tick+g) & 0x1f) == 0x00) {
        int16_t dx = plrx[0] - gobx[g];
        int16_t dy = plry[0] - goby[g];
        uint8_t theta = arctan8(dx, dy);
        uint8_t newdir = turntoward8(gobdat[g], theta);
        rifashark_setdir(g, newdir );
    }

    gobx[g] += gobvx[g];
    goby[g] += gobvy[g];
}

void rifashark_shot(uint8_t g, uint8_t shot)
{
    gob_standard_kaboom(g, shot, SCORE_100);
}


/*
 * Rifaspawner
 *
 * gobdat: current heading (0..7)
 */

// helper
static void rifaspawner_newdir(uint8_t g) {
    int16_t vx = (rnd() - 128)/4;
    int16_t vy = (rnd() - 128)/4;


    const int16_t minspd = 32;
    if (vx < 0 && vx > -minspd) {
        vx = -minspd;
    } else if (vx >= 0 && vx < minspd) {
        vx = minspd;
    }
    if (vy < 0 && vy > -minspd) {
        vy = -minspd;
    } else if (vy >= 0 && vy < minspd) {
        vy = minspd;
    }

    gobvx[g] = vx;
    gobvy[g] = vy;
    gobtimer[g] = 128 + (rnd()/2);
}

void rifaspawner_create(uint8_t g)
{
    gobkind[g] = GK_RIFASPAWNER;
    gobflags[g] = GF_PERSIST | GF_LOCKS_LEVEL | GF_COLLIDES_SHOT | GF_COLLIDES_PLAYER;
    rifaspawner_reset(g);
}

void rifaspawner_reset(uint8_t g)
{
    rifaspawner_newdir(g);
    gob_randompos(g);
}

void rifaspawner_tick(uint8_t g)
{
    // shoot at regular intervals
    uint8_t f = tick + (g<<6);
    if(f  == 0) {
        int16_t x = gobx[g] + ((8-4)<<FX);
        int16_t y = goby[g] + ((8-4)<<FX);
        for (uint8_t theta = 0; theta < 8; theta += 2) {
            uint8_t m = gob_alloc();
            if (m >= MAX_GOBS) {
                break;
            }
            rifashark_spawn(m, x, y, theta);
        }
    }

    // slowly run away from player
    if(((tick+g)&0x15) ==0) {
        gob_seek_x(g, plrx[0], -((1<<FX)>>2), 2<<FX);
        gob_seek_y(g, plry[0], -((1<<FX)>>2), 2<<FX);
    }
    gob_move_bounce_x(g);
    gob_move_bounce_y(g);
}

void rifaspawner_shot(uint8_t g, uint8_t shot)
{
    gob_standard_kaboom(g, shot, SCORE_200);
}

/*
 * Gobber
 * gobtimer: dir (0..7)
 * gobdat: life
 *
 */

void gobber_create(uint8_t g)
{
    gobkind[g] = GK_GOBBER;
    gobflags[g] = GF_PERSIST | GF_LOCKS_LEVEL | GF_COLLIDES_SHOT | GF_COLLIDES_PLAYER;
    gobdat[g] = 12;   // life
    gobber_reset(g);
}

void gobber_reset(uint8_t g)
{
    gob_randompos(g);
    gobvx[g] = 0;
    gobvy[g] = 0;
    gobtimer[g] = rnd() & 0x07;     //dir
}


void gobber_tick(uint8_t g)
{
    if (((tick + g) & 0x0f) == 0) {
        // turn to player
        int16_t dx = plrx[0] - gobx[g];
        int16_t dy = plry[0] - goby[g];
        uint8_t dir8 = arctan8(dx, dy);

        gobtimer[g] = turntoward8(gobtimer[g], dir8);
      
        // accelerate toward player if facing
        gob_seek_x(g, plrx[0], (1<<FX)/16, (1<<FX)/8);
        gob_seek_y(g, plry[0], (1<<FX)/16, (1<<FX)/8);
    }

    if (((tick + g) & 0x1f) == 0) {
        // FIRE!
        uint8_t m = gob_alloc();
        if (m < MAX_GOBS) {
            uint8_t dir8 = gobtimer[g];
            missile_spawn(m, gobx[g] + (2<<FX), goby[g] + (2<<FX), dir8);
        }
    }

    gob_move_bounce_x(g);
    gob_move_bounce_y(g);
}

void gobber_shot(uint8_t g, uint8_t shot)
{
    gob_knockback_shot(g, shot, SCORE_150);
}

/*
 * Missile
 * gobdat: dir (0..7)
 * gobtimer: time remaining
 *
 */

static const int8_t missile_vx[8] = {
    0,4,6,4, 0,-4,-6,-4,
};
static const int8_t missile_vy[8] = {
    -6,-4,0,4, 6,4,0,-4,
};
static void missile_setdir(uint8_t g, uint8_t dir8) {
    gobdat[g] = dir8;
    gobvx[g] = missile_vx[dir8]<<4;
    gobvy[g] = missile_vy[dir8]<<4;
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
    gobtimer[g] = 64;
    missile_setdir(g, rnd() & 0x07);
}

void missile_spawn(uint8_t g, int16_t x, int16_t y, uint8_t dir8)
{
    gobkind[g] = GK_MISSILE;
    gobflags[g] = GF_LOCKS_LEVEL | GF_COLLIDES_SHOT | GF_COLLIDES_PLAYER;
    gobx[g] = x;
    goby[g] = y;
    gobtimer[g] = 128;
    missile_setdir(g, dir8);
}

void missile_tick(uint8_t g)
{
    if (gobtimer[g] == 0)
    {
        gobkind[g] = GK_NONE;
        return;
    }
    --gobtimer[g];
    gobx[g] += gobvx[g];
    goby[g] += gobvy[g];
}

void missile_shot(uint8_t g, uint8_t shot)
{
    gob_standard_kaboom(g, shot, SCORE_20);
}

/*
 * Boss
 *
 * gobdat: LLLLLLTT
 *         L = life (6 bits)
 *         T = target point (2bits)
 * gobtimer: time until next target
 */

// NOTE: Boss is incomplete and stubbed out.
// We ran out of memory - hit the cx16 38KB BASIC RAM limit.
// Maybe revisit having a proper boss if later versions of llvm-mos
// produce smaller code...

void boss_create(uint8_t g) {}
void boss_reset(uint8_t g) {}
void boss_tick(uint8_t g) {}
void boss_shot(uint8_t g, uint8_t shot) {}

void bossseg_spawn(uint8_t g, uint8_t slot) {}
void bossseg_reset(uint8_t g) {}
void bossseg_tick(uint8_t g) {}
void bossseg_shot(uint8_t g, uint8_t shot) {}

#if 0
#define NUM_BOSSSEGS 12
#define NUM_BOSS_FORMATIONS 3

static const int8_t boss_formx[NUM_BOSS_FORMATIONS][NUM_BOSSSEGS] = {
    {-6,-5,-4,-3,-2,-1,2,3,4,5,6,7},
    {0,0,0,0,0,0, 0,0,0,0,0,0},
    {-1,-1,-1,-1,-1, 0,1, 2,2,2,2,2},
};

static const int8_t boss_formy[NUM_BOSS_FORMATIONS][NUM_BOSSSEGS] = {
    {0,0,0,0,0,0, 0,0,0,0,0,0},
    {-6,-5,-4,-3,-2,-1,2,3,4,5,6,7},
    {-1,0,1,2,3, -1,-1, -1,0,1,2,3},
};

static uint8_t boss;
static uint8_t boss_formation;

void boss_create(uint8_t g)
{
    boss = g;
    boss_formation = 0;

    const uint8_t initiallife = 63;
    uint8_t i;
    gobkind[g] = GK_BOSS;
    gobflags[g] = GF_PERSIST | GF_COLLIDES_PLAYER | GF_COLLIDES_SHOT | GF_LOCKS_LEVEL;
    gobdat[g] = (initiallife << 2) | (rnd() & 3);
    gobtimer[g] = 0;
    boss_reset(g);
  
    for (i = 0; i < NUM_BOSSSEGS; ++i) {
        uint8_t c = gob_alloc();
        if (c >= MAX_GOBS) {
            return;
        }
        bossseg_spawn(c, i);
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
        uint8_t delay = 4;
        for (uint8_t child = 0; child < MAX_GOBS; ++child) {
            bossseg_destruct(child, delay);
            delay += 4;
        }
        gob_standard_kaboom(g, shot, SCORE_250);
        return;
    }
    --life;
    gobdat[g] = (life << 2) & (gobdat[g] & 0xFC);

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
        ++boss_formation;
        if(boss_formation >= NUM_BOSS_FORMATIONS) {
            boss_formation = 0;
        }
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
//      rifashark_spawn(m, x, y, theta);
    }

}


/*
 * Boss segment
 *
 * gobdat: ssssffff
 *         s=state SEGSTATE_*
 *         f=formation slot (0 to NUM_BOSSSEGS-1)
 * gobvx,gobvy: current position, relative to boss head
 * gobtimer: 0=moving, 1=at rest
 *           or countdown timer, if gobdat==0xff
 */


// return x for current formation, relative to boss head
static inline int16_t bossseg_x(uint8_t g)
{
    uint8_t slot = gobdat[g] & 0x0F;
    int16_t x = (boss_formx[boss_formation][slot] * 16)<<FX;
    return x;
}

static inline int16_t bossseg_y(uint8_t g)
{
    uint8_t slot = gobdat[g] & 0x0F;
    int16_t y = (boss_formy[boss_formation][slot] * 16)<<FX;
    return y;
}



void bossseg_spawn(uint8_t g, uint8_t slot)
{
    gobkind[g] = GK_BOSSSEG;
    gobflags[g] = GF_PERSIST | GF_COLLIDES_PLAYER | GF_COLLIDES_SHOT | GF_LOCKS_LEVEL; 
    gobdat[g] = SEGSTATE_NORMAL | slot;
    bossseg_reset(g);
}

//
void bossseg_destruct(uint8_t g, uint8_t delay)
{
    if (bossseg_state(g) != SEGSTATE_NORMAL) {
        // If already hidden, just drop out without fanfare.
        gobkind[g] = GK_NONE;
        return;
    }
    gobdat[g] = SEGSTATE_DESTRUCT | (gobdat[g] & 0x0f);
    gobtimer[g] = delay;
}

void bossseg_reset(uint8_t g)
{
    gobvx[g] = bossseg_x(g);
    gobvy[g] = bossseg_y(g);
    gobdat[g] = SEGSTATE_NORMAL | (gobdat[g] & 0x0f);
    gobtimer[g] = 8;
    
    gobx[g] = gobx[boss] + gobvx[g];
    goby[g] = goby[boss] + gobvy[g];
}

void bossseg_shot(uint8_t g, uint8_t shot)
{
#if 0
    if (bossseg_state(g) == SEGSTATE_NORMAL) {
        if (gobtimer[g] == 0) {
            // Hide and turn off collisions.
            gobdat[g] = SEGSTATE_HIDE | (gobdat[g] & 0x0f);
            gobflags[g] = gobflags[g] & ~(GF_COLLIDES_SHOT | GF_COLLIDES_PLAYER);
            gobtimer[g] = 64;
        } else {
            --gobtimer[g];
        }
    }
#endif
}

void bossseg_tick(uint8_t g)
{
    //uint8_t slot = gobdat[g] & 0x0f;
    if (bossseg_state(g) == SEGSTATE_DESTRUCT) {
        // parent gone - we're counting down to destruction.
        if (gobtimer[g] == 0) {
            gob_standard_kaboom(g, 0xff, SCORE_50);   // 0xff=no shot
        } else {
            --gobtimer[g];
        }
        return;
    }

    if (bossseg_state(g) == SEGSTATE_HIDE) {
        if (gobtimer[g] == 0) {
            // Show and turn on collisions.
            gobdat[g] = SEGSTATE_NORMAL | (gobdat[g] & 0x0f);
            gobflags[g] = gobflags[g] | GF_COLLIDES_SHOT | GF_COLLIDES_PLAYER;
            gobtimer[g] = 8;
        } else {
            --gobtimer[g];
        }
        return;
    }

    int16_t targx = bossseg_x(g);
    int16_t dx = (targx - gobvx[g])/8;
    gobvx[g] += dx;

    int16_t targy = bossseg_y(g);
    int16_t dy = (targy - gobvy[g])/8;
    gobvy[g] += dy;
    if ((dx/8) == 0 && (dy/8) == 0) {
        gobtimer[g] = 1;    // at rest
    } else {
        gobtimer[g] = 0;
    }
    // position in boss-space
    gobx[g] = gobx[boss] + gobvx[g];
    goby[g] = goby[boss] + gobvy[g];
}
#endif

