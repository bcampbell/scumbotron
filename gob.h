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

// Flags
#define GF_SPAWNING 0x01
// bits for update phases...


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
extern uint8_t gobs_lockcnt;   // num dudes holding level open.
extern uint8_t gobs_spawncnt;  // num dudes spawning.


// gob functions
void gobs_init();
void gobs_tick(bool spawnphase);
void gobs_render();
uint8_t dude_alloc();
void dude_randompos(uint8_t d);
void dudes_reset();
void dudes_spawn(uint8_t kind, uint8_t n);

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
void grunt_init(uint8_t d);
void grunt_shot(uint8_t d, uint8_t shot);

// baiter fns
void baiter_init(uint8_t d);
void baiter_shot(uint8_t d, uint8_t shot);

// block fns
void block_init(uint8_t d);
void block_shot(uint8_t d, uint8_t shot);

// tank fns
void tank_init(uint8_t d);
void tank_shot(uint8_t d, uint8_t shot);

// amoeba fns
void amoeba_init(uint8_t d);
void amoeba_shot(uint8_t d, uint8_t shot);

// zapper fns
void hzapper_init(uint8_t d);
void vzapper_init(uint8_t d);
void zapper_shot(uint8_t d, uint8_t shot);
#define ZAPPER_OFF 0
#define ZAPPER_WARMING_UP 1
#define ZAPPER_ON 2
uint8_t zapper_state(uint8_t d);

// fragger
void fragger_init(uint8_t d);
void fragger_shot(uint8_t d, uint8_t shot);

// frag
void frag_spawn(uint8_t f, int16_t x, int16_t y, uint8_t dir);
void frag_shot(uint8_t d, uint8_t shot);

#endif // GOB_H
