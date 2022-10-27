// mos-cx16-clang -Os -o game.prg game.c

#include <stdint.h>
#include <stdbool.h>

#include "sys_cx16.h"
#include "gob.h"

void titlescreen()
{
    sys_clr();
    while(1)
    {
        inp_tick();
        sys_render_start();
        for (uint8_t i=0; i<20; ++i) {
            sys_text(i,i,"*** TITLE SCREEN ***", ((tick/2)-i) & 0x0f);
        }
        sys_render_finish();

        if ((inp_joystate & 0x80) == 0) {
            return;
        }

        waitvbl();
    }
}

void testlevel()
{
    dudes_spawn(GK_BLOCK, 5);
    dudes_spawn(GK_GRUNT, MAX_DUDES-5);
    dudes_reset();    // position and intro
}


#define LEVELSTATE_GETREADY 0   // pause, get ready
#define LEVELSTATE_PLAY 1       // main gameplay
#define LEVELSTATE_CLEARED 2    // all dudes clear
#define LEVELSTATE_COMPLETE 4   // level was completed
#define LEVELSTATE_KILLED 5     // player just died
#define LEVELSTATE_GAMEOVER 6   // all lives lost



void level() {
    uint8_t statetimer=0;
    uint8_t state = LEVELSTATE_GETREADY;

    sys_clr();
    gobs_init();
    player_create(FIRST_PLAYER, (SCREEN_W/2) - 8, (SCREEN_H/2)-8);
    testlevel();
    while (1) {
        if (state == LEVELSTATE_COMPLETE) {
            return;
        }
        if (state == LEVELSTATE_GAMEOVER) {
            return;
        }

        sys_render_start();
        gobs_render();
        sys_render_finish();

        inp_tick();

        gobs_tick(state == LEVELSTATE_GETREADY);    // spawnphase?
        shot_collisions();
        bool playerkilled = player_collisions();

        if (playerkilled) {
            state = LEVELSTATE_KILLED;
            statetimer = 0;
        }

        // update levelstate
        switch (state) {
            case LEVELSTATE_GETREADY:
                sys_text(10,10,"GET READY", tick & 0x07);
                if(gobs_spawncnt == 0) {
                    //if (++statetimer >= 16) {
                        sys_text(10,10,"GET READY", 0); // clear
                        state = LEVELSTATE_PLAY;
                    //}
                };
                break;
            case LEVELSTATE_PLAY:
                if (gobs_lockcnt == 0) {
                    state = LEVELSTATE_CLEARED;
                    statetimer = 0;
                }
                break;
            case LEVELSTATE_CLEARED:
                sys_text(10,10,"CLEAR!", tick & 0x07);
                if (++statetimer > 60) {
                    sys_text(10,10,"CLEAR!", 0);
                    state = LEVELSTATE_COMPLETE;
                    statetimer = 0;
                }
                break;
            case LEVELSTATE_KILLED:
                sys_text(10,10,"OWWWWWWWW!", tick & 0x07);
                if (++statetimer > 60) {
                    sys_text(10,10,"OWWWWWWWW!", 0);
                    state = LEVELSTATE_GETREADY;

                    player_create(FIRST_PLAYER, (SCREEN_W/2) - 8, (SCREEN_H/2)-8);
                    dudes_reset();
                    statetimer = 0;
                }
                break;
        }
        waitvbl();
    }
}


int main(void) {
    sys_init();
    while(1) {
        titlescreen();
        level();
    }
}
