#include "input.h"

uint8_t inp_dualstick = 0;
uint8_t inp_gamepad = 0;
uint8_t inp_keys = 0;
uint8_t inp_cheatkeys = 0;

static uint8_t dualstick_prevraw = 0;
static uint8_t gamepad_prevraw = 0;
static uint8_t keys_prevraw = 0;
static uint8_t cheatkeys_prevraw = 0;

// Dejam an input axis, where the player might press conflicting
// directions simultaneously. In such cases, the most recently-pressed
// direction should take precedent.
// params:
// mask - The opposing direction bits we're handling (eg INP_DIR_UP|INP_DIR_DOWN)
// raw - The raw input bits, possibly conflicting.
// prevraw - Raw from previous frame.
// state - The dejammed state from the previous frame.
// returns the new state bits (masked by mask)
static uint8_t dejam(uint8_t mask, uint8_t raw, uint8_t prevraw, uint8_t state)
{
    raw &= mask;
    if (raw != mask) {
        // No conflict - just use raw input.
        return raw;
    }

    uint8_t pressed = raw & (~prevraw);
    if (pressed) {
        // A new direction was pressed since last time - use it.
        return pressed;
    }
    // No change - use previous direction.
    return state & mask;
}



void inp_tick()
{
    // update dualstick input
    {
        // read the raw input (and the previous value)
        uint8_t raw = plat_raw_dualstick();
        uint8_t prevraw = dualstick_prevraw;

        // Make sure opposing-directions-at-the-same-time doesn't gum things up.
        // This can happen a lot when using keys.
        uint8_t prevstate = inp_dualstick;
        inp_dualstick = dejam(INP_UP|INP_DOWN, raw, prevraw, prevstate) |
            dejam(INP_LEFT|INP_RIGHT, raw, prevraw, prevstate) |
            dejam(INP_FIRE_UP|INP_FIRE_DOWN, raw, prevraw, prevstate) |
            dejam(INP_FIRE_LEFT|INP_FIRE_RIGHT, raw, prevraw, prevstate);
        dualstick_prevraw = raw;
    }

    // gamepad update
    {
        uint8_t raw = plat_raw_gamepad();
        // Which ones were pressed since last check?
        inp_gamepad = (~gamepad_prevraw) & raw;
        gamepad_prevraw = raw;
    }

    // keyboard update
    {
        uint8_t raw = plat_raw_keys();
        // Which ones were pressed since last check?
        inp_keys = (~keys_prevraw) & raw;
        keys_prevraw = raw;
    }

    // cheatkeys update
    {   
        uint8_t raw = plat_raw_cheatkeys();
        inp_cheatkeys = (~cheatkeys_prevraw) & raw;
        cheatkeys_prevraw = raw;
    }
}




#if defined(PLAT_FAKE_DUALSTICK)

// Faked dualstick using plat_raw_gamepad(), and button A to lock
// fire direction.
// Returns direction + FIRE_ bits.
static uint8_t firelock = 0;    // fire bits if locked (else 0)
static uint8_t facing = 0;  // last non-zero direction

uint8_t plat_raw_dualstick()
{
    uint8_t pad = plat_raw_gamepad();

    uint8_t out = pad & (INP_UP|INP_DOWN|INP_LEFT|INP_RIGHT);
    if (out != 0) {
        facing = out;
    }

    if (pad & INP_PAD_A) {
        if (!firelock) {
            firelock = (facing<<4);
        }
        out |= firelock;
    } else {
        firelock = 0;
    }

    return out;
}

#endif

