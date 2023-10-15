#include "plat.h"
#include "gob.h"
#include "player.h"
#include "highscore.h"
#include "game.h"

uint8_t state;
uint16_t statetimer;
static uint8_t level;
static uint8_t baiter_timer;
static uint8_t baiter_count;

static void tick_STATE_ATTRACT();
static void tick_STATE_TITLESCREEN();
static void tick_STATE_NEWGAME();
static void tick_STATE_GETREADY();
static void tick_STATE_PLAY();
static void tick_STATE_CLEARED();
static void tick_STATE_KILLED();
static void tick_STATE_GAMEOVER();

static void render_STATE_ATTRACT();
static void render_STATE_TITLESCREEN();
static void render_STATE_NEWGAME();
static void render_STATE_GETREADY();
static void render_STATE_PLAY();
static void render_STATE_CLEARED();
static void render_STATE_KILLED();
static void render_STATE_GAMEOVER();

static void level_init(uint8_t level);
static void level_baiter_check();

void game_init()
{
    highscore_init();
    enter_STATE_ATTRACT();
}

void game_tick()
{
    switch(state) {
    case STATE_ATTRACT: tick_STATE_ATTRACT(); break;
    case STATE_TITLESCREEN: tick_STATE_TITLESCREEN(); break;
    case STATE_NEWGAME:     tick_STATE_NEWGAME(); break;
    case STATE_GETREADY:    tick_STATE_GETREADY(); break;
    case STATE_PLAY:        tick_STATE_PLAY(); break;
    case STATE_CLEARED:     tick_STATE_CLEARED(); break;
    case STATE_KILLED:      tick_STATE_KILLED(); break;
    case STATE_GAMEOVER:    tick_STATE_GAMEOVER(); break;
    case STATE_HIGHSCORES:    tick_STATE_HIGHSCORES(); break;
    case STATE_ENTERHIGHSCORE:    tick_STATE_ENTERHIGHSCORE(); break;
    case STATE_GALLERY_BADDIES:    tick_STATE_GALLERY_BADDIES(); break;
    case STATE_GALLERY_GOODIES:    tick_STATE_GALLERY_GOODIES(); break;
    case STATE_STORY_INTRO:    tick_STATE_STORY_INTRO(); break;
    case STATE_STORY_OHNO:    tick_STATE_STORY_OHNO(); break;
    case STATE_STORY_ATTACK:    tick_STATE_STORY_ATTACK(); break;
    case STATE_STORY_RUNAWAY:    tick_STATE_STORY_RUNAWAY(); break;
    case STATE_STORY_WHATNOW:    tick_STATE_STORY_WHATNOW(); break;
    case STATE_STORY_DONE:    tick_STATE_STORY_DONE(); break;
    }
}

void game_render()
{
    switch(state) {
        case STATE_ATTRACT:     render_STATE_ATTRACT(); break;
        case STATE_TITLESCREEN: render_STATE_TITLESCREEN(); break;
        case STATE_NEWGAME:     render_STATE_NEWGAME(); break;
        case STATE_GETREADY:    render_STATE_GETREADY(); break;
        case STATE_PLAY:        render_STATE_PLAY(); break;
        case STATE_CLEARED:     render_STATE_CLEARED(); break;
        case STATE_KILLED:      render_STATE_KILLED(); break;
        case STATE_GAMEOVER:    render_STATE_GAMEOVER(); break;
        case STATE_HIGHSCORES:    render_STATE_HIGHSCORES(); break;
        case STATE_ENTERHIGHSCORE:  render_STATE_ENTERHIGHSCORE(); break;
        case STATE_GALLERY_BADDIES: render_STATE_GALLERY_BADDIES(); break;
        case STATE_GALLERY_GOODIES: render_STATE_GALLERY_GOODIES(); break;
        case STATE_STORY_INTRO:      render_STATE_STORY_INTRO(); break;
        case STATE_STORY_OHNO:      render_STATE_STORY_OHNO(); break;
        case STATE_STORY_ATTACK:    render_STATE_STORY_ATTACK(); break;
        case STATE_STORY_RUNAWAY:   render_STATE_STORY_RUNAWAY(); break;
        case STATE_STORY_WHATNOW:   render_STATE_STORY_WHATNOW(); break;
        case STATE_STORY_DONE:   render_STATE_STORY_DONE(); break;
    }
}

/*
 * STATE_ATTRACT
 * dummy state which just hands off immediately to another state
 */
static uint8_t attract_phase = 0;

void enter_STATE_ATTRACT()
{
    state = STATE_ATTRACT;
    statetimer=0;
}


static void tick_STATE_ATTRACT()
{
    ++attract_phase;
    if (attract_phase > 3) {
        attract_phase = 0;
    }
    switch (attract_phase) {
        case 1: enter_STATE_TITLESCREEN(); return;
        case 2: enter_STATE_HIGHSCORES(); return;
        case 3: enter_STATE_GALLERY_BADDIES(); return;
        case 0: enter_STATE_STORY_INTRO(); return;
    }
}

static void render_STATE_ATTRACT()
{
}

/*
 * STATE_TITLESCREEN
 */
void enter_STATE_TITLESCREEN()
{
    state = STATE_TITLESCREEN;
    statetimer=0;
    plat_clr();
}


static void tick_STATE_TITLESCREEN()
{
    uint8_t inp = plat_inp_menu();
    if (inp & (INP_MENU_START|INP_MENU_A)) {
        enter_STATE_NEWGAME();
        return;
    }

    if (++statetimer > 400 || inp & (INP_UP|INP_DOWN|INP_LEFT|INP_RIGHT)) {
        enter_STATE_ATTRACT();
    }
}

// $ tools/png2sprites -w 64 -h 8 -c -fmt mono4x2 logo.png 
static const unsigned char logo_img[] = {
	// img 0 (64x8)
	0xd3, 0x32, 0x4c, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x4c, 0x0d, 0x3e,
	0x7c, 0xc0, 0xf0, 0x5a, 0xf0, 0xcc, 0xc0, 0xcc,
	0x04, 0xc4, 0xcc, 0x8f, 0x78, 0xf5, 0xaf, 0x0f,
	0x00, 0x5a, 0xf0, 0x5a, 0xf5, 0xaf, 0x5a, 0xfd,
	0x2f, 0x5a, 0x5a, 0x0f, 0xd2, 0xf5, 0xaf, 0x0f,
	0xcc, 0xd2, 0x7c, 0x1e, 0xb5, 0xaf, 0x5a, 0xfd,
	0x27, 0xd2, 0x5a, 0x0f, 0x5a, 0x7d, 0x2f, 0x0f,
};


static void render_STATE_TITLESCREEN()
{
//    uint8_t i;
//    for (i=0; i<20; ++i) {
//        plat_text(i,i,"*** TITLE SCREEN ***", ((tick/2)-i) & 0x0f);
//    }
    const uint8_t cx = (SCREEN_TEXT_W-32)/2;
    const uint8_t cy = 4;
    plat_mono4x2(cx,cy, logo_img, 32, 4, (tick/2) & 0x0f);
    plat_text((SCREEN_TEXT_W-22)/2, cy + 6, "(C) 1981 SCUMWAYS INC.", 1);


    uint8_t col1 = (SCREEN_TEXT_W/2) - 5 - 8;
    uint8_t col2 = (SCREEN_TEXT_W/2) - 5 + 8;

    plat_text(col1, 14, "  MOVE: ", 1);
    plat_text(col1, 16, "    W   ", 1);
    plat_text(col1, 17, "  A S D ", 1);

    plat_text(col2, 14, "  FIRE: ", 1);
    plat_text(col2, 16, "   UP   ", 1);
    plat_text(col2, 17, "LF DN RT", 1);

    if ((tick/32) & 0x01) {
        plat_text((SCREEN_TEXT_W-20)/2, 23, "PRESS ENTER TO PLAY", 1);
        plat_text((SCREEN_TEXT_W-20)/2 + 6, 23, "ENTER", 1);
    }

}

/*
 * STATE_NEWGAME
 *
 * dummy state: just set up a new game then go on to next phase.
 */
void enter_STATE_NEWGAME()
{
    // init new game
    level = 0;
    player_lives = 3;
    player_score = 0;
    level_init(level);

    player_create(0);
    state = STATE_NEWGAME;
    statetimer = 0;
}

static void tick_STATE_NEWGAME()
{
    enter_STATE_GETREADY();
}

static void render_STATE_NEWGAME()
{
}


/*
 * STATE_GETREADY
 */
void enter_STATE_GETREADY()
{
    state = STATE_GETREADY;
    statetimer = 0;

    baiter_count = 0;
    baiter_timer = 0;
    // We expect all the gobs to be set up by here.
    // Kick them into spawning-in mode.
    gobs_spawning();
    player_reset(0);
    shot_clearall();
    plat_clr();
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
    gobs_render();
    player_renderall();
    plat_hud(level, player_lives, player_score);
    plat_text(10,10,"GET READY", (tick/2) & 0x0f);
}


/*
 * STATE_PLAY
 */

void enter_STATE_PLAY()
{
    state = STATE_PLAY;
    statetimer = 0;
    plat_clr();
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
    if (gobs_lockcnt == 0 ) {
        enter_STATE_CLEARED();
        return;
    }
    level_baiter_check();
    // CHEAT
    {
        uint8_t cheat = plat_inp_cheat();
        if (cheat & INP_CHEAT_NEXTLEVEL) {
            // skip to next level
            ++level;
            level_init(level);
            enter_STATE_GETREADY();
            return;
        }
        if (cheat & INP_CHEAT_POWERUP) {
            player_powerup(0);
        }
        if (cheat & INP_CHEAT_EXTRALIFE) {
            player_extra_life(0);
        }
    }
    // ENDCHEAT
}

static void render_STATE_PLAY()
{
    gobs_render();
    player_renderall();
    plat_hud(level, player_lives, player_score);
    if (level >= 20  && (tick/16)&0x01) {
        plat_text((SCREEN_TEXT_W-20)/2, 10, "ERROR: TOO MANY FISH", 1);
    }
}


/*
 * STATE_KILLED
 */
void enter_STATE_KILLED()
{
    state = STATE_KILLED;
    statetimer = 0;
    plat_clr();
}

static void tick_STATE_KILLED()
{
    if (++statetimer > 60) {
        if (player_lives > 0 ) {
            --player_lives;
            gobs_reset();
            enter_STATE_GETREADY();
        } else {
            enter_STATE_GAMEOVER();
        }
    }
}

static void render_STATE_KILLED()
{
    gobs_render();
    player_renderall();
    plat_hud(level, player_lives, player_score);
    plat_text(10,10,"OWWWWWWWW", (tick/2) & 0x0f);
}

/*
 * STATE_CLEARED
 */
void enter_STATE_CLEARED()
{
    state = STATE_CLEARED;
    statetimer = 0;
    plat_clr();
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
    gobs_render();
    player_renderall();
    plat_hud(level, player_lives, player_score);
    plat_text(10,10,"CLEAR     ", (tick/2) & 0x0f);
}

/*
 * STATE_GAMEOVER
 */
void enter_STATE_GAMEOVER()
{
    state = STATE_GAMEOVER;
    statetimer = 0;
    plat_clr();
}

static void tick_STATE_GAMEOVER()
{
    if (++statetimer > 60) {
        // enter_STATE_TITLESCREEN();
        enter_STATE_ENTERHIGHSCORE(player_score);
    }
}

static void render_STATE_GAMEOVER()
{
    plat_text(10, 10, "GAME OVER", (tick/2) & 0x0f);
}

/*
 * Game fns
 */

static void level_init(uint8_t level)
{
    gobs_clear();

    switch (level) {
        case 0:
            gobs_create(GK_GRUNT, 10);
            gobs_create(GK_BLOCK, 10);
            gobs_create(GK_MARINE, 5);
            break;
        case 1:
            gobs_create(GK_BLOCK, 4);
            gobs_create(GK_GRUNT, 5);
            gobs_create(GK_FRAGGER, 2);
            gobs_create(GK_MARINE, 3);
            break;
        case 2:
            gobs_create(GK_BLOCK, 5);
            gobs_create(GK_TANK, 1);
            gobs_create(GK_GRUNT, 10);
            gobs_create(GK_MARINE, 3);
            break;
        case 3:
            gobs_create(GK_BLOCK, 12);
            gobs_create(GK_AMOEBA_BIG, 2);
            gobs_create(GK_MARINE, 5);
            break;
        case 4:
            gobs_create(GK_BLOCK, 4);
            gobs_create(GK_FRAGGER, 10);
            gobs_create(GK_HZAPPER, 1);
            gobs_create(GK_MARINE, 3);
            break;
        case 5:
//            gobs_create(GK_BLOCK, 10);
            gobs_create(GK_GRUNT, 20);
            gobs_create(GK_HZAPPER, 1);
            gobs_create(GK_VULGON, 1);
            gobs_create(GK_MARINE, 3);
            break;
        case 6:
            gobs_create(GK_GRUNT, 10);
            gobs_create(GK_HAPPYSLAPPER, 10);
            gobs_create(GK_MARINE, 3);
            break;
        case 7:
            gobs_create(GK_TANK, 10);
            gobs_create(GK_RIFASPAWNER, 5);
            gobs_create(GK_BLOCK, 10);
            gobs_create(GK_MARINE, 5);
            break;
        case 8:
            gobs_create(GK_BLOCK, 30);
            gobs_create(GK_RIFASHARK, 10);
            gobs_create(GK_VULGON, 4);
            gobs_create(GK_MARINE, 3);
            break;
        case 9:
            gobs_create(GK_BLOCK, 15);
            gobs_create(GK_TANK, 2);
            gobs_create(GK_FRAGGER, 10);
            gobs_create(GK_VZAPPER, 1);
            gobs_create(GK_MARINE, 3);
            break;
        case 10:
            gobs_create(GK_HAPPYSLAPPER, 10);
            gobs_create(GK_GRUNT, 10);
            gobs_create(GK_MARINE, 3);
            break;
        case 11:
            gobs_create(GK_BLOCK, 20);
            gobs_create(GK_BRAIN, 1);
            gobs_create(GK_MARINE, 16);
            break;
        case 12:
            gobs_create(GK_FRAGGER, 30);
            gobs_create(GK_MARINE, 5);
            break;
        case 13:
            gobs_create(GK_BLOCK, 15);
            gobs_create(GK_GRUNT, 15);
            gobs_create(GK_AMOEBA_BIG, 2);
            gobs_create(GK_VZAPPER, 1);
            gobs_create(GK_HZAPPER, 1);
            gobs_create(GK_MARINE, 3);
            break;
        case 14:
            gobs_create(GK_HAPPYSLAPPER, 40);
            gobs_create(GK_MARINE, 3);
            break;
        case 15:
            gobs_create(GK_BRAIN, 2);
            gobs_create(GK_AMOEBA_BIG, 1);
            gobs_create(GK_GRUNT, 30);
            gobs_create(GK_VZAPPER, 2);
            gobs_create(GK_MARINE, 8);
            break;
        case 16:
            gobs_create(GK_BLOCK, 30);
            gobs_create(GK_VULGON, 3);
            gobs_create(GK_GRUNT, 10);
            gobs_create(GK_MARINE, 3);
            break;
        case 17:
            gobs_create(GK_BLOCK, 10);
            gobs_create(GK_FRAGGER, 10);
            gobs_create(GK_AMOEBA_BIG, 5);
            gobs_create(GK_VZAPPER, 1);
            gobs_create(GK_HZAPPER, 1);
            gobs_create(GK_MARINE, 3);
            break;
        case 18:
            gobs_create(GK_BLOCK, 10);
            gobs_create(GK_GRUNT, 30);
            gobs_create(GK_HAPPYSLAPPER, 20);
            gobs_create(GK_MARINE, 3);
            break;
        case 19:
            // nails, but good combo :-)
            gobs_create(GK_GRUNT, 10);
            gobs_create(GK_FRAGGER, 10);
            gobs_create(GK_HZAPPER, 2);
            gobs_create(GK_VZAPPER, 2);
            gobs_create(GK_AMOEBA_BIG, 3);
            gobs_create(GK_MARINE, 3);
            break;
        default:
            gobs_create(GK_BAITER, 1);
            gobs_create(GK_MARINE, 3);
            break;
    }
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
                uint8_t b = gob_alloc();
                if (b >= MAX_GOBS) {
                    return;
                }
                // TODO: spawning.
                baiter_create(b);
                gob_randompos(b);
                gobflags[b] |= GF_SPAWNING;
                gobtimer[b] = 8 + (i*8);
            }
        }
    }
}

