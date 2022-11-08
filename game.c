// mos-cx16-clang -Os -o game.prg game.c

#include <stdint.h>
#include <stdbool.h>

#include "sys_cx16.h"
#include "gob.h"


void titlescreen_run()
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



#define LEVELRESULT_QUIT 0
#define LEVELRESULT_COMPLETED 1
#define LEVELRESULT_GAMEOVER 2

static void level_init(uint8_t level) {
    switch(level & 1) {
        case 0:
            dudes_spawn(GK_AMOEBA_BIG, 4);
//            dudes_spawn(GK_AMOEBA_BIG, 5);
//            dudes_spawn(GK_AMOEBA_MED, 5);
//            dudes_spawn(GK_AMOEBA_SMALL, 10);
            break;
        case 1:
            dudes_spawn(GK_AMOEBA_BIG, 5);
            dudes_spawn(GK_BLOCK, MAX_DUDES-5);
            break;
    }

    dudes_reset();    // position and intro
}


#define LEVELSTATE_GETREADY 0   // pause, get ready
#define LEVELSTATE_PLAY 1       // main gameplay
#define LEVELSTATE_CLEARED 2    // all dudes clear
#define LEVELSTATE_KILLED 3     // player just died

uint8_t level_run(uint8_t level) {
    uint8_t statetimer=0;
    uint8_t state = LEVELSTATE_GETREADY;

    sys_clr();
    gobs_init();
    player_create(FIRST_PLAYER, ((SCREEN_W / 2) - 8) << FX, ((SCREEN_H / 2) - 8) << FX);
    level_init(level);
    while (1) {
        sys_render_start();
        gobs_render();
        player_score += 1000;
        sys_hud(level, player_lives, player_score);

        sys_render_finish();

        inp_tick();

        gobs_tick(state == LEVELSTATE_GETREADY);    // spawnphase?
        shot_collisions();
        bool playerkilled = player_collisions();

        if (playerkilled) {
            --player_lives;
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
                    return LEVELRESULT_COMPLETED;
                }
                break;
            case LEVELSTATE_KILLED:
                if (player_lives > 0) {
                    sys_text(10,10,"OWWWWWWWW!", tick & 0x07);
                } else {
                    sys_text(10,10,"GAME OVER!", tick & 0x07);
                }
                if (++statetimer > 60) {
                    sys_text(10,10,"OWWWWWWWW!", 0);

                    if (player_lives > 0) {
                        state = LEVELSTATE_GETREADY;

                        player_create(FIRST_PLAYER, ((SCREEN_W / 2) - 8) << FX, ((SCREEN_H / 2) - 8) << FX);
                        dudes_reset();
                        statetimer = 0;
                    } else {
                        return LEVELRESULT_GAMEOVER;
                    }
                }
                break;
        }
        waitvbl();
    }
}


void game_run()
{
    // init new game
    player_lives = 3;
    player_score = 0;

    uint8_t level = 0;

    while(1) {
        switch(level_run(level)) {
            case LEVELRESULT_COMPLETED:
                ++level;
                break;
            case LEVELRESULT_GAMEOVER:
            case LEVELRESULT_QUIT:
                return;
        }
    }
}



int main(void) {
    sys_init();
    while(1) {
        titlescreen_run();
        game_run();
    }
}
