#ifndef GOB_H
#define GOB_H

#include <stdint.h>
#include <stdbool.h>

#include "sys_cx16.h"

//
// Game object (gob) Stuff
//

#define DIR_UP 0x08
#define DIR_DOWN 0x04
#define DIR_LEFT 0x02
#define DIR_RIGHT 0x01


// Kinds
#define GK_NONE 0
#define GK_SHOT 2
#define GK_BLOCK 3
#define GK_GRUNT 4
#define GK_BAITER 5
#define GK_AMOEBA_BIG 6
#define GK_AMOEBA_MED 7
#define GK_AMOEBA_SMALL 8
#define GK_NUM_KINDS 9

// Flags
#define GF_SPAWNING 0x01
// bits for update phases...


// Gob tables.
#define MAX_SHOTS 7
#define MAX_DUDES 40

#define FIRST_SHOT 0
#define FIRST_DUDE (FIRST_SHOT + MAX_SHOTS)

#define MAX_GOBS (MAX_SHOTS + MAX_DUDES)

// Vars

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
void shot_collisions();
uint8_t dude_alloc();
void dude_randompos(uint8_t d);
void dudes_reset();
void dudes_spawn(uint8_t kind, uint8_t n);

uint8_t rnd();

static inline int16_t gob_size(uint8_t d) {
    switch (gobkind[d]) {
        case GK_AMOEBA_BIG: return 32<<FX;
        case GK_AMOEBA_SMALL: return 12<<FX;
        default: return 16<<FX;
    }
}


void amoeba_init(uint8_t d);
void grunt_init(uint8_t d);
void baiter_init(uint8_t d);
void block_init(uint8_t d);

#endif // GOB_H
