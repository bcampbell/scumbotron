#ifndef GOB_H
#define GOB_H

#include <stdint.h>
#include <stdbool.h>

//
// Game object (gob) Stuff
//

#define DIR_UP 0x08
#define DIR_DOWN 0x04
#define DIR_LEFT 0x02
#define DIR_RIGHT 0x01


// Kinds
#define GK_NONE 0
#define GK_PLAYER 1
#define GK_SHOT 2
#define GK_BLOCK 32
#define GK_GRUNT 33

#define GK_SPAWNFLAG 0x80

// Gob tables.
#define MAX_PLAYERS 1
#define MAX_SHOTS 7
#define MAX_DUDES 40

#define FIRST_PLAYER 0
#define FIRST_SHOT (FIRST_PLAYER + MAX_PLAYERS)
#define FIRST_DUDE (FIRST_SHOT + MAX_SHOTS)

#define MAX_GOBS (MAX_PLAYERS + MAX_SHOTS + MAX_DUDES)

// Vars

extern uint8_t gobkind[MAX_GOBS];
extern uint8_t gobflags[MAX_GOBS];
extern uint16_t gobx[MAX_GOBS];
extern uint16_t goby[MAX_GOBS];
extern uint8_t gobdat[MAX_GOBS];
extern uint8_t gobtimer[MAX_GOBS];

// these two set by gobs_tick()
extern uint8_t gobs_lockcnt;   // num dudes holding level open.
extern uint8_t gobs_spawncnt;  // num dudes spawning.


extern uint8_t player_lives;
extern uint32_t player_score;

// gob functions
void gobs_init();
void gobs_tick(bool spawnphase);
void gobs_render();
void shot_collisions();
bool player_collisions();
uint8_t dude_alloc();
uint8_t shot_alloc();
void dude_randompos(uint8_t d);
void dudes_reset();

// create fns
void player_create(uint8_t d, int x, int y);
void dudes_spawn(uint8_t kind, uint8_t n);

// tick fns
void spawning_tick(uint8_t g);
void player_tick(uint8_t d);
void shot_tick(uint8_t d);
void grunt_tick(uint8_t d);


#endif // GOB_H
