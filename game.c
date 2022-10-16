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



typedef struct dude {
    int8_t kind;
    int8_t dat;
    int16_t x;
    int16_t y;
} dude;

// Dude flags
#define DF_LIVE 0x01


// Dude Kinds
#define DK_NONE 0
#define DK_PLAYER 1
#define DK_SHOT 2
#define DK_BLOCK 3
#define DK_GRUNT 4

#define MAX_DUDES 80

dude dudes[MAX_DUDES];


void player_tick(dude* d);
void shot_tick(dude* d);
void grunt_tick(dude* d);


void dudes_init()
{
    for (uint8_t i = 0; i < MAX_DUDES; ++i) {
        dude* d = &dudes[i];
        d->kind = DK_NONE;
    }
}

void dudes_tick()
{
    for (uint8_t i = 0; i < MAX_DUDES; ++i) {
        dude* d = &dudes[i];
        switch(d->kind) {
            case DK_NONE:
                break;
            case DK_PLAYER:
                player_tick(d);
                break;
            case DK_SHOT:
                shot_tick(d);
                break;
            case DK_BLOCK:
                break;
            case DK_GRUNT:
                grunt_tick(d);
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

void sprout(int16_t x, int16_t y, uint8_t idx) {
    VERA.data0 = ((0x10000+256*idx)>>5) & 0xFF;
    VERA.data0 = (1 << 7) | ((0x10000+256*idx)>>13);
    VERA.data0 = x & 0xff;  // x lo
    VERA.data0 = (x>>8) & 0x03;  // x hi
    VERA.data0 = y & 0xff;  // y lo
    VERA.data0 = (y>>8) & 0x03;  // y hi
    VERA.data0 = 3<<2; // collmask(4),z(2),vflip,hflip
    VERA.data0 = (1 << 6) | (1 << 4);  // 16x16, 0 palette offset.
}

void dudes_render()
{
    VERA.control = 0x00;
    // sprite attrs start at 0x1FC00
    VERA.address = 0xFC00;
    VERA.address_hi = VERA_INC_1 | 0x01; // hi bit = 1

    for (uint8_t i = 0; i < MAX_DUDES; ++i) {
        dude* d = &dudes[i];
        switch(d->kind) {
            case DK_NONE:
                sproff();
                break;
            case DK_PLAYER:
                sprout(d->x, d->y, 0);
                break;
            case DK_SHOT:
                sprout(d->x, d->y, 1);
                break;
            case DK_BLOCK:
                sprout(d->x, d->y, 2);
                break;
            case DK_GRUNT:
                sprout(d->x, d->y, 3 + (tick >> 2) & 0x03);
                break;
            default:
                sproff();
                break;
        }
    }
}

void dude_init(dude* d, uint8_t kind, int x, int y) {
    d->kind = kind;
    d->x = x;
    d->y = y;
    d->dat = 0;
}

dude* dude_alloc() {
    for(uint8_t i=0; i<MAX_DUDES; ++i) {
        if(dudes[i].kind == DK_NONE) {
            return &dudes[i];
        }
    }
    return 0;
}


// player

void player_tick(dude* d) {

    if ((inp_joystate & JOY_UP_MASK) ==0) {
        d->y -= 2;
    } else if ((inp_joystate & JOY_DOWN_MASK) ==0) {
        d->y += 2;
    }
    if ((inp_joystate & JOY_LEFT_MASK) ==0) {
        d->x -= 2;
    } else if ((inp_joystate & JOY_RIGHT_MASK) ==0) {
        d->x += 2;
    }

    if ((inp_joystate & JOY_BTN_1_MASK) == 0) {
        dude* shot = dude_alloc();
        if (shot) {
            dude_init(shot, DK_SHOT, d->x, d->y);
        }
    }
}

// shot
void shot_tick(dude* d)
{
    ++d->dat;
    if (d->dat>30) {
        d->kind = DK_NONE;
    }
}

// grunt
void grunt_tick(dude* d)
{
    d->dat++;
    if (d->dat>13) {
        d->dat = 0;
        dude* plr = &dudes[0];
        if (plr->x < d->x) {
            --d->x;
        } else if (plr->x > d->x) {
            ++d->x;
        }
        if (plr->y < d->y) {
            --d->y;
        } else if (plr->y > d->y) {
            ++d->y;
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

int main(void) {
    display_init();

    irq_init();

    dudes_init();
    dude_init(&dudes[0], DK_PLAYER, (320/2) + 8, (240/2)+8);

    uint8_t i=1;
    for( uint8_t y=0; y<8; ++y) {
        for( uint8_t x=0; x<8; ++x) {
            dude_init(&dudes[i++], DK_GRUNT, 4+x*38,4+y*38);
        }
    }
    while (1) {
        VERA.display.border = 2;
        dudes_render();
        VERA.display.border = 3;
        testum();
        inp_tick();
        VERA.display.border = 15;
        dudes_tick();
        VERA.display.border = 0;
        waitvbl();
    }
    return 0;
}




