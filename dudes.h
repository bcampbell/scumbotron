#ifndef DUDES_H
#define DUDES_H

#include "gob.h"

void amoeba_init(uint8_t d);
void grunt_init(uint8_t d);
void baiter_init(uint8_t d);
void block_init(uint8_t d);

void grunt_tick(uint8_t d);
void baiter_tick(uint8_t d);
void amoeba_tick(uint8_t d);
void amoeba_shot(uint8_t d);

#endif // DUDES_H
