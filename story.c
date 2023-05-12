#include "plat.h"
#include "game.h"
//#include "misc.h"


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
    if (++statetimer > 250 || plat_inp_menu()) {
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

    plat_text(cx2 - 4, 2 ,"THE CAST", 1);

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
    if (++statetimer > 250 || plat_inp_menu()) {
        enter_STATE_TITLESCREEN();
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

    plat_text(cx - 4, 2 ,"THE CAST", 1);

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

