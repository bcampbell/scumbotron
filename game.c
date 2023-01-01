// mos-cx16-clang -Os -o game.prg game.c

#include <stdint.h>
#include <stdbool.h>

#include "sys.h"
#include "gob.h"
#include "player.h"

void titlescreen_run()
{
    sys_clr();
    while(1)
    {
        uint8_t i;
        inp_tick();
        sys_render_start();
        for (i=0; i<20; ++i) {
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
//    dudes_spawn(GK_AMOEBA_BIG, 10);
    dudes_spawn(GK_GRUNT, 10);
    dudes_spawn(GK_HZAPPER, 2);
    dudes_spawn(GK_VZAPPER, 2);
    dudes_spawn(GK_BAITER, 5);

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
    player_create(0, ((SCREEN_W / 2) - 8) << FX, ((SCREEN_H / 2) - 8) << FX);
    level_init(level);
    while (1) {
        sys_gatso(GATSO_ON);
        sys_render_start();
        player_renderall();
        gobs_render();
        sys_hud(level, player_lives, player_score);

        sys_render_finish();

        inp_tick();

        gobs_tick(state == LEVELSTATE_GETREADY);    // spawnphase?
        player_tickall();
        shot_collisions();
        if (state == LEVELSTATE_PLAY) {
            bool playerkilled = player_collisions();
            if (playerkilled) {
                --player_lives;
                state = LEVELSTATE_KILLED;
                statetimer = 0;
            }
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

                        player_create(0, ((SCREEN_W / 2) - 8) << FX, ((SCREEN_H / 2) - 8) << FX);
                        dudes_reset();
                        statetimer = 0;
                    } else {
                        return LEVELRESULT_GAMEOVER;
                    }
                }
                break;
        }
        sys_gatso(GATSO_OFF);
        waitvbl();
    }
}


void game_run()
{
    // init new game
    uint8_t level = 0;

    player_lives = 3;
    player_score = 0;

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
