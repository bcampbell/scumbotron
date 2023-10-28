#ifndef HIGHSCORE_H_INCLUDED
#define HIGHSCORE_H_INCLUDED

#include <stdint.h>

#define HIGHSCORE_COUNT 10
#define HIGHSCORE_NAME_SIZE 8

typedef struct highscore {
    uint32_t score;
    char name[HIGHSCORE_NAME_SIZE];
} highscore;


extern highscore highscore_table[HIGHSCORE_COUNT];

void highscore_init();

#endif // HIGHSCORE_H_INCLUDED
