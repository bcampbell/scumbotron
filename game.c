// mos-cx16-clang -Os -o game.prg game.c

#include <stdint.h>

// from sys
extern void sys_init();
extern void testum();
extern void waitvbl();
extern void inp_tick();
extern void sys_render_start();
extern void sproff();
extern void sprout(int16_t x, int16_t y, uint8_t img);
extern void sys_render_finish();

extern void sys_clr();
extern void sys_text(uint8_t cx, uint8_t cy, const char* txt, uint8_t colour);

extern volatile uint16_t inp_joystate;
extern volatile uint8_t tick;


// core game stuff.
#define SCREEN_W 320
#define SCREEN_H 240


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



#define GK_NONE 0
// Player kind
#define GK_PLAYER 0x10
// Shot kinds
#define GK_SHOT 0x20
// Dude kinds
#define GK_BLOCK 0x80
#define GK_GRUNT 0x81


#define MAX_PLAYERS 1
#define MAX_SHOTS 7
#define MAX_DUDES 40

#define FIRST_PLAYER 0
#define FIRST_SHOT (FIRST_PLAYER + MAX_PLAYERS)
#define FIRST_DUDE (FIRST_SHOT + MAX_SHOTS)

#define MAX_GOBS (MAX_PLAYERS + MAX_SHOTS + MAX_DUDES)

// Game objects table.
uint8_t gobkind[MAX_GOBS];
uint16_t gobx[MAX_GOBS];
uint16_t goby[MAX_GOBS];
uint8_t gobdat[MAX_GOBS];
uint8_t gobtimer[MAX_GOBS];


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

void gobs_render()
{
    for (uint8_t d = 0; d < MAX_GOBS; ++d) {
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

// hackhackhack
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

        for (uint8_t d = FIRST_DUDE /*+ (tick & 0x01)*/; d < (FIRST_DUDE + MAX_DUDES); d=d+1) {
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
        sys_text(0,12,"*** TITLE SCREEN ***", tick & 0x0f);
        sys_text(0,14,"FIRE TO START", tick & 0x01);
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
    for( uint8_t i=0; i<MAX_DUDES; ++i) {
            uint8_t d = dude_alloc();
            if (!d) {
                return; // no more free dudes.
            }
            gobkind[d] = GK_GRUNT;
            gobdat[d] = 0;
            gobtimer[d] = 0;
            dude_randompos(d);
    }
}

void level() {
    sys_clr();
    gobs_init();
    gob_init(0, GK_PLAYER, (SCREEN_W/2) - 8, (SCREEN_H/2)-8);
    testlevel();
    while (1) {
        sys_render_start();
        //VERA.display.border = 2;
        gobs_render();
        testum();
        sys_render_finish();

        //VERA.display.border = 3;
        inp_tick();
        //VERA.display.border = 15;
        gobs_tick();
        //VERA.display.border = 7;
        shot_collisions();

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
