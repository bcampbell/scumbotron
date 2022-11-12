#ifndef PLAYER_H
#define PLAYER_H

#include <stdint.h>
#include <stdbool.h>

extern uint8_t player_lives;
extern uint32_t player_score;
extern const uint8_t shot_spr[16];

#define MAX_PLAYERS 1
// player vars
extern int16_t plrx[MAX_PLAYERS];
extern int16_t plry[MAX_PLAYERS];
extern uint8_t plrtimer[MAX_PLAYERS];
extern uint8_t plrfacing[MAX_PLAYERS];

#if 0
// shot vars
extern int16_t shotx[MAX_SHOTS];
extern int16_t shoty[MAX_SHOTS];
extern uint8_t shotdir[MAX_SHOTS];
extern uint8_t shottimer[MAX_SHOTS];
#endif

void player_renderall();
void player_tickall();
bool player_collisions();
void player_create(uint8_t d, int16_t x, int16_t y);
void player_tick(uint8_t d);
void shot_tick(uint8_t s);

#endif // PLAYER_H
