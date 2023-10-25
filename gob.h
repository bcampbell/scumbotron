#ifndef GOB_H
#define GOB_H

#include <stdint.h>
#include <stdbool.h>
#include "plat.h"   // for FX

//
// Game object (gob) Stuff
//

// Kinds
#define GK_NONE 0
#define GK_BLOCK 3
#define GK_GRUNT 4
#define GK_BAITER 5
#define GK_AMOEBA_BIG 6
#define GK_AMOEBA_MED 7
#define GK_AMOEBA_SMALL 8
#define GK_TANK 9
#define GK_HZAPPER 10
#define GK_VZAPPER 11
#define GK_FRAGGER 12
#define GK_FRAG 13
#define GK_POWERUP 14
#define GK_VULGON 15
#define GK_POOMERANG 16
#define GK_HAPPYSLAPPER 17
#define GK_MARINE 18
#define GK_BRAIN 19
#define GK_ZOMBIE 20
#define GK_RIFASHARK 21
#define GK_RIFASPAWNER 22
#define GK_BOSS 23
#define GK_BOSSSEG 24

// Flags
// lower 3 bits used for highlight timer
#define GF_HIGHLIGHT_MASK 0x07
#define GF_SPAWNING 0x08    // Currently spawning (uses gobtimer)
#define GF_LOCKS_LEVEL 0x10 // Must be destroyed to complete level
#define GF_PERSIST 0x20  // Survives level reset (eg after player death)
#define GF_COLLIDES_PLAYER 0x40 // Collides with player.
#define GF_COLLIDES_SHOT 0x80   // Collides with player shots.

// Gob tables.
#define MAX_GOBS 40

extern uint8_t gobkind[MAX_GOBS];
extern uint8_t gobflags[MAX_GOBS];
extern int16_t gobx[MAX_GOBS];
extern int16_t goby[MAX_GOBS];
extern int16_t gobvx[MAX_GOBS];
extern int16_t gobvy[MAX_GOBS];
extern uint8_t gobdat[MAX_GOBS];
extern uint8_t gobtimer[MAX_GOBS];


// set when level is completely cleared
extern bool gobs_certainbonus;

// these are set by gobs_tick()
extern uint8_t gobs_lockcnt;   // num gobs holding level open.
extern uint8_t gobs_spawncnt;  // num gobs spawning.
extern uint8_t gobs_clearablecnt;  // num gobs remaining which are clearable.

// table to filter out illegal directions (left+right etc)
extern const uint8_t dir_fix[16];
// table to convert DIR_ bits to angle 0..23
extern const uint8_t dir_to_angle24[16];

// gob functions
void gobs_clear();
void gobs_tick(bool spawnphase);
void gobs_render();
void gob_shot(uint8_t g, uint8_t shot);
bool gob_playercollide(uint8_t g, uint8_t plr);
uint8_t gob_alloc();
void gob_randompos(uint8_t d);
void gobs_reset();
void gobs_spawning();   // put active gobs into spawn mode.
void gobs_create(uint8_t kind, uint8_t n);

// generic gob fns
void gob_move_bounce_x(uint8_t d);
void gob_move_bounce_y(uint8_t d);

static inline int16_t gob_size(uint8_t d) {
    switch (gobkind[d]) {
        case GK_AMOEBA_BIG: return 32<<FX;
        case GK_AMOEBA_SMALL: return 12<<FX;
        case GK_BOSS: return 32<<FX;
        case GK_BOSSSEG: return 16<<FX;
        default: return 16<<FX;
    }
}
int16_t gob_manhattan_dist(uint8_t a, uint8_t b);

// grunt fns
void grunt_create(uint8_t d);
void grunt_reset(uint8_t d);
void grunt_tick(uint8_t d);
void grunt_shot(uint8_t d, uint8_t shot);

// baiter fns
void baiter_create(uint8_t g);
void baiter_reset(uint8_t g);
void baiter_spawn(uint8_t g, int16_t x, int16_t y);
void baiter_tick(uint8_t g);
void baiter_reset(uint8_t g);
void baiter_shot(uint8_t g, uint8_t shot);

// block fns
void block_create(uint8_t d);
void block_reset(uint8_t d);
void block_shot(uint8_t d, uint8_t shot);

// tank fns
void tank_create(uint8_t d);
void tank_shot(uint8_t d, uint8_t shot);
void tank_tick(uint8_t d);
void tank_reset(uint8_t d);

// amoeba fns
void amoeba_create(uint8_t d);
void amoeba_shot(uint8_t d, uint8_t shot);
void amoeba_tick(uint8_t d);
void amoeba_reset(uint8_t d);

// zapper fns
void hzapper_create(uint8_t d);
void vzapper_create(uint8_t d);
void zapper_tick(uint8_t d);
void zapper_reset(uint8_t d);
void zapper_shot(uint8_t d, uint8_t shot);
#define ZAPPER_OFF 0
#define ZAPPER_WARMING_UP 1
#define ZAPPER_ON 2
uint8_t zapper_state(uint8_t d);

// fragger
void fragger_create(uint8_t d);
void fragger_shot(uint8_t d, uint8_t shot);
void fragger_tick(uint8_t d);
void fragger_reset(uint8_t d);
void frag_tick(uint8_t d);

// frag
void frag_spawn(uint8_t f, int16_t x, int16_t y, uint8_t dir);
void frag_shot(uint8_t d, uint8_t shot);

// powerup fns
void powerup_create(uint8_t d, int16_t x, int16_t y, uint8_t kind);
void powerup_tick(uint8_t d);
bool powerup_playercollide(uint8_t g, uint8_t plr);

// vulgon fns
void vulgon_create(uint8_t d);
void vulgon_tick(uint8_t d);
void vulgon_reset(uint8_t d);
void vulgon_shot(uint8_t d, uint8_t shot);

// poomerang fns
#define POOMERANG_VEL (1<<FX)
void poomerang_spawn(uint8_t d, int16_t x, int16_t y, int16_t vx, int16_t vy);
void poomerang_tick(uint8_t d);
void poomerang_shot(uint8_t d, uint8_t shot);

// happyslapper fns
#define HAPPYSLAPPER_SLEEP_TIME 50
#define HAPPYSLAPPER_RUN_TIME 50
void happyslapper_create(uint8_t d);
void happyslapper_tick(uint8_t d);
void happyslapper_reset(uint8_t d);
void happyslapper_shot(uint8_t d, uint8_t shot);

// heavily armoured space marine fns
void marine_create(uint8_t g);
void marine_tick(uint8_t g);
void marine_reset(uint8_t g);
bool marine_playercollide(uint8_t g, uint8_t plr);
// return true if marine has been collected and is trailing after player.
static inline bool marine_is_trailing(uint8_t g) {
    return (gobflags[g] & GF_COLLIDES_PLAYER) ? false : true;
};

// Brain fns
void brain_create(uint8_t d);
void brain_tick(uint8_t d);
void brain_reset(uint8_t d);
void brain_shot(uint8_t d, uint8_t shot);

// Zombie fns
void zombie_create(uint8_t d);
void zombie_tick(uint8_t d);
void zombie_reset(uint8_t d);
void zombie_shot(uint8_t d, uint8_t shot);

// Rifashark fns
void rifashark_spawn(uint8_t g, int16_t x, int16_t y, uint8_t direction);
void rifashark_create(uint8_t d);
void rifashark_tick(uint8_t d);
void rifashark_reset(uint8_t d);
void rifashark_shot(uint8_t d, uint8_t shot);

// Rifaspawner fns
void rifaspawner_create(uint8_t d);
void rifaspawner_tick(uint8_t d);
void rifaspawner_reset(uint8_t d);
void rifaspawner_shot(uint8_t d, uint8_t shot);

// Boss fns.
void boss_create(uint8_t g);
void boss_shot(uint8_t g, uint8_t shot);
void boss_tick(uint8_t g);
void boss_reset(uint8_t g);

// Boss segment fns.
// upper 4 bits of gobdat is state:
#define SEGSTATE_NORMAL     0x00
#define SEGSTATE_DESTRUCT   0xf0
#define SEGSTATE_HIDE       0x10

static inline uint8_t bossseg_state(uint8_t g) {
    return gobdat[g] & 0xf0;
}
void bossseg_spawn(uint8_t g, uint8_t slot);
void bossseg_shot(uint8_t g, uint8_t shot);
void bossseg_tick(uint8_t g);
void bossseg_reset(uint8_t g);
void bossseg_destruct(uint8_t g, uint8_t delay);


// Returns true if player can clear this gob from the level (by shooting or collecting)
static inline bool gob_is_clearable(uint8_t g) {
    uint8_t k = gobkind[g];
    if ((k == GK_MARINE && marine_is_trailing(g)) || 
        k == GK_NONE || k == GK_HZAPPER || k == GK_VZAPPER) {
        return false;
    } else {
        return true;
    }
}

#endif // GOB_H
