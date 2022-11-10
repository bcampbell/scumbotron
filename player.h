#ifndef PLAYER_H
#define PLAYER_H

#include <stdint.h>
#include <stdbool.h>

extern uint8_t player_lives;
extern uint32_t player_score;
extern const uint8_t shot_spr[16];

void shot_collisions();
bool player_collisions();
uint8_t shot_alloc();
void player_create(uint8_t d, int16_t x, int16_t y);


#endif // PLAYER_H
