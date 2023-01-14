#ifndef GOB_H
#define GOB_H

#include <stdint.h>
#include <stdbool.h>

#include "sys.h"

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

// Flags
#define GF_SPAWNING 0x01    // Currently spawning (uses gobtimer)
#define GF_LOCKS_LEVEL 0x02 // Must be destroyed to complete level
#define GF_PERSIST 0x04  // Survives level reset (eg after player death)
#define GF_COLLIDES_PLAYER 0x08 // Collides with player.
#define GF_COLLIDES_SHOT 0x10   // Collides with player shots.

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

// these two set by gobs_tick()
extern uint8_t gobs_lockcnt;   // num gobs holding level open.
extern uint8_t gobs_spawncnt;  // num gobs spawning.


// gob functions
void gobs_clear();
void gobs_tick(bool spawnphase);
void gobs_render();
void gob_shot(uint8_t g, uint8_t shot);
uint8_t gob_alloc();
void gob_randompos(uint8_t d);
void gobs_reset();
void gobs_create(uint8_t kind, uint8_t n);

uint8_t rnd();

// generic gob fns
void gob_move_bounce_x(uint8_t d);
void gob_move_bounce_y(uint8_t d);

static inline int16_t gob_size(uint8_t d) {
    switch (gobkind[d]) {
        case GK_AMOEBA_BIG: return 32<<FX;
        case GK_AMOEBA_SMALL: return 12<<FX;
        default: return 16<<FX;
    }
}

// grunt fns
void grunt_create(uint8_t d);
void grunt_reset(uint8_t d);
void grunt_tick(uint8_t d);
void grunt_shot(uint8_t d, uint8_t shot);

// baiter fns
void baiter_tick(uint8_t d);
void baiter_create(uint8_t d);
void baiter_shot(uint8_t d, uint8_t shot);

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

// vulgon fns
void vulgon_create(uint8_t d);
void vulgon_tick(uint8_t d);
void vulgon_reset(uint8_t d);
void vulgon_shot(uint8_t d, uint8_t shot);

#endif // GOB_H
