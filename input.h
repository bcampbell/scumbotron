#ifndef SCUMBOTRON_INPUT_H
#define SCUMBOTRON_INPUT_H

#include <stdint.h>

#include "plat.h"

// The current state of the dual-stick input
// 4 bits of INP_DIR_* and 4 bits of INP_DIR_FIRE_*
extern uint8_t inp_dualstick;

// Which gamepad buttons have been pressed since the last frame?
extern uint8_t inp_gamepad;

// Which keybaord keys have been presses since the last frame?
extern uint8_t inp_keys;

// Which cheatkeys have been pressed since the last frame?
extern uint8_t inp_cheatkeys;

void inp_tick();

#endif  // SCUMBOTRON_INPUT_H

