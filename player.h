#ifndef PLAYER_H
#define PLAYER_H

#include <stdint.h>
#include <stdbool.h>
#include "gob.h"

extern uint8_t player_lives;
extern uint32_t player_score;

// player vars
#define MAX_PLAYERS 1
extern int16_t plrx[MAX_PLAYERS];
extern int16_t plry[MAX_PLAYERS];
extern uint8_t plrtimer[MAX_PLAYERS];
extern uint8_t plrfacing[MAX_PLAYERS];
extern uint8_t plralive[MAX_PLAYERS]; // 0=dead

// record previous positions of players
#define PLR_HIST_LEN 32
extern int16_t plrhistx[MAX_PLAYERS][PLR_HIST_LEN];
extern int16_t plrhisty[MAX_PLAYERS][PLR_HIST_LEN];
extern uint8_t plrhistidx[MAX_PLAYERS];

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
extern int16_t shotvx[MAX_SHOTS];
extern int16_t shotvy[MAX_SHOTS];
extern uint8_t shotdir[MAX_SHOTS];  // 0 = inactive
extern uint8_t shottimer[MAX_SHOTS];

#define SHOT_SPD (8<<FX)

void shot_clearall();
void shot_tick(uint8_t s);
void shot_collisions();

#endif // PLAYER_H
