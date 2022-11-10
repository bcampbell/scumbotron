#include "dudes.h"
#include "gob.h"

// block

void block_init(uint8_t d)
{
    gobkind[d] = GK_BLOCK;
    gobflags[d] = 0;
    gobx[d] = 0;
    goby[d] = 0;
    gobvx[d] = 0;
    gobvy[d] = 0;
    gobdat[d] = 0;
    gobtimer[d] = 0;
}


// grunt

void grunt_init(uint8_t d)
{
    gobkind[d] = GK_GRUNT;
    gobflags[d] = 0;
    gobx[d] = 0;
    goby[d] = 0;
    gobvx[d] = (2 << FX) + (rnd() & 0x7f);
    gobvy[d] = (2 << FX) + (rnd() & 0x7f);
    gobdat[d] = 0;
    gobtimer[d] = 0;
}


void grunt_tick(uint8_t d)
{
    // update every 16 frames
    if (((tick+d) & 0x0f) != 0x00) {
       return;
    }
    int16_t px = gobx[0];
    if (px < gobx[d]) {
        gobx[d] -= gobvx[d];
    } else if (px > gobx[d]) {
        gobx[d] += gobvx[d];
    }
    int16_t py = goby[0];
    if (py < goby[d]) {
        goby[d] -= gobvy[d];
    } else if (py > goby[d]) {
        goby[d] += gobvy[d];
    }
}


// baiter
void baiter_init(uint8_t d)
{
    gobkind[d] = GK_BAITER;
    gobflags[d] = 0;
    gobx[d] = 0;
    goby[d] = 0;
    gobvx[d] = 0;
    gobvy[d] = 0;
    gobdat[d] = 0;
    gobtimer[d] = 0;
}

void baiter_tick(uint8_t d)
{
    const int16_t BAITER_MAX_SPD = 4 << FX;
    const int16_t BAITER_ACCEL = 2;
    int16_t px = gobx[0];
    if (px < gobx[d] && gobvx[d] > -BAITER_MAX_SPD) {
        gobvx[d] -= BAITER_ACCEL;
    } else if (px > gobx[d] && gobvx[d] < BAITER_MAX_SPD) {
        gobvx[d] += BAITER_ACCEL;
    }
    gobx[d] += gobvx[d];

    int16_t py = goby[0];
    if (py < goby[d] && gobvy[d] > -BAITER_MAX_SPD) {
        gobvy[d] -= BAITER_ACCEL;
    } else if (py > goby[d] && gobvy[d] < BAITER_MAX_SPD) {
        gobvy[d] += BAITER_ACCEL;
    }
    goby[d] += gobvy[d];

}

// amoeba
void amoeba_init(uint8_t d)
{
    gobkind[d] = GK_AMOEBA_BIG;
    gobflags[d] = 0;
    gobx[d] = 0;
    goby[d] = 0;
    gobvx[d] = 0;
    gobvy[d] = 0;
    gobdat[d] = 0;
    gobtimer[d] = 0;
}

void amoeba_tick(uint8_t d)
{
    // jitter acceleration
    gobvx[d] += (rnd() - 128)>>2;
    gobvy[d] += (rnd() - 128)>>2;


    // apply drag
    int16_t vx = gobvx[d];
    gobvx[d] = (vx >> 1) + (vx >> 2);
    int16_t vy = gobvy[d];
    gobvy[d] = (vy >> 1) + (vy >> 2);
    
    const int16_t AMOEBA_MAX_SPD = (4<<FX)/3;
    const int16_t AMOEBA_ACCEL = 2 * FX_ONE / 2;

    gobx[d] += gobvx[d];
    goby[d] += gobvy[d];

    // update homing every 16 frames
    if (((tick+d) & 0x0f) != 0x00) {
       return;
    }
    int16_t px = gobx[0];
    if (px < gobx[d] && gobvx[d] > -AMOEBA_MAX_SPD) {
        gobvx[d] -= AMOEBA_ACCEL;
    } else if (px > gobx[d] && gobvx[d] < AMOEBA_MAX_SPD) {
        gobvx[d] += AMOEBA_ACCEL;
    }

    int16_t py = goby[0];
    if (py < goby[d] && gobvy[d] > -AMOEBA_MAX_SPD) {
        gobvy[d] -= AMOEBA_ACCEL;
    } else if (py > goby[d] && gobvy[d] < AMOEBA_MAX_SPD) {
        gobvy[d] += AMOEBA_ACCEL;
    }



}

void amoeba_spawn(uint8_t kind, int16_t x, int16_t y, int16_t vx, int16_t vy) {
    uint8_t d = dude_alloc();
    if (!d) {
        return;
    }
    gobkind[d] = kind;
    gobx[d] = x;
    goby[d] = y;
    gobvx[d] = vx;
    gobvy[d] = vy;
    gobdat[d] = 0;
    gobtimer[d] = 0;
}

void amoeba_shot(uint8_t d)
{
    if (gobkind[d] == GK_AMOEBA_SMALL) {
        gobkind[d] = GK_NONE;
        return;
    }

    if (gobkind[d] == GK_AMOEBA_MED) {
        const int16_t v = FX<<5;
        gobkind[d] = GK_NONE;
        amoeba_spawn(GK_AMOEBA_SMALL, gobx[d], goby[d], -v, v);
        amoeba_spawn(GK_AMOEBA_SMALL, gobx[d], goby[d], v, v);
        amoeba_spawn(GK_AMOEBA_SMALL, gobx[d], goby[d], 0, -v);
        return;
    }

    if (gobkind[d] == GK_AMOEBA_BIG) {
        const int16_t v = FX<<5;
        gobkind[d] = GK_NONE;
        amoeba_spawn(GK_AMOEBA_MED, gobx[d], goby[d], -v, v);
        amoeba_spawn(GK_AMOEBA_MED, gobx[d], goby[d], v, v);
        amoeba_spawn(GK_AMOEBA_MED, gobx[d], goby[d], 0, -v);
        return;
    }
}
