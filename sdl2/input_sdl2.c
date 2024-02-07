#include "../plat.h"
#include <SDL.h>

// If a game controller is available, this will be set.
static SDL_GameController* theController = NULL;
int16_t plat_mouse_x = 0;
int16_t plat_mouse_y = 0;
uint8_t plat_mouse_buttons = 0;
static uint8_t mouse_watchdog = 0; // >0 = active

void controller_init()
{
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            theController = SDL_GameControllerOpen(i);
            return;
        }
    }
}

void controller_shutdown()
{
    if (theController) {
        SDL_GameControllerClose(theController);
        theController = NULL;
    }
}


// Called in response to SDL_CONTROLLERDEVICEADDED event.
void controller_added(int id)
{
    if (theController) {
        return; // Ignore. We've already got a controller.
    }
    if (!SDL_IsGameController(id)) {
        return;
    }
    theController = SDL_GameControllerOpen(id);
}

// Called in response to SDL_CONTROLLERDEVICEREMOVED event.
void controller_removed(SDL_JoystickID joyid)
{
    SDL_GameController *affected = SDL_GameControllerFromInstanceID(joyid);
    if (affected == theController) {
        SDL_GameControllerClose(theController);
        theController = NULL;
    }
}


uint8_t plat_raw_dualstick()
{
    uint8_t state = 0;

    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    static const struct {
        uint8_t scancode;
        uint8_t bitmask;
    } keymapping[] = {
        {SDL_SCANCODE_UP, INP_FIRE_UP},
        {SDL_SCANCODE_DOWN, INP_FIRE_DOWN},
        {SDL_SCANCODE_LEFT, INP_FIRE_LEFT},
        {SDL_SCANCODE_RIGHT, INP_FIRE_RIGHT},
        {SDL_SCANCODE_W, INP_UP},
        {SDL_SCANCODE_S, INP_DOWN},
        {SDL_SCANCODE_A, INP_LEFT},
        {SDL_SCANCODE_D, INP_RIGHT},
        {0,0},
    };

    for (int i = 0; keymapping[i].bitmask != 0; ++i) {
        if (keys[keymapping[i].scancode]) {
            state |= keymapping[i].bitmask;
        }
    }

    // Early-out if no controller plugged in.
    if (!theController) {
        return state;
    }

    // Gamepad button mapping.
    static const struct {
        SDL_GameControllerButton button;
        uint8_t bitmask;
    } buttonmapping[] = {
        {SDL_CONTROLLER_BUTTON_DPAD_UP, INP_UP},
        {SDL_CONTROLLER_BUTTON_DPAD_DOWN, INP_DOWN},
        {SDL_CONTROLLER_BUTTON_DPAD_LEFT, INP_LEFT},
        {SDL_CONTROLLER_BUTTON_DPAD_RIGHT, INP_RIGHT},
        {SDL_CONTROLLER_BUTTON_Y, INP_FIRE_UP},
        {SDL_CONTROLLER_BUTTON_A, INP_FIRE_DOWN},
        {SDL_CONTROLLER_BUTTON_X, INP_FIRE_LEFT},
        {SDL_CONTROLLER_BUTTON_B, INP_FIRE_RIGHT},
        {SDL_CONTROLLER_BUTTON_INVALID, 0},
    };

    for (int i = 0; buttonmapping[i].bitmask != 0; ++i) {
        if (SDL_GameControllerGetButton(theController, buttonmapping[i].button)) {
            state |= buttonmapping[i].bitmask;
        }
    }
    return state;
}

uint8_t plat_raw_gamepad()
{
    static const struct {
        SDL_GameControllerButton button;
        uint8_t bitmask;
    } map[] = {
        {SDL_CONTROLLER_BUTTON_DPAD_UP, INP_UP},
        {SDL_CONTROLLER_BUTTON_DPAD_DOWN, INP_DOWN},
        {SDL_CONTROLLER_BUTTON_DPAD_LEFT, INP_LEFT},
        {SDL_CONTROLLER_BUTTON_DPAD_RIGHT, INP_RIGHT},
        {SDL_CONTROLLER_BUTTON_A, INP_PAD_A},
        {SDL_CONTROLLER_BUTTON_B, INP_PAD_B},
        {SDL_CONTROLLER_BUTTON_START, INP_PAD_START},
        {SDL_CONTROLLER_BUTTON_INVALID, 0},
    };

    uint8_t state = 0;
    for (int i = 0; map[i].bitmask != 0; ++i) {
        if (SDL_GameControllerGetButton(theController, map[i].button)) {
            state |= map[i].bitmask;
        }
    }
    return state;
}

uint8_t plat_raw_keys()
{
    uint8_t state = 0;
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    uint8_t i;
    static const struct {
        uint8_t scancode;
        uint8_t bitmask;
    } mapping[6] = {
        {SDL_SCANCODE_UP, INP_UP},
        {SDL_SCANCODE_DOWN, INP_DOWN},
        {SDL_SCANCODE_LEFT, INP_LEFT},
        {SDL_SCANCODE_RIGHT, INP_RIGHT},
        {SDL_SCANCODE_RETURN, INP_KEY_ENTER},
        {SDL_SCANCODE_ESCAPE, INP_KEY_ESC},
    };
   
    for (i = 0; i < 6; ++i) {
        if (keys[mapping[i].scancode]) {
            state |= mapping[i].bitmask;
        }
    }
    return state;
}

uint8_t plat_raw_cheatkeys()
{
    uint8_t state = 0;
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    if (keys[SDL_SCANCODE_F1]) {
        state |= INP_CHEAT_POWERUP;
    }
    if (keys[SDL_SCANCODE_F2]) {
        state |= INP_CHEAT_EXTRALIFE;
    }
    if (keys[SDL_SCANCODE_F3]) {
        state |= INP_CHEAT_NEXTLEVEL;
    }
    return state;
}


void mouse_update(SDL_Renderer* rr)
{
    int x,y;
    Uint32 b = SDL_GetMouseState(&x, &y);
    uint8_t mb = 0;
    if (b & SDL_BUTTON(SDL_BUTTON_LEFT)) {
        mb |= 0x01;
    }
    if (b & SDL_BUTTON(SDL_BUTTON_RIGHT)) {
        mb |= 0x02;
    }
    if (b & SDL_BUTTON(SDL_BUTTON_MIDDLE)) {
        mb |= 0x04;
    }

    float lx, ly;
    SDL_RenderWindowToLogical(rr, x,y, &lx, &ly);
    int16_t mx = (int16_t)(lx * (float)FX_ONE);
    int16_t my = (int16_t)(ly * (float)FX_ONE);

    if (mb != 0 || mx != plat_mouse_x || my != plat_mouse_y) {
        plat_mouse_buttons = mb;
        plat_mouse_x = mx;
        plat_mouse_y = my;
        mouse_watchdog = 60;
    } else {
       if (mouse_watchdog > 0) {
           --mouse_watchdog;
       }
    }
}

// Show the mouse pointer (maybe).
void mouse_render()
{
    if (mouse_watchdog > 0) {
        sprout16(plat_mouse_x, plat_mouse_y, 0);
    }
}


#ifdef PLAT_HAS_TEXTENTRY

static char textentry_buf[16] = {0};
static size_t textentry_n = 0;

void textentry_put(char c)
{
    if (textentry_n < sizeof(textentry_buf)) {
        textentry_buf[textentry_n++] = c;
    }
}

void plat_textentry_start()
{
    SDL_StartTextInput();
    textentry_n = 0;    // clear buffer
}

void plat_textentry_stop()
{
    SDL_StopTextInput();
}

char plat_textentry_getchar()
{
    char c = '\0';
    if (textentry_n > 0) {
        --textentry_n;
        c = textentry_buf[textentry_n];
    }
    return c;
}

#endif // PLAT_HAS_TEXTENTRY

