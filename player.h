#ifndef PLAYER_H
#define PLAYER_H

#include <stdint.h>
#include <stdbool.h>
#include "gob.h"

extern uint8_t player_lives;
extern uint32_t player_score;

#define MAX_PLAYERS 1
// player vars
extern int16_t plrx[MAX_PLAYERS];
extern int16_t plry[MAX_PLAYERS];
extern uint8_t plrtimer[MAX_PLAYERS];
extern uint8_t plrfacing[MAX_PLAYERS];
extern uint8_t plralive[MAX_PLAYERS]; // 0=dead

void player_renderall();
void player_tickall();
bool player_collisions();
void player_create(uint8_t d, int16_t x, int16_t y);
void player_tick(uint8_t d);
void player_add_score(uint8_t points);

#define MAX_SHOTS 7
// shot vars
extern int16_t shotx[MAX_SHOTS];
extern int16_t shoty[MAX_SHOTS];
extern uint8_t shotdir[MAX_SHOTS];  // 0 = inactive
extern uint8_t shottimer[MAX_SHOTS];

#define SHOT_SPD (8<<FX)

void shot_clearall();
void shot_tick(uint8_t s);
void shot_collisions();

static inline int16_t shot_yvel(uint8_t s) {
    uint8_t dir = shotdir[s];
    if (dir & DIR_UP) {
       return -SHOT_SPD;
    } else if (dir & DIR_DOWN) {
       return SHOT_SPD;
    }
    return 0;
}

static inline int16_t shot_xvel(uint8_t s) {
    uint8_t dir = shotdir[s];
    if (dir & DIR_LEFT) {
       return -SHOT_SPD;
    } else if (dir & DIR_RIGHT) {
       return SHOT_SPD;
    }
    return 0;
}


#endif // PLAYER_H
