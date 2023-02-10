#include "sys.h"
#include "game.h"
#include "misc.h"

#define NUM_HIGHSCORES 10

#define HIGHSCORE_NAME_SIZE 3

static char names[NUM_HIGHSCORES][HIGHSCORE_NAME_SIZE] = {
    {'a','c','e'},
    {'a','c','e'},
    {'a','c','e'},
    {'a','c','e'},
    {'a','c','e'},
    {'a','c','e'},
    {'a','c','e'},
    {'a','c','e'},
    {'a','c','e'},
    {'a','c','e'}
};

static uint32_t scores[NUM_HIGHSCORES] = {90000,80000,70000,60000,50000,40000,30000,20000,10000,0};


/*
 * STATE_HIGHSCORES
 */
void enter_STATE_HIGHSCORES()
{
    state = STATE_HIGHSCORES;
    statetimer = 0;
    sys_clr();
}

void tick_STATE_HIGHSCORES()
{
    if (++statetimer > 200 || sys_inp_dualsticks()) {
        enter_STATE_TITLESCREEN();
    }
}

void render_STATE_HIGHSCORES()
{
    uint8_t slot;
    char buf[8];
    for (slot = 0; slot<NUM_HIGHSCORES; ++slot) {
        sys_textn(slot + slot + 10, 4 + slot, names[slot], HIGHSCORE_NAME_SIZE, ((tick+slot)/2) & 0x0f);
        hex32(bin2bcd(scores[slot]), buf);
        sys_textn(slot + slot, 4 + slot, buf, 8, ((tick-slot)/2) & 0x0f);
    }
}


