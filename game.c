#include "game.h"
#include "gob.h"
#include "highscore.h"
#include "input.h"
#include "misc.h"
#include "plat.h"
#include "player.h"
#include "sfx.h"
#include "vfx.h"

uint8_t state;
uint16_t statetimer;
static uint8_t level;
static uint8_t level_bcd;   // level number as BCD.
static uint8_t baiter_timer;
static uint8_t baiter_count;

static void tick_STATE_TITLESCREEN();
static void tick_STATE_NEWGAME();
static void tick_STATE_ENTERLEVEL();
static void tick_STATE_GETREADY();
static void tick_STATE_PLAY();
static void tick_STATE_CLEARED();
static void tick_STATE_KILLED();
static void tick_STATE_GAMEOVER();
static void tick_STATE_COMPLETE();
static void tick_STATE_PAUSED();

static void render_STATE_TITLESCREEN();
static void render_STATE_NEWGAME();
static void render_STATE_ENTERLEVEL();
static void render_STATE_GETREADY();
static void render_STATE_PLAY();
static void render_STATE_CLEARED();
static void render_STATE_KILLED();
static void render_STATE_GAMEOVER();
static void render_STATE_COMPLETE();
static void render_STATE_PAUSED();

static void level_init(uint8_t level);
static void level_baiter_check();

void game_init()
{

    highscore_init();
    sfx_init();
    vfx_init();
    //enter_STATE_COMPLETE();
    enter_STATE_TITLESCREEN();
}

void game_tick()
{
    inp_tick();
    sfx_tick();
    if (state != STATE_PAUSED) {
        vfx_tick();
    }

    switch(state) {
    case STATE_TITLESCREEN: tick_STATE_TITLESCREEN(); break;
    case STATE_NEWGAME:     tick_STATE_NEWGAME(); break;
    case STATE_ENTERLEVEL:     tick_STATE_ENTERLEVEL(); break;
    case STATE_GETREADY:    tick_STATE_GETREADY(); break;
    case STATE_PLAY:        tick_STATE_PLAY(); break;
    case STATE_CLEARED:     tick_STATE_CLEARED(); break;
    case STATE_KILLED:      tick_STATE_KILLED(); break;
    case STATE_GAMEOVER:    tick_STATE_GAMEOVER(); break;
    case STATE_COMPLETE:    tick_STATE_COMPLETE(); break;
    case STATE_PAUSED:      tick_STATE_PAUSED(); break;
    case STATE_HIGHSCORES:    tick_STATE_HIGHSCORES(); break;
    case STATE_ENTERHIGHSCORE:    tick_STATE_ENTERHIGHSCORE(); break;
    case STATE_GALLERY_BADDIES_1:    tick_STATE_GALLERY_BADDIES_1(); break;
    case STATE_GALLERY_BADDIES_2:    tick_STATE_GALLERY_BADDIES_2(); break;
    case STATE_GALLERY_GOODIES:    tick_STATE_GALLERY_GOODIES(); break;
    case STATE_STORY_INTRO:    tick_STATE_STORY_INTRO(); break;
    case STATE_STORY_OHNO:    tick_STATE_STORY_OHNO(); break;
    case STATE_STORY_ATTACK:    tick_STATE_STORY_ATTACK(); break;
    case STATE_STORY_RUNAWAY:    tick_STATE_STORY_RUNAWAY(); break;
    case STATE_STORY_WHATNOW:    tick_STATE_STORY_WHATNOW(); break;
    }

}

void game_render()
{
    switch(state) {
        case STATE_TITLESCREEN: render_STATE_TITLESCREEN(); break;
        case STATE_NEWGAME:     render_STATE_NEWGAME(); break;
        case STATE_ENTERLEVEL:  render_STATE_ENTERLEVEL(); break;
        case STATE_GETREADY:    render_STATE_GETREADY(); break;
        case STATE_PLAY:        render_STATE_PLAY(); break;
        case STATE_CLEARED:     render_STATE_CLEARED(); break;
        case STATE_KILLED:      render_STATE_KILLED(); break;
        case STATE_GAMEOVER:    render_STATE_GAMEOVER(); break;
        case STATE_COMPLETE:    render_STATE_COMPLETE(); break;
        case STATE_PAUSED:      render_STATE_PAUSED(); break;
        case STATE_HIGHSCORES:    render_STATE_HIGHSCORES(); break;
        case STATE_ENTERHIGHSCORE:  render_STATE_ENTERHIGHSCORE(); break;
        case STATE_GALLERY_BADDIES_1: render_STATE_GALLERY_BADDIES_1(); break;
        case STATE_GALLERY_BADDIES_2: render_STATE_GALLERY_BADDIES_2(); break;
        case STATE_GALLERY_GOODIES: render_STATE_GALLERY_GOODIES(); break;
        case STATE_STORY_INTRO:      render_STATE_STORY_INTRO(); break;
        case STATE_STORY_OHNO:      render_STATE_STORY_OHNO(); break;
        case STATE_STORY_ATTACK:    render_STATE_STORY_ATTACK(); break;
        case STATE_STORY_RUNAWAY:   render_STATE_STORY_RUNAWAY(); break;
        case STATE_STORY_WHATNOW:   render_STATE_STORY_WHATNOW(); break;
    }
    vfx_render();
}

// If player requests pause, enter STATE_PAUSED and return true.
static bool pausemode_check()
{
    // Check for gamepad START or ESC or ENTER key.
    if((inp_gamepad & INP_PAD_START) || (inp_keys & (INP_KEY_ESC | INP_KEY_ENTER))) {
        // Pause records current state and will resume it when unpausing.
        enter_STATE_PAUSED();
        return true;
    }
    return false;
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
    ++statetimer;
    // collect some entropy
    rnd();


    if (inp_keys & INP_KEY_ESC) {
        plat_quit();
        return;
    }

    if ((inp_gamepad & (INP_PAD_START | INP_PAD_A)) ||
        (inp_keys & INP_KEY_ENTER)) {
        enter_STATE_NEWGAME();
        return;
    }

#if 0
    {
        static uint8_t prev = 0;
        uint8_t cur = plat_raw_cheatkeys();
        if (~prev & cur & INP_CHEAT_POWERUP) {
        //    vfx_play_warp();
            sfx_play(SFX_BONUS,2);
            statetimer = 0;
        }
        prev = cur;
    }
#endif

    if (statetimer > 400 || nav_fwd()) {
        enter_STATE_HIGHSCORES();
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
    // title
    {
        const uint8_t cx = (SCREEN_TEXT_W-32)/2;
        const uint8_t cy = 4;
        plat_mono4x2(cx,cy, logo_img, 32, 4, (tick/2) & 0x0f);
        plat_text((SCREEN_TEXT_W-22)/2, cy + 6, "(C) 1984 SCUMWAYS INC.", 1);
    }
    // controls
    {
        const uint8_t col1 = (SCREEN_TEXT_W/2) - 5 - 8;
        const uint8_t col2 = (SCREEN_TEXT_W/2) - 5 + 8;
        const uint8_t cy = 15;
        plat_text(col1, cy, "  MOVE: ", 1);
        plat_text(col1, cy+2, "    W   ", 1);
        plat_text(col1, cy+3, "  A S D ", 1);

        plat_text(col2, cy, "  FIRE: ", 1);
        plat_text(col2, cy+2, "   UP   ", 1);
        plat_text(col2, cy+3, "LF DN RT", 1);

        plat_text((SCREEN_TEXT_W-26)/2, cy+6, "...OR GAMEPAD, OR MOUSE...", 1);
    }

    // start
    {
        uint8_t c = ((tick/32) & 0x01) ? 1:0;
        plat_text((SCREEN_TEXT_W-20)/2, 28, "START/ENTER TO PLAY", c);
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
    level = 1;
    level_bcd = 1;
    player_lives = 3;
    player_score = 0;

    gobs_certainbonus = false;
    gobs_noextralivesforyou = false;

    player_create(0);
    //state = STATE_NEWGAME;
    //statetimer = 0;
    enter_STATE_ENTERLEVEL();
}

static void tick_STATE_NEWGAME()
{
}

static void render_STATE_NEWGAME()
{
}

/*
 * STATE_ENTERLEVEL
 *
 * Set up level, show warp effect.
 */
void enter_STATE_ENTERLEVEL()
{
    plat_clr();
    gobs_clear();
    //player_create(0);
    state = STATE_ENTERLEVEL;
    statetimer = 0;
    vfx_play_warp();
    sfx_play(SFX_WARP, 3);
}

static void tick_STATE_ENTERLEVEL()
{
    if (pausemode_check()) {
        return;
    }

    ++statetimer;
    if (statetimer == 64) {
        // Set up all the bad dudes
        level_init(level);
        enter_STATE_GETREADY();
        return;
    }
}

static void render_STATE_ENTERLEVEL()
{
    uint8_t ch = (SCREEN_TEXT_H/2) - 3;
    uint8_t cw = (SCREEN_TEXT_W-8)/2;
    plat_textn(cw, ch, "LEVEL", 5, (tick/2) & 0x0f);
    uint8_t b = level_bcd;
    char buf[2] = {' ', ' '};
    if (b & 0xf0) {
        buf[0] = '0' + (b>>4);
        buf[1] = '0' + (b & 0x0f);
    } else {
        buf[0] = '0' + (b & 0x0f);
    }
    plat_textn(cw+5+1, ch, buf, 2, (tick/2) & 0x0f);
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
    if (pausemode_check()) {
        return;
    }

    gobs_tick(true);    // spawnphase?
    if(gobs_spawncnt == 0) {
        enter_STATE_PLAY();
        return;
    }
}

static void render_STATE_GETREADY()
{
    gobs_render();
    player_renderall();
    plat_hud(level, player_lives, player_score);
    uint8_t ch = (SCREEN_TEXT_H/2) - 3;
    plat_text((SCREEN_TEXT_W-9)/2, ch, "GET READY", (tick/2) & 0x0f);
}


/*
 * STATE_PLAY
 */

static uint8_t state_play_cleartime;
static bool save_em_all_bonus;
static bool very_clear_bonus;

void enter_STATE_PLAY()
{
    state = STATE_PLAY;
    statetimer = 0;
    state_play_cleartime = 0;
    plat_clr();
    save_em_all_bonus = false;
    very_clear_bonus = false;
}



static void tick_STATE_PLAY()
{
    if (pausemode_check()) {
        return;
    }

    gobs_tick(false);    // not spawnphase
    player_tickall();
    shot_collisions();
    bool playerkilled = player_collisions();
    if (playerkilled) {
        enter_STATE_KILLED();
        return;
    }

    // blasted everything that can be blasted?
    if (gobs_clearablecnt == 0) {
        // Make sure the next thing the player clobbers yields a powerup.
        gobs_certainbonus = true;

        if (!very_clear_bonus) {
            // TODO: play bonus sound effect
            player_add_score(250);
            player_add_score(250);
            very_clear_bonus = true;
        }

    }

    // blasted everything which is keeping the level running?
    if (gobs_lockcnt == 0 ) {
        // saved all the marines?
        if (!save_em_all_bonus &&
            gobs_num_marines == 0 &&
            gobs_num_marines_trailing>0) {
            // TODO: play bonus sound effect
            player_add_score(250);
            player_add_score(250);
            save_em_all_bonus = true; 
        }

        if(++state_play_cleartime > 120) {
            enter_STATE_CLEARED();
        }
        return;
    }
    state_play_cleartime = 0;
    level_baiter_check();
#if ENABLE_CHEATS
    // CHEAT
    {
        uint8_t cheat = inp_cheatkeys;
        if (cheat & INP_CHEAT_NEXTLEVEL) {
            for (uint8_t g; g<MAX_GOBS; ++g) {
                if (gobflags[g] & GF_LOCKS_LEVEL) {
                    gobkind[g] = GK_NONE;
                }
            }
            return;
        }
        if (cheat & INP_CHEAT_POWERUP) {
            player_powerup(0);
            player_powerup(0);
            player_next_weapon(0);
            player_next_weapon(0);
        }
        if (cheat & INP_CHEAT_EXTRALIFE) {
            player_extra_life(0);
        }
    }
#endif  // ENABLE_CHEATS
}

static void render_STATE_PLAY()
{
    gobs_render();
    player_renderall();
    plat_hud(level, player_lives, player_score);
    uint8_t cy = (SCREEN_TEXT_H/2) - 3;
    /*
    if (level >= 20  && (tick/16)&0x01) {
        plat_text((SCREEN_TEXT_W-20)/2, cy, "ERROR: TOO MANY FISH", 1);
    }
    */

    if (gobs_lockcnt == 0 ) {
        uint8_t cx = (SCREEN_TEXT_W-5)/2;
        plat_textn(cx, cy, "CLEAR", 5, (tick/2) & 0x0f);
        ++cy;
        if (very_clear_bonus) {
            cy += 2;
            uint8_t cx = (SCREEN_TEXT_W-31)/2;
            plat_text(cx, cy, "      500 VERY-CLEAR BONUS     ", 1);
        }
        if (save_em_all_bonus) {
            cy += 2;
            uint8_t cx = (SCREEN_TEXT_W-31)/2;
            plat_text(cx, cy, "500 COLLECT-THE-WHOLE-SET BONUS", 1);
        }
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
    if (pausemode_check()) {
        return;
    }
    if (++statetimer > 60) {
        if (player_lives > 0 ) {
            --player_lives;
            gobs_reset();
            enter_STATE_GETREADY();
            return;
        } else {
            enter_STATE_GAMEOVER();
            return;
        }
    }
}

static void render_STATE_KILLED()
{
    gobs_render();
    player_renderall();
    plat_hud(level, player_lives, player_score);
    uint8_t ch = (SCREEN_TEXT_H/2) - 3;
    plat_text((SCREEN_TEXT_W-8)/2, ch, "OWWWWWWWW", (tick/2) & 0x0f);
}

/*
 * STATE_CLEARED
 * Stub state - just sets up next level and goes to STATE_ENTERLEVEL
 */

void enter_STATE_CLEARED()
{
    plat_clr();

    //state = STATE_CLEARED;
    //statetimer = 0;


    if (level == NUM_LEVELS) {
        // Go back to first level
        level = 1;
        level_bcd = 0x01;
        gobs_noextralivesforyou = true;
        // Divert to congratulate player
        enter_STATE_COMPLETE();
        return;
    }

    // next level
    ++level;
    level_bcd = bin2bcd8(level);
    enter_STATE_ENTERLEVEL();
}

static void tick_STATE_CLEARED()
{
}


static void render_STATE_CLEARED()
{
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
    uint8_t ch = (SCREEN_TEXT_H/2) - 3;
    plat_text((SCREEN_TEXT_W-9)/2, ch, "GAME OVER", (tick/2) & 0x0f);
}


/*
 * STATE_COMPLETE
 *
 * NOTE: GAAAAAH! Overran the cx16 38KB BASIC RAM limit just at the last
 * moment. So stubbing out STATE_COMPLETE screen and going straight back
 * into game. Re-evaluate if we can squeeze out a few more bytes...
 * (eg maybe fiddle linker to have BSS section at $0400 or banked ram?)
 */
void enter_STATE_COMPLETE()
{
#if 0
    state = STATE_COMPLETE;
    statetimer = 0;
    plat_clr();
#endif
     enter_STATE_ENTERLEVEL();
}


static void tick_STATE_COMPLETE()
{
#if 0
    ++statetimer;
    if ((tick & 0x7) == 0) {
        int16_t x = ((SCREEN_W/2) + (int8_t)rnd())<<FX;
        int16_t y = ((SCREEN_H/2) + (int8_t)rnd())<<FX;
        if (tick & 0x08) {
            vfx_play_zombify(x,y);
        } else {
            vfx_play_kaboom(x,y);
        }
    }

    if (statetimer > 180) {
        if ((inp_menukeys & INP_MENU_START) ||
            (inp_dualstick & (INP_FIRE_UP|INP_FIRE_RIGHT|INP_FIRE_DOWN|INP_FIRE_LEFT))) {
        enter_STATE_ENTERLEVEL();
        }
    }
#endif
}

static void render_STATE_COMPLETE()
{
#if 0
    uint8_t cy = 8;
    plat_text((SCREEN_TEXT_W-5)/2, cy, "NICE!", (tick/2) & 0x0f);
    if (statetimer > 60) {
        plat_text((SCREEN_TEXT_W-16)/2, cy + 5, "NOW TRY AGAIN...", 1);
    }
    if (statetimer > 120) {
        plat_text((SCREEN_TEXT_W-27)/2, cy + 7, "...WITHOUT ANY EXTRA LIVES.", 1);
    }

    if (statetimer > 180) {
        plat_text((SCREEN_TEXT_W-20)/2, 23, "START/ENTER TO PLAY", 1);
    }
#endif
}





/*
 * STATE_PAUSED
 */

static uint8_t state_paused_prevstate;
static uint8_t state_paused_sel; // 0=resume, 1=quit 0x80=execute!

void enter_STATE_PAUSED()
{
    state_paused_prevstate = state;
    state_paused_sel = 0;
    state = STATE_PAUSED;
}

static void tick_STATE_PAUSED()
{
    if (state_paused_sel & 0x80) {
        switch (state_paused_sel & 0x7f) {
            case 0:
                state = state_paused_prevstate;
                return;
            case 1:
                enter_STATE_TITLESCREEN();
                return;
        }
    }

    if (inp_gamepad & INP_UP || inp_keys & INP_UP) {
        if( state_paused_sel > 0) {
            --state_paused_sel;
        }
    }
    if (inp_gamepad & INP_DOWN || inp_keys & INP_DOWN) {
        if( state_paused_sel < 1) {
            ++state_paused_sel;
        }
    }

    // support mashing escape to quit game 
    if (inp_keys & INP_KEY_ESC)
    {
        if (state_paused_sel == 1) {
            state_paused_sel |= 0x80;   // actually quit
        } else {
            state_paused_sel = 1;   // select quit
        }
    }

    if (inp_gamepad & (INP_PAD_START|INP_PAD_A) || inp_keys & INP_KEY_ENTER) {
        // give the rendering one tick to erase the pause message
        state_paused_sel |= 0x80;
    }
}

static void render_STATE_PAUSED()
{
    // fudge!
    state = state_paused_prevstate;
    //game_render();
    state = STATE_PAUSED;

    // if we're resuming, erase message now!
    uint8_t colour = 0;
    if (!(state_paused_sel & 0x80)) {
        colour = 1; //(tick/2)&0x0f;
    }
    uint8_t cy = 10;
    uint8_t cx = (SCREEN_TEXT_W-10)/2;

    plat_text(cx, cy, "* PAUSED *", colour);
    plat_text(cx-1, cy+2, "  CONTINUE", colour);
    plat_text(cx-1, cy+3, "  QUIT", colour);

    if (!(state_paused_sel & 0x80)) {
        plat_text(cx-1, cy + 2 + (state_paused_sel & 0x7f), ">", (tick>>1) & 0x0F);
    }

}

/*
 * Game fns
 */

static void level_init(uint8_t level)
{
    // handle wrapping 
    while (level > NUM_LEVELS) {
        level -= NUM_LEVELS;
    }
    switch (level) {
        case 1:
            gobs_create(GK_GRUNT, 8);
            gobs_create(GK_MARINE, 5);
            break;
        case 2:
            gobs_create(GK_GRUNT, 5);
            gobs_create(GK_FRAGGER, 2);
            gobs_create(GK_MARINE, 3);
            break;
        case 3:
            gobs_create(GK_BLOCK, 5);
            gobs_create(GK_TANK, 2);
            gobs_create(GK_GRUNT, 10);
            gobs_create(GK_MARINE, 3);
            break;
        case 4:
            gobs_create(GK_BLOCK, 12);
            gobs_create(GK_AMOEBA_BIG, 2);
            gobs_create(GK_MARINE, 5);
            break;
        case 5:
            gobs_create(GK_TANK, 3);
            gobs_create(GK_AMOEBA_BIG, 3);
            gobs_create(GK_BLOCK, 8);
            gobs_create(GK_MARINE, 3);
            break;
        case 6:
            gobs_create(GK_BLOCK, 4);
            gobs_create(GK_HAPPYSLAPPER, 4);
            gobs_create(GK_MARINE, 5);
            break;
        case 7:
            gobs_create(GK_BLOCK, 4);
            gobs_create(GK_FRAGGER, 10);
            gobs_create(GK_HZAPPER, 1);
            gobs_create(GK_MARINE, 3);
            break;
        case 8:
            gobs_create(GK_BLOCK, 10);
            gobs_create(GK_RIFASHARK, 5);
            gobs_create(GK_MARINE, 3);
            break;
        case 9:
            gobs_create(GK_GRUNT, 20);
            gobs_create(GK_HZAPPER, 1);
            gobs_create(GK_VULGON, 1);
            gobs_create(GK_MARINE, 3);
            break;
        case 10:
            gobs_create(GK_BLOCK, 15);
            gobs_create(GK_TANK, 2);
            gobs_create(GK_FRAGGER, 10);
            gobs_create(GK_VZAPPER, 1);
            gobs_create(GK_MARINE, 3);
            break;
        case 11:
            gobs_create(GK_GOBBER, 1);
            gobs_create(GK_BLOCK, 15);
            gobs_create(GK_GRUNT, 15);
            break;
        case 12:
            gobs_create(GK_HAPPYSLAPPER, 10);
            gobs_create(GK_GRUNT, 10);
            gobs_create(GK_MARINE, 3);
            break;
        case 13:
            gobs_create(GK_BRAIN, 1);
            gobs_create(GK_TANK, 4);
            gobs_create(GK_MARINE, 16);
            break;
        case 14:
            gobs_create(GK_RIFASPAWNER, 1);
            gobs_create(GK_GRUNT, 16);
            gobs_create(GK_MARINE, 5);
            break;
        case 15:
            gobs_create(GK_FRAGGER, 30);
            gobs_create(GK_MARINE, 5);
            break;
        case 16:
            gobs_create(GK_BLOCK, 15);
            gobs_create(GK_GRUNT, 15);
            gobs_create(GK_AMOEBA_BIG, 2);
            gobs_create(GK_VZAPPER, 1);
            gobs_create(GK_HZAPPER, 1);
            gobs_create(GK_MARINE, 3);
            break;
        case 17:
            gobs_create(GK_HAPPYSLAPPER, 20);
            gobs_create(GK_MARINE, 3);
            break;
        case 18:
            gobs_create(GK_BRAIN, 2);
            gobs_create(GK_AMOEBA_BIG, 1);
            gobs_create(GK_GRUNT, 16);
            gobs_create(GK_VZAPPER, 2);
            gobs_create(GK_MARINE, 10);
            break;
        case 19:
            gobs_create(GK_FRAGGER, 8);
            gobs_create(GK_GOBBER, 2);
            gobs_create(GK_BLOCK, 8);
            gobs_create(GK_MARINE, 5);
            break;
        case 20:
            gobs_create(GK_BLOCK, 10);
            gobs_create(GK_VULGON, 3);
            gobs_create(GK_GRUNT, 10);
            gobs_create(GK_MARINE, 3);
            break;
        case 21:
            gobs_create(GK_BLOCK, 5);
            gobs_create(GK_FRAGGER, 10);
            gobs_create(GK_AMOEBA_BIG, 3);
            gobs_create(GK_VZAPPER, 1);
            gobs_create(GK_HZAPPER, 1);
            gobs_create(GK_MARINE, 3);
            break;
        case 22:
            gobs_create(GK_BLOCK, 5);
            gobs_create(GK_TANK, 8);
            gobs_create(GK_HAPPYSLAPPER, 16);
            gobs_create(GK_MARINE, 3);
            break;
        case 23:
            gobs_create(GK_GOBBER, 3);
            gobs_create(GK_GRUNT, 12);
            gobs_create(GK_MARINE, 5);
            break;
        case 24:
            gobs_create(GK_TANK, 8);
            gobs_create(GK_RIFASPAWNER, 3);
            gobs_create(GK_BLOCK, 10);
            gobs_create(GK_MARINE, 5);
            break;
        case 25:
            gobs_create(GK_FRAGGER, 30);
            gobs_create(GK_BLOCK, 8);
            gobs_create(GK_MARINE, 5);
            break;
        case 26:
            gobs_create(GK_BRAIN, 3);
            gobs_create(GK_FRAGGER, 8);
            gobs_create(GK_MARINE, 30);
            break;
        case 27:
            gobs_create(GK_GOBBER, 3);
            gobs_create(GK_BLOCK, 5);
            gobs_create(GK_HZAPPER, 1);
            gobs_create(GK_VZAPPER, 1);
            gobs_create(GK_MARINE, 5);
        case 28:
            gobs_create(GK_VULGON, 8);
            gobs_create(GK_BLOCK, 8);
            gobs_create(GK_MARINE, 5);
            break;
        case 29:
            gobs_create(GK_HAPPYSLAPPER, 16);
            gobs_create(GK_TANK, 6);
            gobs_create(GK_MARINE, 5);
            break;
        case 30:
            gobs_create(GK_RIFASHARK, 24);
            gobs_create(GK_FRAGGER, 6);
            gobs_create(GK_MARINE, 5);
            break;
        case 31:
            gobs_create(GK_VULGON, 8);
            gobs_create(GK_VZAPPER, 5);
            gobs_create(GK_BLOCK, 8);
            gobs_create(GK_MARINE, 5);
            break;
        case 32:
            gobs_create(GK_BRAIN, 4);
            gobs_create(GK_RIFASHARK, 8);
            gobs_create(GK_MARINE, 24);
            break;
        case 33:
            // nails, but good combo :-)
            gobs_create(GK_GRUNT, 10);
            gobs_create(GK_FRAGGER, 10);
            gobs_create(GK_HZAPPER, 2);
            gobs_create(GK_VZAPPER, 2);
            gobs_create(GK_AMOEBA_BIG, 3);
            gobs_create(GK_MARINE, 3);
            break;
        case 34:
            gobs_create(GK_TANK, 10);
            gobs_create(GK_RIFASPAWNER, 5);
            gobs_create(GK_MARINE, 5);
            break;
        case 35:
            gobs_create(GK_GOBBER, 4);
            gobs_create(GK_HAPPYSLAPPER, 20);
            gobs_create(GK_MARINE, 5);
            break;
        case 36:
            gobs_create(GK_RIFASHARK, 24);
            gobs_create(GK_MARINE, 5);
            break;
        case 37:
            gobs_create(GK_VZAPPER, 5);
            gobs_create(GK_TANK, 10);
            gobs_create(GK_GRUNT, 10);
            gobs_create(GK_MARINE, 5);
            break;
        case 38:
            gobs_create(GK_RIFASPAWNER, 4);
            gobs_create(GK_FRAGGER, 16);
            gobs_create(GK_MARINE, 5);
            break;
        case 39:
            gobs_create(GK_HAPPYSLAPPER, 8);
            gobs_create(GK_FRAGGER, 8);
            gobs_create(GK_AMOEBA_BIG, 2);
            gobs_create(GK_VULGON, 4);
            gobs_create(GK_MARINE, 5);
            break;
        case 40:
            gobs_create(GK_VULGON, 8);
            gobs_create(GK_GOBBER, 2);
            gobs_create(GK_BRAIN, 2);
            gobs_create(GK_MARINE, 14);
            break;
        case 41:
            gobs_create(GK_RIFASHARK, 20);
            gobs_create(GK_RIFASPAWNER, 4);
            gobs_create(GK_VZAPPER, 2);
            gobs_create(GK_HZAPPER, 2);
            gobs_create(GK_MARINE, 5);
            break;
        case 42:
            gobs_create(GK_BLOCK, 10);
            gobs_create(GK_BAITER, 20);
            gobs_create(GK_MARINE, 5);
            break;
        default:
            gobs_create(GK_BAITER, 30);
            break;
    }
}

static void level_baiter_check()
{
    // update baiters
    if ((tick & 0x0f) == 0x0f) {
        // 16*60 = 960 (about 16 secs)
        // 16*33 = about 9 secs
        uint8_t threshold = (baiter_count == 0) ? 60 : 33;
        ++baiter_timer;
        if (baiter_timer > threshold) {
            uint8_t i;
            baiter_timer = 0;
            if (baiter_count == 0) {
                // just a warning...
                vfx_play_alerttext("HURRY UP!");
                sfx_play(SFX_HURRYUP, 3);
            } else {
                if (baiter_count == 1) {
                    vfx_play_alerttext("LOOK OUT!");
                }
                sfx_play(SFX_LOOKOUT, 3);
                // spawn `em!
                for (i = 0; i < baiter_count; ++i) {
                    uint8_t b = gob_alloc();
                    if (b >= MAX_GOBS) {
                        return;
                    }
                    baiter_create(b);
                    // make these baiters disappear if player dies
                    gobflags[b] &= ~GF_PERSIST;
                    gobflags[b] |= GF_SPAWNING;
                    gobtimer[b] = 8 + (i*8);
                }
            }
            if (baiter_count < 15) {
                ++baiter_count;
            }
        }
    }
}

