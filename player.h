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
extern uint8_t plrweapon[MAX_PLAYERS];

// record previous positions of players
#define PLR_HIST_LEN 32
extern int16_t plrhistx[MAX_PLAYERS][PLR_HIST_LEN];
extern int16_t plrhisty[MAX_PLAYERS][PLR_HIST_LEN];
extern uint8_t plrhistidx[MAX_PLAYERS];

void player_renderall();
void player_tickall();
bool player_collisions();
void player_create(uint8_t plr);  // to create player at start of game
void player_reset(uint8_t plr);   // reset player for getready stage
void player_tick(uint8_t plr);
void player_add_score(uint8_t points);
void player_extra_life(uint8_t plr);    // give extra life
void player_powerup(uint8_t plr);   // power up weapon

#define MAX_SHOTS 16
// shot vars
extern int16_t shotx[MAX_SHOTS];
extern int16_t shoty[MAX_SHOTS];
extern int16_t shotvx[MAX_SHOTS];
extern int16_t shotvy[MAX_SHOTS];
extern uint8_t shotdir[MAX_SHOTS];      // 0..23
extern uint8_t shottimer[MAX_SHOTS];    // 0=inactive
extern uint8_t shotkind[MAX_SHOTS];

void shot_clearall();
void shot_tick(uint8_t s);
void shot_collisions();

#endif // PLAYER_H
