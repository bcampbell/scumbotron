#ifndef PLAYER_H
#define PLAYER_H

#include <stdint.h>
#include <stdbool.h>

extern uint8_t player_lives;
extern uint32_t player_score;
extern const uint8_t shot_spr[16];

void player_create(uint8_t d, int16_t x, int16_t y);
void player_tick(uint8_t d);
void shot_tick(uint8_t s);

#endif // PLAYER_H
