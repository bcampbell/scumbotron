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
uint8_t gobs_lockcnt;   // num gobs holding level open.
uint8_t gobs_spawncnt;  // num gobs spawning.


// by darsie,
// https://www.avrfreaks.net/forum/tiny-fast-prng
uint8_t rnd() {
        static uint8_t s=0xaa,a=0;
        s^=s<<3;
        s^=s>>5;
        s^=a++>>2;
        return s;
}

void gobs_clear()
{
    uint8_t i;
    for (i = 0; i < MAX_GOBS; ++i) {
        gobkind[i] = GK_NONE;
    }
    gobs_lockcnt = 0;
    gobs_spawncnt = 0;
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
                sys_addeffect(gobx[i]+(8<<FX), goby[i]+(8<<FX), EK_SPAWN);
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
            default:
                break;
        }
    }
}

void gobs_render()
{
    uint8_t d;
    for (d = 0; d < MAX_GOBS; ++d) {
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
            case GK_HZAPPER:
                sys_hzapper_render(gobx[d], goby[d], zapper_state(d));
                break;
            case GK_VZAPPER:
                sys_vzapper_render(gobx[d], goby[d], zapper_state(d));
                break;
            case GK_FRAGGER:
                sys_fragger_render(gobx[d], goby[d]);
                break;
            case GK_FRAG:
                sys_frag_render(gobx[d], goby[d], gobdat[d]);
                break;
            case GK_POWERUP:
                sys_powerup_render(gobx[d], goby[d], gobdat[d]);
                break;
            case GK_VULGON:
                sys_vulgon_render(gobx[d], goby[d], gobtimer[d] > 0, gobdat[d]);
                break;
            case GK_POOMERANG:
                sys_poomerang_render(gobx[d], goby[d]);
                break;
            default:
                break;
        }
    }
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
        case GK_POOMERANG:      poomerang_shot(d, s); break;
        default:
            break;
    }
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
            default:
                break;
        }
    }
}

// for all gobs:
// - set to new random position.
// - put into spawning mode.
void gobs_reset() {
    uint8_t t=0;
    uint8_t g;
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
            default:
                gob_randompos(g);
                break;
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


void gob_standard_kaboom(uint8_t d, uint8_t shot, uint8_t points)
{
    player_add_score(points);
    sys_addeffect(gobx[d]+(8<<FX), goby[d]+(8<<FX), EK_KABOOM);
    sys_sfx_play(SFX_KABOOM);
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
void grunt_create(uint8_t d)
{
    gobkind[d] = GK_GRUNT;
    gobflags[d] = GF_PERSIST | GF_LOCKS_LEVEL | GF_COLLIDES_SHOT | GF_COLLIDES_PLAYER;
    gobdat[d] = 0;
    gobtimer[d] = 0;
    grunt_reset(d);
}

void grunt_reset(uint8_t d)
{
    gob_randompos(d);
    gobvx[d] = (2 << FX) + (rnd() & 0x7f);
    gobvy[d] = (2 << FX) + (rnd() & 0x7f);
}


void grunt_tick(uint8_t d)
{
    int16_t px,py;
    // update every 16 frames
    if (((tick+d) & 0x0f) != 0x00) {
       return;
    }
    px = plrx[0];
    if (px < gobx[d]) {
        gobx[d] -= gobvx[d];
    } else if (px > gobx[d]) {
        gobx[d] += gobvx[d];
    }
    py = plry[0];
    if (py < goby[d]) {
        goby[d] -= gobvy[d];
    } else if (py > goby[d]) {
        goby[d] += gobvy[d];
    }
}

void grunt_shot(uint8_t d, uint8_t shot)
{
    gob_standard_kaboom(d, shot, 50);
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

void tank_create(uint8_t d)
{
    gobkind[d] = GK_TANK;
    gobflags[d] = GF_PERSIST | GF_LOCKS_LEVEL | GF_COLLIDES_SHOT | GF_COLLIDES_PLAYER;
    gobdat[d] = 8;  // life
}

void tank_reset(uint8_t d)
{
    gob_randompos(d);
    gobvx[d] = 0;
    gobvy[d] = 0;
    gobtimer[d] = 0;    // highlight timer
}


void tank_tick(uint8_t d)
{
    const int16_t vx = (5<<FX);
    const int16_t vy = (5<<FX);
    int16_t px = plrx[0];
    int16_t py = plry[0];

    if (gobtimer[d] > 0) {
        --gobtimer[d];
    }
    // update every 32 frames
    if (((tick+d) & 0x1f) != 0x00) {
       return;
    }
    if (px < gobx[d]) {
        gobx[d] -= vx;
    } else if (px > gobx[d]) {
        gobx[d] += vx;
    }
    if (py < goby[d]) {
        goby[d] -= vy;
    } else if (py > goby[d]) {
        goby[d] += vy;
    }
}

void tank_shot(uint8_t d, uint8_t shot)
{
    --gobdat[d];
    if (gobdat[d]==0) {
        // boom.
        gob_standard_kaboom(d, shot, 100);
    } else {
        // knockback
        gobx[d] += (shot_xvel(shot) >> 0);
        goby[d] += (shot_yvel(shot) >> 0);
        gobtimer[d] = 8;
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
    gobvx[d] += (shot_xvel(s) >> FX);
    gobvy[d] += (shot_yvel(s) >> FX);
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
        gobvx[d] += (shot_xvel(shot) >> 4);
        gobvy[d] += (shot_yvel(shot) >> 4);
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

