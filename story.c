#include "plat.h"
#include "game.h"
#include "misc.h"


/*
 * STATE_GALLERY_BADDIES
 */
void enter_STATE_GALLERY_BADDIES()
{
    state = STATE_GALLERY_BADDIES;
    statetimer=0;
    plat_clr();
}


void tick_STATE_GALLERY_BADDIES()
{
    uint8_t inp = plat_inp_menu();
    if (inp & INP_MENU_ACTION) {
        enter_STATE_NEWGAME();
        return;
    }

    if (++statetimer > 250 || inp) {
        enter_STATE_GALLERY_GOODIES();
    }
}

void render_STATE_GALLERY_BADDIES()
{

    // Aim to fit within a 256x192 (NDS) size

    // columns
    const uint8_t cellw = 10;
    const uint8_t cx1 = (((SCREEN_W/8) - (cellw*3))/2) + (cellw/2);
    const uint8_t cx2 = cx1 + cellw;
    const uint8_t cx3 = cx2 + cellw;

    // rows
    const uint8_t cellh = 6;    // in chars
    const uint8_t cy1 = 9;
    const uint8_t cy2 = cy1 + cellh;
    const uint8_t cy3 = cy2 + cellh;

    const uint8_t cxtable[9] = {cx1, cx2, cx3, cx1, cx2, cx3, cx1, cx2, cx3};
    const uint8_t cytable[9] = {cy1, cy1, cy1, cy2, cy2, cy2, cy3, cy3, cy3};

    plat_text(cx2 - 4, 1 ,"THE CAST", 1);

    for (uint8_t i = 0; i < 9; ++i) {
        const uint8_t cx = cxtable[i];
        const uint8_t cy = cytable[i];
        const int16_t x = (cx * 8) << FX;
        const int16_t y = ((cy * 8) - 24) << FX;

        const uint8_t c = 15;
        const uint8_t appear = 20 + i*10;
        if (statetimer == (appear-8)) {
            plat_addeffect(x,y,EK_SPAWN);
        }
        if (statetimer < appear) {
            continue;
        }

        switch(i) {
            // row 1
            case 0:
                plat_grunt_render(x - (8 << FX), y); 
                plat_text(cx - 2, cy, "GRUNT", c);
                break;
            case 1:
                plat_block_render(x - (8 << FX), y);
                plat_text(cx - 3, cy1,  "BLOCK", c);
                break;
            case 2:
                plat_fragger_render(x - (8 << FX), y); 
                plat_text(cx - 3, cy, "FRAGGER", c); 
                break;
            // row 2
            case 3:
                plat_amoeba_big_render(x - (16 << FX), y - (8 << FX)); 
                plat_text(cx - 3, cy, "AMOEBA", c);
                break;
            case 4:
                plat_tank_render(x - (8 << FX), y, false);
                plat_text(cx - 2, cy, "TANK", c); 
                break;
            case 5:
                plat_happyslapper_render(x - (8 << FX), y, false); 
                plat_text(cx - 6, cy, "HAPPYSLAPPER", c); 
                break;
            // row 3
            case 6:
                plat_vulgon_render(x - (8 << FX), y, false, 0); 
                plat_text(cx - 3, cy, "VULGON", c); 
                break;
            case 7:
                plat_poomerang_render(x - (8 << FX), y); 
                plat_text(cx - 4, cy, "POOMERANG", c); 
                break;
            case 8:
                plat_hzapper_render(x - (8 << FX), y, 0); 
                plat_text(cx - 3, cy, "ZAPPER", c); 
                break;
        }
    }
}

/*
 * STATE_GALLERY_GOODIES
 */
void enter_STATE_GALLERY_GOODIES()
{
    state = STATE_GALLERY_GOODIES;
    statetimer=0;
    plat_clr();
}


void tick_STATE_GALLERY_GOODIES()
{
    uint8_t inp = plat_inp_menu();
    if (inp & INP_MENU_ACTION) {
        enter_STATE_NEWGAME();
        return;
    }
    if (++statetimer > 250 || inp) {
        enter_STATE_ATTRACT();  // Back to attract mode.
    }
}

void render_STATE_GALLERY_GOODIES()
{

    // Aim to fit within a 256x192 (NDS) size

    // center
    const uint8_t cx = (SCREEN_W/2)/8;

    // rows
    const uint8_t cellh = 6;    // in chars
    const uint8_t cy1 = 9;
    const uint8_t cy2 = cy1 + cellh;
    const uint8_t cy3 = cy2 + cellh;

    //const uint8_t cxtable[8] = {cx1, cx2, cx3, cx1, cx2, cx3, cx1, cx2 };
    const uint8_t cytable[9] = {cy1, cy2, cy3, cy2, cy3, cy3, cy3, cy3, cy3};

    plat_text(cx - 4, 1 ,"THE CAST", 1);

    for (uint8_t i = 0; i < 3; ++i) {
        const uint8_t cy = cytable[i];
        const int16_t x = ((cx * 8) - 8)  << FX;    // -8 for 16x16 spr
        const int16_t y = ((cy * 8) - 24) << FX;


        const uint8_t c = 15;
        const uint8_t appear = 20 + i*30;
        if (statetimer == (appear-8)) {
            plat_addeffect(x,y,EK_SPAWN);
        }
        if (statetimer < appear) {
            continue;
        }

        switch(i) {
            // row 1
            case 0:
                plat_player_render(x, y, DIR_LEFT, true);
                plat_text(cx - 4, cy,  "OUR HERO", c);
                break;
            // row 2 (3 items)
            case 1:
                plat_text(cx - 4, cy, "POWERUPS", c);
                plat_powerup_render(x - (24<<FX), y, 0); 
                plat_powerup_render(x, y, 0); 
                plat_powerup_render(x + (24<<FX), y, 0); 
                break;
            // row 3 (5 items)
            case 2:
                plat_text(cx - 13, cy, "HEAVILY ARMED SPACE MARINES", c); 
                plat_marine_render(x - (48 << FX), y); 
                plat_marine_render(x - (24 << FX), y); 
                plat_marine_render(x + (0 << FX), y); 
                plat_marine_render(x + (24 << FX), y); 
                plat_marine_render(x + (48 << FX), y); 
                break;
        }
    }
}


static void render_robo_rain(uint8_t t) {
    rnd_seed(13);
     int16_t dy1 = (int16_t)t*2;
     int16_t dy2 = (int16_t)t*4;
     int16_t dy3 = (int16_t)t*8;
     int16_t dy4 = (int16_t)t*16;

    for (uint8_t i = 0; i<100; ++i) {
        int16_t x = ((SCREEN_W/2)-128) + ((int16_t)rnd());
        int16_t y = 0 + ((int16_t)rnd())*2;

        //int16_t dx = ((int16_t)t)<<FX;

        uint8_t foo = rnd();
        x += (i+t) & 0x03;

        switch(foo & 0x03) {
            case 0: y += dy1; break;
            case 1: y += dy2; break;
            case 2: y += dy3; break;
            case 3: y += dy4; break;
        }

        x <<= FX;
        y  = ((y & 511) << FX); // wrap around
        y -= 16<<FX;    // start above top border
        switch ((foo>>2) & 0x07) {
            case 0:
            case 1:
            case 2:
                plat_grunt_render(x, y);
                break;
            case 3:
                plat_fragger_render(x, y);
                break;
            case 4:
                plat_amoeba_med_render(x, y);
                break;
            case 5:
                plat_tank_render(x, y, false);
                break;
            case 6:
                plat_happyslapper_render(x, y, false);
                break;
            case 7:
                plat_vulgon_render(x, y, false, 1);
                break;
        }
    }
}

static const uint8_t bluefade[8] = {1,1,1,1,8,9,10,0}; 
static void render_marine_formation(uint16_t t) {
    rnd_seed(53);
    const int16_t s = 24;
    int16_t x = (-5*s);
    x += t*2;
    for (uint8_t col = 0; col < 5; ++col) {
        int16_t y = 70;
        for (uint8_t row = 0; row < 5; ++row) {
            uint8_t j = ((((t>>2) + col) & 3) == 3) ? 2 :0; // jitter
            plat_marine_render(x << FX, (y + -j) << FX);
            y += 24;
        }
        x += s;
    }


    // Show some "hup"s
    const uint8_t cw = SCREEN_W/8;
    const uint8_t hupspacing = 2;
    const uint8_t nhups = (cw-4)/hupspacing;

    uint8_t hx = 0;
    for (uint8_t hup = 0; hup < nhups; ++hup) {
        const uint8_t tduration = 32;
        const uint16_t tbegin = 80 + (hup*8);
        const uint16_t tend = tbegin + tduration;
        uint8_t hy = 7 + ((rnd()&7)<<1) + (hup&1); 
        if (t >= tbegin && t < tend) {
            uint8_t c = bluefade[(t-tbegin)/4];
            plat_text(hx, hy, "HUP",c);
        }
        hx += hupspacing;
    }
}



static void render_marine_disorder(uint8_t t) {
    rnd_seed(53);
    const int16_t s = 24;
    int16_t x = SCREEN_W + 8*s;
    x -= (t*4 + t);
    for (uint8_t col = 0; col < 8; ++col) {
        int16_t y = 120;
        for (uint8_t row = 0; row < 3; ++row) {
            int16_t xjitter = rnd() >> 4;
            int16_t yjitter = rnd() >> 4;
            xjitter += (((col+row+(t>>1)) & 9) == 9) ? 1 : 0;
            yjitter += (((col+row+(t>>1)) & 3) == 3) ? 2 : 0;
            plat_marine_render((x+xjitter) << FX, (y + yjitter) << FX);
            y += 15;
        }
    x += s;
    }

    // Show some "run away!"s
    const uint8_t cw = SCREEN_W/8;
    const uint8_t hupspacing = 5;
    const uint8_t nhups = (cw - 9)/hupspacing;

    uint8_t hx = cw - 9;
    for (uint8_t hup = 0; hup < nhups; ++hup) {
        const uint8_t tduration = 32;
        const uint16_t tbegin = 70 + (hup*8);
        const uint16_t tend = tbegin + tduration;
        uint8_t hy = 14 + ((rnd()&3)<<1) + (hup&3); 
        if (t >= tbegin && t < tend) {
            uint8_t c = bluefade[(t-tbegin)/4];
            plat_text(hx, hy, "RUN AWAY!",c);
        }
        hx -= hupspacing;
    }
}


/*
 * STATE_STORY_INTRO
 */
void enter_STATE_STORY_INTRO()
{
    state = STATE_STORY_INTRO;
    statetimer=0;
    plat_clr();
}


void tick_STATE_STORY_INTRO()
{
    uint8_t inp = plat_inp_menu();
    if (inp & INP_MENU_ACTION) {
        enter_STATE_STORY_DONE();
        return;
    }
    if (++statetimer > 550 || inp) {
        enter_STATE_STORY_OHNO();
    }
}

void render_STATE_STORY_INTRO()
{
    const uint8_t cx = (SCREEN_W/2)/8;
    plat_text(cx - 8, 1, "THE STORY SO FAR", 1);
    if (statetimer > 90) {
        plat_text(cx-8, 8, "IT'S A LOVELY DAY", 1);
    }
    if (statetimer > 150) {
        plat_text(cx-14, 11, "HUMANITY IS JUST SITTING DOWN", 1);
        plat_text(cx-10, 13, "FOR A NICE CUP OF TEA", 1);
    }

    if (statetimer > 220) {
        uint8_t l;
        if (statetimer < 300) {
            l = 5;
        } else if (statetimer < 320) {
            l = 14;
        } else if (statetimer < 340) {
            l = 14 + 1;
        } else if (statetimer < 360) {
            l = 14 + 2;
        } else {
            l = 14 + 3;
        }
        plat_textn(cx-8, 17, "WHEN, SUDDENLY...", l, 1);
    }
}


/*
 * STATE_STORY_OHNO
 */
void enter_STATE_STORY_OHNO()
{
    state = STATE_STORY_OHNO;
    statetimer=0;
    plat_clr();
}


void tick_STATE_STORY_OHNO()
{
    uint8_t inp = plat_inp_menu();
    if (inp & INP_MENU_ACTION) {
        enter_STATE_STORY_DONE();
        return;
    }
    if (++statetimer > 250 || inp) {
        enter_STATE_STORY_ATTACK();
    }
}

void render_STATE_STORY_OHNO()
{
   const uint8_t cx = (SCREEN_W/2)/8;
   plat_text(cx - 11, 14, "ALIEN SCUMBOTS ATTACK!", 15);
   render_robo_rain((uint8_t)(statetimer));
}



/*
 * STATE_STORY_ATTACK
 */
void enter_STATE_STORY_ATTACK()
{
    state = STATE_STORY_ATTACK;
    statetimer=0;
    plat_clr();
}


void tick_STATE_STORY_ATTACK()
{
    uint8_t inp = plat_inp_menu();
    if (inp & INP_MENU_ACTION) {
        enter_STATE_STORY_DONE();
        return;
    }
    if (++statetimer > 340 || inp) {
        enter_STATE_STORY_RUNAWAY();
    }
}
void render_STATE_STORY_ATTACK()
{
    const uint8_t cx = (SCREEN_W/2)/8;
    plat_text(cx - 6, 3, "SEND IN THE", 1);
    const uint16_t appeartime = 40;
    if (statetimer < appeartime) {
        return;
    }
    plat_text(cx - 14, 5, "HEAVILY ARMED SPACE MARINES!", 15); 
    render_marine_formation((uint16_t)(statetimer - appeartime));
}



/*
 * STATE_STORY_RUNAWAY
 */
void enter_STATE_STORY_RUNAWAY()
{
    state = STATE_STORY_RUNAWAY;
    statetimer=0;
    plat_clr();
}


void tick_STATE_STORY_RUNAWAY()
{
    uint8_t inp = plat_inp_menu();
    if (inp & INP_MENU_ACTION) {
        enter_STATE_STORY_DONE();
        return;
    }
    if (++statetimer > 170 || inp) {
        enter_STATE_STORY_DONE();
    }
}


void render_STATE_STORY_RUNAWAY()
{
    render_marine_disorder((uint8_t)statetimer);
}

/*
 * STATE_STORY_DONE
 */
void enter_STATE_STORY_DONE()
{
    state = STATE_STORY_DONE;
    statetimer = 0;
}


void tick_STATE_STORY_DONE()
{
    uint8_t inp = plat_inp_menu();
    if (inp & INP_MENU_ACTION) {
        enter_STATE_NEWGAME();
        return;
    }
    if (++statetimer > 250 || inp) {
        enter_STATE_ATTRACT();  // Back to attract mode.
    }
}


void render_STATE_STORY_DONE()
{
    const uint8_t cx = (SCREEN_W/2)/8;
    plat_text(cx-2, 5, "OH.", 1);
    if (statetimer > 70) {
        plat_text(cx-15, 12, "BUT WHO WILL SAVE HUMANITY NOW?", 1);
    }
}





