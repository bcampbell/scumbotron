// mos-cx16-clang -Os -o game.prg game.c

#include <cx16.h>
#include <stdint.h>

//#define IRQVEC        (*(volatile void* *)0xFFFE)
#define IRQVEC        (*(volatile void* *)0x0314)

extern volatile uint8_t tick;
extern void irq_init();
extern void waitvbl();
extern void inp_tick();

//  .A, byte 0:      | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
//              SNES | B | Y |SEL|STA|UP |DN |LT |RT |
//
//  .X, byte 1:      | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
//              SNES | A | X | L | R | 1 | 1 | 1 | 1 |
extern volatile uint16_t inp_joystate;

extern uint8_t sprites[16*16*16];
extern uint8_t palette[16*2];   // just 16 colours

void testum() {
    VERA.control = 0x00;
    // Writing to 0x1b000 onward.
    VERA.address = 0xB000;
    VERA.address_hi = 0x11; // increment = 1
    uint16_t b = inp_joystate;
    for (uint8_t i=0; i<16; ++i) {
        // char, then colour
        VERA.data0 = (b & 0x8000) ? '.' : 'X';
        VERA.data0 = COLOR_BLACK<<4 | COLOR_GREEN;
        b = b<<1;
    }
}

#define GK_NONE 0
// Player kind
#define GK_PLAYER 0x10
// Shot kinds
#define GK_SHOT 0x20
// Dude kinds
#define GK_BLOCK 0x80
#define GK_GRUNT 0x81


#define MAX_PLAYERS 1
#define MAX_SHOTS 15
#define MAX_DUDES 64

#define FIRST_PLAYER 0
#define FIRST_SHOT (FIRST_PLAYER + MAX_PLAYERS)
#define FIRST_DUDE (FIRST_SHOT + MAX_SHOTS)

#define MAX_GOBS (MAX_PLAYERS + MAX_SHOTS + MAX_DUDES)

// Game objects table.
uint8_t gobkind[MAX_GOBS];
uint16_t gobx[MAX_GOBS];
uint16_t goby[MAX_GOBS];
uint8_t gobdat[MAX_GOBS];


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


void sproff() {
    const int16_t x = -16;
    const int16_t y = -16;
    VERA.data0 = ((0x10000+256)>>5) & 0xFF;
    VERA.data0 = (1 << 7) | ((0x10000+256)>>13);
    VERA.data0 = x & 0xff;  // x lo
    VERA.data0 = (x>>8) & 0x03;  // x hi
    VERA.data0 = y & 0xff;  // y lo
    VERA.data0 = (y>>8) & 0x03;  // y hi
    VERA.data0 = 3<<2; // collmask(4),z(2),vflip,hflip
    VERA.data0 = (1 << 6) | (1 << 4);  // 16x16, 0 palette offset.
}

void sprout(uint8_t d, uint8_t img) {
    int16_t x = gobx[d];
    int16_t y = goby[d];
    VERA.data0 = ((0x10000+256*img)>>5) & 0xFF;
    VERA.data0 = (1 << 7) | ((0x10000+256*img)>>13);
    VERA.data0 = x & 0xff;  // x lo
    VERA.data0 = (x>>8) & 0x03;  // x hi
    VERA.data0 = y & 0xff;  // y lo
    VERA.data0 = (y>>8) & 0x03;  // y hi
    VERA.data0 = 3<<2; // collmask(4),z(2),vflip,hflip
    VERA.data0 = (1 << 6) | (1 << 4);  // 16x16, 0 palette offset.
}

void gobs_render()
{
    VERA.control = 0x00;
    // sprite attrs start at 0x1FC00
    VERA.address = 0xFC00;
    VERA.address_hi = VERA_INC_1 | 0x01; // hi bit = 1

    for (uint8_t d = 0; d < MAX_GOBS; ++d) {
        switch(gobkind[d]) {
            case GK_NONE:
                sproff();
                break;
            case GK_PLAYER:
                sprout(d, 0);
                break;
            case GK_SHOT:
                sprout(d, 1);
                break;
            case GK_BLOCK:
                sprout(d, 2);
                break;
            case GK_GRUNT:
                sprout(d, 3 + ((tick >> 5) & 0x01));
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


// player

void player_tick(uint8_t d) {

    if ((inp_joystate & JOY_UP_MASK) ==0) {
        goby[d] -= 2;
    } else if ((inp_joystate & JOY_DOWN_MASK) ==0) {
        goby[d] += 2;
    }
    if ((inp_joystate & JOY_LEFT_MASK) ==0) {
        gobx[d] -= 2;
    } else if ((inp_joystate & JOY_RIGHT_MASK) ==0) {
        gobx[d] += 2;
    }

    if ((inp_joystate & JOY_BTN_1_MASK) == 0) {
        uint8_t shot = shot_alloc();
        if (shot) {
            gob_init(shot, GK_SHOT, gobx[d], goby[d]);
        }
    }
}

// shot

void shot_tick(uint8_t d)
{
    ++gobdat[d];
    if (gobdat[d] > 30) {
        gobkind[d] = GK_NONE;
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

        for (uint8_t d = FIRST_DUDE + (tick & 0x01); d < (FIRST_DUDE + MAX_DUDES); d=d+2) {
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
            --gobx[d];
        } else if (px > gobx[d]) {
            ++gobx[d];
            ++gobx[d];
            ++gobx[d];
        }
        int16_t py = goby[0];
        if (py < goby[d]) {
            --goby[d];
            --goby[d];
            --goby[d];
        } else if (py > goby[d]) {
            ++goby[d];
            ++goby[d];
            ++goby[d];
        }
    }
}


void display_init()
{
    // screen mode 40x30
    VERA.control = 0x00;    //DCSEL=0
    uint8_t vid = VERA.display.video;
    vid |= 1<<6;    // enable sprites
    vid |= 1<<5; // layer 1 on
    vid &= ~(1<<4);    // layer 1 off
    VERA.display.video = vid;

    VERA.display.hscale = 64;   // 2x scale
    VERA.display.vscale = 64;   // 2x scale
    VERA.control = 0x02;    //DCSEL=1
    VERA.display.hstart = 2;    // 2 to allow some border for rastertiming
    VERA.display.hstop = 640>>2;
    VERA.display.vstart = 0;
    VERA.display.vstop = 480>>1;

    // layer setup
    VERA.layer1.config = 0x10;    // 64x32 tiles, !T256C, !bitmapmode, 1bpp
//    VERA.layer0.mapbase = 
//    VERA.layer0.tilebase = 
    VERA.layer1.hscroll = 0; // 16bit
    VERA.layer1.vscroll = 0; // 16bit

    // load the palette to vram (fixed at 0x1FA00 onward)
    {
        VERA.control = 0x00;
        VERA.address = 0xFA00;
        VERA.address_hi = VERA_INC_1 | 0x01; // hi bit = 1
        const uint8_t* src = palette;
        // just 16 colours
        for (int i=0; i<16*2; ++i) {
            VERA.data0 = *src++;
        }
    }

    // load the sprite images to vram (0x10000 onward)
    {
        VERA.control = 0x00;
        VERA.address = 0x0000;
        VERA.address_hi = VERA_INC_1 | 0x01; // hi bit = 1

        // 16 16*16 images
        const uint8_t* src = sprites;
        for (int i=0; i<16*16*16; ++i) {
            VERA.data0 = *src++;
        }
    }

    // clear the screen (0x1B0000)
    {
        VERA.control = 0x00;
        VERA.address = 0xB000;
        VERA.address_hi = VERA_INC_1 | 0x01; // hi bit = 1

        // 16 16*16 images
        const uint8_t* src = sprites;
        for (int i=0; i<64*32; ++i) {
            VERA.data0 = ' '; // tile
            VERA.data0 = 0; // colour
        }
    }
}

void testlevel()
{
    for( uint8_t y=0; y<8; ++y) {
        for( uint8_t x=0; x<8; ++x) {
            uint8_t d = dude_alloc();
            if (!d) {
                return; // no more free dudes.
            }
            gob_init(d, GK_GRUNT, 4+x*28, 4+y*28);
        }
    }
}


int main(void) {
    display_init();

    irq_init();

    gobs_init();
    gob_init(0, GK_PLAYER, (320/2) + 8, (240/2)+8);
    testlevel();
    while (1) {
        VERA.display.border = 2;
        gobs_render();
        VERA.display.border = 3;
        testum();
        inp_tick();
        VERA.display.border = 15;
        gobs_tick();
        VERA.display.border = 7;
        shot_collisions();
        VERA.display.border = 0;
        waitvbl();
    }
    return 0;
}




