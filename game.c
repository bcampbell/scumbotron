// mos-cx16-clang -Os -o game.prg game.c

#include <stdint.h>
#include <stdbool.h>

#include "sys.h"
#include "gob.h"
#include "player.h"

#define STATE_TITLESCREEN 0
#define STATE_GETREADY 1   // pause, get ready
#define STATE_PLAY 2       // main gameplay
#define STATE_CLEARED 3    // all dudes clear
#define STATE_KILLED 4     // player just died (but has another life)
#define STATE_GAMEOVER 5

static uint8_t state;
static uint8_t statetimer;
static uint8_t level;
static uint8_t baiter_timer;
static uint8_t baiter_count;

static void enter_STATE_TITLESCREEN();
static void enter_STATE_GETREADY();
static void enter_STATE_PLAY();
static void enter_STATE_CLEARED();
static void enter_STATE_KILLED();
static void enter_STATE_GAMEOVER();

static void tick_STATE_TITLESCREEN();
static void tick_STATE_GETREADY();
static void tick_STATE_PLAY();
static void tick_STATE_CLEARED();
static void tick_STATE_KILLED();
static void tick_STATE_GAMEOVER();

static void render_STATE_TITLESCREEN();
static void render_STATE_GETREADY();
static void render_STATE_PLAY();
static void render_STATE_CLEARED();
static void render_STATE_KILLED();
static void render_STATE_GAMEOVER();

static void level_init(uint8_t level);
static void level_baiter_check();

void game_init()
{
    enter_STATE_TITLESCREEN();
}

void game_tick()
{
    switch(state) {
    case STATE_TITLESCREEN: tick_STATE_TITLESCREEN(); break;
    case STATE_GETREADY:    tick_STATE_GETREADY(); break;
    case STATE_PLAY:        tick_STATE_PLAY(); break;
    case STATE_CLEARED:     tick_STATE_CLEARED(); break;
    case STATE_KILLED:      tick_STATE_KILLED(); break;
    case STATE_GAMEOVER:    tick_STATE_GAMEOVER(); break;
    }
}

#if 0

const char * statenames[] = {
    "TITLESCREEN",
    "GETREADY",
    "PLAY",
    "CLEARED",
    "KILLED",
    "GAMEOVER"
};
#endif

void game_render()
{
    switch(state) {
        case STATE_TITLESCREEN: render_STATE_TITLESCREEN(); break;
        case STATE_GETREADY:    render_STATE_GETREADY(); break;
        case STATE_PLAY:        render_STATE_PLAY(); break;
        case STATE_CLEARED:     render_STATE_CLEARED(); break;
        case STATE_KILLED:      render_STATE_KILLED(); break;
        case STATE_GAMEOVER:    render_STATE_GAMEOVER(); break;
    }
}

/*
 * STATE_TITLESCREEN
 */
static void enter_STATE_TITLESCREEN()
{
    state = STATE_TITLESCREEN;
    statetimer=0;
    sys_clr();
}


static void tick_STATE_TITLESCREEN()
{
    if (sys_inp_dualsticks()) {
        // init new game
        uint8_t level = 0;
        player_lives = 3;
        player_score = 0;
        level_init(level);
        enter_STATE_GETREADY();
    }
}

static void render_STATE_TITLESCREEN()
{
    uint8_t i;
    for (i=0; i<20; ++i) {
        sys_text(i,i,"*** TITLE SCREEN ***", ((tick/2)-i) & 0x0f);
    }
}


/*
 * STATE_GETREADY
 */
static void enter_STATE_GETREADY()
{
    state = STATE_GETREADY;
    statetimer = 0;

    baiter_count = 0;
    baiter_timer = 0;
    player_create(0, ((SCREEN_W / 2) - 8) << FX, ((SCREEN_H / 2) - 8) << FX);
    sys_clr();
}

static void tick_STATE_GETREADY()
{
    gobs_tick(true);    // spawnphase?
    if(gobs_spawncnt == 0) {
        enter_STATE_PLAY();
    }
}

static void render_STATE_GETREADY()
{
    player_renderall();
    gobs_render();
    sys_hud(level, player_lives, player_score);
    sys_text(10,10,"GET READY", (tick/2) & 0x0f);
}


/*
 * STATE_PLAY
 */

static void enter_STATE_PLAY()
{
    state = STATE_PLAY;
    statetimer = 0;
    sys_clr();
}

static void tick_STATE_PLAY()
{
    gobs_tick(false);    // not spawnphase
    player_tickall();
    shot_collisions();
    bool playerkilled = player_collisions();
    if (playerkilled) {
        enter_STATE_KILLED();
        return;
    }
    if (gobs_lockcnt == 0) {
        enter_STATE_CLEARED();
        return;
    }
    level_baiter_check();
}

static void render_STATE_PLAY()
{
    player_renderall();
    gobs_render();
    sys_hud(level, player_lives, player_score);
}


/*
 * STATE_KILLED
 */
static void enter_STATE_KILLED()
{
    state = STATE_KILLED;
    statetimer = 0;
    sys_clr();
}

static void tick_STATE_KILLED()
{
    if (++statetimer > 60) {
        if (player_lives > 0 ) {
            --player_lives;
            dudes_reset();
            enter_STATE_GETREADY();
        } else {
            enter_STATE_GAMEOVER();
        }
    }
}

static void render_STATE_KILLED()
{
    player_renderall();
    gobs_render();
    sys_hud(level, player_lives, player_score);
    sys_text(10,10,"OWWWWWWWW", (tick/2) & 0x0f);
}

/*
 * STATE_CLEARED
 */
static void enter_STATE_CLEARED()
{
    state = STATE_CLEARED;
    statetimer = 0;
    sys_clr();
}

static void tick_STATE_CLEARED()
{
    if (++statetimer > 60) {
        // next level
        ++level;
        level_init(level);
        enter_STATE_GETREADY();
    }
}

static void render_STATE_CLEARED()
{
    player_renderall();
    gobs_render();
    sys_hud(level, player_lives, player_score);
    sys_text(10,10,"CLEAR     ", (tick/2) & 0x0f);
}

/*
 * STATE_GAMEOVER
 */
static void enter_STATE_GAMEOVER()
{
    state = STATE_GAMEOVER;
    statetimer = 0;
    sys_clr();
}

static void tick_STATE_GAMEOVER()
{
    if (++statetimer > 60) {
        enter_STATE_TITLESCREEN();
    }
}

static void render_STATE_GAMEOVER()
{
    sys_text(10, 10, "GAME OVER", (tick/2) & 0x0f);
}


/*
 * Game fns
 */

static void level_init(uint8_t level)
{
    gobs_init();

//    dudes_spawn(GK_AMOEBA_BIG, 10);
    dudes_spawn(GK_GRUNT, 10);
    dudes_spawn(GK_FRAGGER, 10);
    dudes_spawn(GK_HZAPPER, 2);
    dudes_spawn(GK_VZAPPER, 2);
    dudes_spawn(GK_AMOEBA_BIG, 2);

    dudes_reset();    // position and intro
}

static void level_baiter_check()
{
    // update baiters
    if ((tick & 0x0f) == 0x0f) {
        // 16*100 = 1600 (about 25 secs)
        // 16*33 = about 9 secs
        uint8_t threshold = baiter_count == 0 ? 100 : 33;
        ++baiter_timer;
        if (baiter_timer > threshold) {
            uint8_t i;
            baiter_timer = 0;
            if (baiter_count < 15) {
                ++baiter_count;
            }
            // spawn `em!
            for (i = 0; i < baiter_count; ++i) {
                uint8_t b = dude_alloc();
                if (b >= MAX_GOBS) {
                    return;
                }
                // TODO: spawning.
                baiter_init(b);
                dude_randompos(b);
                gobflags[b] |= GF_SPAWNING;
                gobtimer[b] = 8 + (i*8);
            }
        }
    }
}

