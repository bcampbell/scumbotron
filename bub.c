#include "gob.h"
#include "sys.h"
#include "misc.h"


// Current gob being targeted, 0xff=none.
static uint8_t bub_targ;
// Time remaining until next bubble check.
static uint8_t bub_timer;

void bub_clear()
{
    bub_timer = rnd();
    bub_targ = 0xff;
}


static bool issuitable(uint8_t g)
{
    if (gobkind[g] == GK_GRUNT) {
        return true;
    }
    // Marines only use speech bubbles when not following player.
    if (gobkind[g] == GK_MARINE && (gobflags[g] & GF_COLLIDES_PLAYER)) {
        return true;
    }
    return false;
}


static void bub_new()
{
    uint8_t targ = rnd();
    if (targ < MAX_GOBS) {
        if (issuitable(targ)) {
            bub_targ = targ;
            // Grunt bubbles last twice as long.
            bub_timer = (gobkind[targ] == GK_GRUNT) ? 200 : 100;
            return;
        }
    }
    bub_targ = 0xff;
    bub_timer = 2;
}

void bub_tick()
{
    if (bub_timer == 0) {
        bub_new();
    } else {
        --bub_timer;
        if (bub_targ != 0xff && !issuitable(bub_targ)) {
            // Stop the speech bubble if dude was killed.
            bub_targ = 0xff;
            bub_timer = 2;
        }
    }
}

void bub_render()
{
    if (bub_targ == 0xff) {
        return;
    }

    uint16_t x = gobx[bub_targ] + (8<<FX);
    uint16_t y = goby[bub_targ] - (9<<FX);
    switch (gobkind[bub_targ]) {
        case GK_GRUNT:
            // 2-phase message for grunts.
            sys_bub_render(x, y, (bub_timer > 100) ? 0 : 1);
            break;
        case GK_MARINE:
            // 2 alternatives for marine. Use gobindex to decide.
            sys_bub_render(x, y, 2 + (bub_targ & 0x01));
            break;
    }
}


