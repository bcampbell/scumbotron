// mos-cx16-clang -Os -o game.prg game.c

#include <stdint.h>
#include <stdbool.h>

#include "sys_cx16.h"

// by darsie,
// https://www.avrfreaks.net/forum/tiny-fast-prng
uint8_t rnd() {
        static uint8_t s=0xaa,a=0;
        s^=s<<3;
        s^=s>>5;
        s^=a++>>2;
        return s;
}

#define DIR_UP 0x08
#define DIR_DOWN 0x04
#define DIR_LEFT 0x02
#define DIR_RIGHT 0x01

//
// Game object (gob) Stuff
//

#define GK_NONE 0
// Player kind
#define GK_PLAYER 0x10
// Shot kinds
#define GK_SHOT 0x20
// Dude kinds
#define GK_BLOCK 0x80
#define GK_GRUNT 0x81


// Gob tables.
#define MAX_PLAYERS 1
#define MAX_SHOTS 7
#define MAX_DUDES 40

#define FIRST_PLAYER 0
#define FIRST_SHOT (FIRST_PLAYER + MAX_PLAYERS)
#define FIRST_DUDE (FIRST_SHOT + MAX_SHOTS)

#define MAX_GOBS (MAX_PLAYERS + MAX_SHOTS + MAX_DUDES)

uint8_t gobkind[MAX_GOBS];
uint16_t gobx[MAX_GOBS];
uint16_t goby[MAX_GOBS];
uint8_t gobdat[MAX_GOBS];
uint8_t gobtimer[MAX_GOBS];


uint8_t dude_alloc();
uint8_t shot_alloc();
void dude_randompos(uint8_t d);

void spawning_tick(uint8_t g);
void player_tick(uint8_t d);
void shot_tick(uint8_t d);
void grunt_tick(uint8_t d);


void gobs_init()
{
    for (uint8_t i = 0; i < MAX_GOBS; ++i) {
        gobkind[i] = GK_NONE;
    }
}

void gobs_tick()
{
    for (uint8_t i = 0; i < MAX_GOBS; ++i) {
        switch(gobkind[i]) {
            case GK_NONE:
                break;
            case GK_PLAYER:
                player_tick(i);
                break;
            case GK_SHOT:
                shot_tick(i);
                break;
            case GK_BLOCK:
                break;
            case GK_GRUNT:
                grunt_tick(i);
                break;
            default:
                break;
        }
    }
}

void gobs_render(uint8_t nspawned)
{
    for (uint8_t d = 0; d < nspawned; ++d) {
        switch(gobkind[d]) {
            case GK_NONE:
                sproff();
                break;
            case GK_PLAYER:
                sprout(gobx[d], goby[d], 0);
                break;
            case GK_SHOT:
                sprout(gobx[d], goby[d], 1);
                break;
            case GK_BLOCK:
                sprout(gobx[d], goby[d], 2);
                break;
            case GK_GRUNT:
                sprout(gobx[d], goby[d],  3 + ((tick >> 5) & 0x01));
                break;
            default:
                sproff();
                break;
        }
    }
}

void dudes_spawn(uint8_t kind, uint8_t n)
{
    while(n--) {
        uint8_t g = dude_alloc();
        if (!g) {
            return;
        }
        gobkind[g] = kind;
        gobdat[g] = 0;
        gobtimer[g] = 0;
        dude_randompos(g);
    }
}



void dudes_respawn() {
    for (uint8_t g = FIRST_DUDE; g < FIRST_DUDE+MAX_DUDES; ++g) {
        if (gobkind[g] == GK_NONE) {
            continue;
        }
        gobtimer[g] = 0;
        dude_randompos(g);
    }
}



void gob_init(uint8_t d, uint8_t kind, int x, int y) {
    gobkind[d] = kind;
    gobx[d] = x;
    goby[d] = y;
    gobdat[d] = 0;
    gobtimer[d] = 0;
}

// returns 0 if none free.
uint8_t shot_alloc() {
    for(uint8_t d = MAX_PLAYERS; d < MAX_PLAYERS + MAX_SHOTS; ++d) {
        if(gobkind[d] == GK_NONE) {
            return d;
        }
    }
    return 0;
}

// returns 0 if none free.
uint8_t dude_alloc() {
    for(uint8_t d = MAX_PLAYERS+MAX_SHOTS; d < MAX_GOBS; ++d) {
        if(gobkind[d] == GK_NONE) {
            return d;
        }
    }
    return 0;
}

void dude_randompos(uint8_t d) {
    const int16_t xmid = (SCREEN_W/2)-8;
    const int16_t ymid = (SCREEN_H/2)-8;

    int16_t x;
    int16_t y;
    while (1) {
        x = -128 + rnd();
        y = -128 + rnd();

        int16_t lsq = x*x + y*y;
        if (lsq > 80*80 && lsq <100*100 ) {
            break;
        }
    }

    gobx[d] = xmid + x;
    goby[d] = ymid + y;
}

// hackhackhack (from cx16.h)
#define JOY_UP_MASK 0x08
#define JOY_DOWN_MASK 0x04
#define JOY_LEFT_MASK 0x02
#define JOY_RIGHT_MASK 0x01
#define JOY_BTN_1_MASK 0x80


// player
#define PLAYER_SPD 1

void player_tick(uint8_t d) {
    uint8_t dir = 0;
    if ((inp_joystate & JOY_UP_MASK) ==0) {
        dir |= DIR_UP;
        goby[d] -= PLAYER_SPD;
    } else if ((inp_joystate & JOY_DOWN_MASK) ==0) {
        dir |= DIR_DOWN;
        goby[d] += PLAYER_SPD;
    }
    if ((inp_joystate & JOY_LEFT_MASK) ==0) {
        dir |= DIR_LEFT;
        gobx[d] -= PLAYER_SPD;
    } else if ((inp_joystate & JOY_RIGHT_MASK) ==0) {
        dir |= DIR_RIGHT;
        gobx[d] += PLAYER_SPD;
    }

    if ((inp_joystate & JOY_BTN_1_MASK) == 0) {
        if (!gobdat[d]) {
            gobdat[d] = dir;
        }

        uint8_t shot = shot_alloc();
        if (shot) {
            gobkind[shot] = GK_SHOT;
            gobx[shot] = gobx[d];
            goby[shot] = goby[d];
            gobdat[shot] = gobdat[d];   // direction
            gobtimer[shot] = 16;
        }
    } else {
        if (dir) {
            gobdat[d] = dir;
        }
    }
}

// shot
// dat: dir
// timer: dies at 0

#define SHOT_SPD 8
void shot_tick(uint8_t s)
{
    if (--gobtimer[s] == 0) {
        gobkind[s] = GK_NONE;
        return;
    }
    uint8_t dir = gobdat[s];
    if (dir & DIR_UP) {
        goby[s] -= SHOT_SPD;
    } else if (dir & DIR_DOWN) {
        goby[s] += SHOT_SPD;
    }
    if (dir & DIR_LEFT) {
        gobx[s] -= SHOT_SPD;
    } else if (dir & DIR_RIGHT) {
        gobx[s] += SHOT_SPD;
    }
}

void shot_collisions()
{
    for (uint8_t s = FIRST_SHOT; s < (FIRST_SHOT + MAX_SHOTS); ++s) {
        if (gobkind[s] == GK_NONE) {
            continue;
        }
        // Take centre point of shot.
        int16_t sx = gobx[s] + 8;
        int16_t sy = goby[s] + 8;

        for (uint8_t d = FIRST_DUDE; d < (FIRST_DUDE + MAX_DUDES); d=d+1) {
            if (gobkind[d]==GK_NONE) {
                continue;
            }
            int16_t dx = gobx[d];
            int16_t dy = goby[d];
            if (sx >= dx && sx < (dx + 16) && sy >= dy && sy < (dy + 16)) {
                // boom.
                gobkind[d] = GK_NONE;
                gobkind[s] = GK_NONE;
                break;  // next shot.
            }
        }
    }
}


static inline bool overlap(uint16_t amin, uint16_t amax, uint16_t bmin, uint16_t bmax)
{
    return (amin <= bmax) && (amax >= bmin);
}

void player_collisions()
{
    for (uint8_t p = FIRST_PLAYER; p < (FIRST_PLAYER + MAX_PLAYERS); ++p) {
        if (gobkind[p] != GK_PLAYER) {
            continue;
        }
        for (uint8_t d = FIRST_DUDE; d < (FIRST_DUDE + MAX_DUDES); d=d+1) {
            if (gobkind[d]==GK_NONE) {
                continue;
            }
            if (overlap(gobx[p]+4, gobx[p]+12, gobx[d], gobx[d]+16) &&
                overlap(goby[p]+4, goby[p]+12, goby[d], goby[d]+16)) {
                // boom
                //gobkind[d] = GK_NONE;
                dudes_respawn();
                return;
                //gamestate = 0;
            }
        }
    }
}



// spawning-in effect

void spawning_tick(uint8_t g)
{
    if(--gobtimer[g] == 0) {
        // Turn it into real dude.
        gobkind[g] = gobdat[g];
        gobdat[g] = 0;
        gobtimer[g] = 0;
    }
}


// grunt
void grunt_tick(uint8_t d)
{
    gobdat[d]++;
    if (gobdat[d]>13) {
        gobdat[d] = 0;
        int16_t px = gobx[0];
        if (px < gobx[d]) {
            --gobx[d];
            --gobx[d];
        } else if (px > gobx[d]) {
            ++gobx[d];
            ++gobx[d];
        }
        int16_t py = goby[0];
        if (py < goby[d]) {
            --goby[d];
            --goby[d];
        } else if (py > goby[d]) {
            ++goby[d];
            ++goby[d];
        }
    }
}

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
        //testum();
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
    uint8_t spawncnt = 0;
    uint8_t spawntimer = 0;

    sys_clr();
    gobs_init();
    gob_init(0, GK_PLAYER, (SCREEN_W/2) - 8, (SCREEN_H/2)-8);
    testlevel();
    while (1) {
        ++statetimer;
        switch (state) {
            case LEVELSTATE_GETREADY:
                sys_text(10,10,"GET READY", tick & 0x07);
                if(statetimer>=2) {
                    statetimer = 0;
                    // skip empty slots.
                    while(++spawncnt <MAX_GOBS && gobkind[spawncnt] == GK_NONE) {
                    }
                    // all spawned?
                    if (spawncnt==MAX_GOBS) {
                        state = LEVELSTATE_PLAY;
                        sys_text(10,10,"GET READY", 0); // clear
                    }
                };
                break;
            case LEVELSTATE_PLAY:
                break;
            case LEVELSTATE_CLEARED:
                if (statetimer > 30) {
                    state = LEVELSTATE_COMPLETE;
                    statetimer = 0;
                }
                break;
            case LEVELSTATE_KILLED:
                if (statetimer > 30) {
                    state = LEVELSTATE_GETREADY;
                    statetimer = 0;
                }
                break;
        }


        sys_render_start();
        for (uint8_t g=spawncnt; g<MAX_GOBS; ++g) {
            if (gobkind[g] != GK_NONE) {
                sys_spawneffect(gobx[g], goby[g], g*4 + tick);
            }
        }
        gobs_render(spawncnt);
        sys_render_finish();

        testum();

        inp_tick();

        if (state != LEVELSTATE_GETREADY) {
            gobs_tick();
            shot_collisions();
            player_collisions();

            // count remaining dudes
            uint8_t cnt = 0;
            for (uint8_t i = FIRST_DUDE; i<FIRST_DUDE+MAX_DUDES; ++i ) {
                if (gobkind[i] != GK_NONE) {
                    ++cnt;
                }
            }
            if (cnt==0) {
                break;
            }

            char foo[2] = {'A'+cnt, '\0'};
            sys_text(0,1,foo, tick&0x0f);
        }

        //VERA.display.border = 0;
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
