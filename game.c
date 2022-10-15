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

void testum() {
    VERA.control = 0x00;
    // Writing to 0x1b000 onward.
    VERA.address = 0xB000;
    VERA.address_hi = 0x11; // increment = 1
    uint16_t b = inp_joystate;
    for (uint8_t i=0; i<16; ++i) {
        // char, then colour
        VERA.data0 = (b & 0x8000) ? 'X' : '.';
        VERA.data0 = COLOR_BLACK<<4 | COLOR_GREEN;
        b = b<<1;
    }
}


void loadsprites() {
    VERA.control = 0x00;
    // Writing to 0x10000 onward.
    VERA.address = 0x0000;
    VERA.address_hi = 0x11; // increment = 1

    // 16 16*16 images
    const uint8_t* p = sprites;
    for (int i=0; i<16*16*16; ++i) {
        VERA.data0 = *p++;
    }
}

void showsprite(int16_t x, int16_t y, uint8_t i, uint8_t off) {
    VERA.control = 0x00;
    // sprite attrs start at 0x1FC00
    VERA.address = 0xFC00 + (i*8);
    VERA.address_hi = VERA_INC_1 | 0x01; // hi bit = 1

    VERA.data0 = ((0x10000+256*i)>>5) & 0xFF;
    VERA.data0 = (1 << 7) | ((0x10000+256*i)>>13);
    VERA.data0 = x & 0xff;  // x lo
    VERA.data0 = (x>>8) & 0x03;  // x hi
    VERA.data0 = y & 0xff;  // y lo
    VERA.data0 = (y>>8) & 0x03;  // y hi
    VERA.data0 = 3<<2; // collmask(4),z(2),vflip,hflip
    VERA.data0 = (1 << 6) | (1 << 4);  // 64x64, 0 palette offset.
}


void bg(uint8_t r, uint8_t g, uint8_t b) {
    VERA.control = 0x00;
    // sprite attrs start at 0x1FC00
    VERA.address = 0xFA00;
    VERA.address_hi = VERA_INC_1 | 0x01; // hi bit = 1

    VERA.data0 = (g & 0xf0) | (b>>4);
    VERA.data0 = (r>>4);
}


int16_t obx[16] = {0};
int16_t oby[16] = {0};



#define INP_B 0x80
#define INP_Y 0x40
#define INP_SELECT 0x20
#define INP_START 0x10
#define INP_UP 0x08
#define INP_DOWN 0x04
#define INP_LEFT 0x02
#define INP_RIGHT 0x01

void tick_player(uint8_t ob) {

    if ((inp_joystate & INP_UP) ==0) {
        oby[ob] -= 4;
    } else if ((inp_joystate & INP_DOWN) ==0) {
        oby[ob] += 4;
    }
    if ((inp_joystate & INP_LEFT) ==0) {
        obx[ob] -= 4;
    } else if ((inp_joystate & INP_RIGHT) ==0) {
        obx[ob] += 4;
    }
}


int main(void) {
    // a little border on the left for rastertiming
    VERA.control = 0x02;    //DCSEL=1
    VERA.display.hstart=2;
    VERA.control = 0x00;

    irq_init();
    testum();
    loadsprites();

    // ENABLE SPRITES DAMMIT!!!
    VERA.display.video |= 0x40;

    int16_t x = 0;
    int16_t y = 0;
    uint8_t i=0;
    uint8_t off=0;
    while (1) {
        VERA.display.border = 2;
        for (i=0; i<16; ++i) {
            showsprite(obx[i], oby[i], i, off);
        }
        VERA.display.border = 1;
        testum();
        inp_tick();
        tick_player(0);
        VERA.display.border = 0;
        waitvbl();
    }
    return 0;
}




