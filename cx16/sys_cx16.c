#include <cx16.h>
#include <cbm.h>

#include "../plat.h"
#include "../gob.h" // for ZAPPER_*
#include "../misc.h"
#include "../gfx_vera.h"

// Platform-specifics for cx16

// To break:
// asm ("stp");

// irq.s
// Joystick 0 is the built-into-the-kernal keyboard joystick,
// but we don't like the mapping, so we'll do our own keyboard-driven
// joystick!
extern void irq_init();
extern void keyhandler_init();
extern uint8_t inp_keystates[16];
extern void waitvbl();
extern void inp_enabletextentry();
extern void inp_disabletextentry();

static void update_inp_mouse();

// start PLAT_HAS_MOUSE
int16_t plat_mouse_x = 0;
int16_t plat_mouse_y = 0;
uint8_t plat_mouse_buttons = 0;
static uint8_t mouse_watchdog = 0; // >0 = active
// end PLAT_HAS_MOUSE

static void cx16_init();

// Unsupported
void plat_quit()
{
    // Could exit back out to basic?
}

void cx16_init()
{
    irq_init();

    // Ensure uppercase+PETSCII charset.
    // 2 = c64 style
    // 4 = thin, pet-style?
    cx16_k_screen_set_charset(4, 0);

    // Init mouse
    cx16_k_mouse_config(0xff, SCREEN_W/8, SCREEN_H/8);

    // It's assumed the exported graphics data is already loaded into VRAM.
    // The loader stub (scumbotron.asm) decompresses all the graphics data
    // into VRAM before our main() even gets called.
    // See gfx_vera.h for the expected VRAM layout.
    // In theory we could load the graphics from disk here instead.
    gfx_vera_init();
}


/*
 * psg sfx support
 */

#define VRAM_PSG 0x1F9C0
void plat_psg(uint8_t chan, uint16_t freq, uint8_t vol, uint8_t waveform, uint8_t pulsewidth)
{
    uint32_t addr = VRAM_PSG + chan * 4;
    VERA.control = 0x00;
    VERA.address = ((addr)&0xffff);
    VERA.address_hi = (VERA_INC_1) | (((addr)>>16)&1);

    VERA.data0 = freq & 0xff;
    VERA.data0 = freq >> 8;
    VERA.data0 = (3 << 6) | vol;  // lrvvvvvv
    VERA.data0 = (waveform << 6) | pulsewidth;         // wwpppppp
}


static inline bool inp_keypressed(uint8_t key) {
    return inp_keystates[key / 8] & (1 << (key & 0x07));
}

// some keycodes for r43+. (previous roms used ps/2 multibye sequences) 
// see https://github.com/X16Community/x16-rom/blob/master/inc/keycode.inc
#define KEYCODE_W 0x12
#define KEYCODE_A 0x1F
#define KEYCODE_S 0x20
#define KEYCODE_D 0x21
#define KEYCODE_ENTER 0x2B
#define KEYCODE_LEFTARROW 0x4F
#define KEYCODE_UPARROW 0x53
#define KEYCODE_DOWNARROW 0x54
#define KEYCODE_RIGHTARROW 0x59
#define KEYCODE_ESC 0x6E
#define KEYCODE_BACKSPACE 0x0F
#define KEYCODE_HOME 0x50
#define KEYCODE_END 0x50

#define KEYCODE_F1 0x70
#define KEYCODE_F2 0x71
#define KEYCODE_F3 0x72
#define KEYCODE_F4 0x73
#define KEYCODE_F5 0x74


static void update_inp_mouse()
{
    mouse_pos_t m;
    uint8_t mb = cx16_k_mouse_get(&m);
    int16_t mx = m.x << FX;
    int16_t my = m.y << FX;

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


uint8_t plat_raw_dualstick()
{
    uint8_t raw = 0;

    // Keys pretending to be a joypad.
    // (cx16 provides a key-driven joypad as joystick 0, but the mapping
    // is no good for twin-stick style controls)
    if (inp_keypressed(KEYCODE_W)) { raw |= INP_UP; }
    if (inp_keypressed(KEYCODE_A)) { raw |= INP_LEFT; }
    if (inp_keypressed(KEYCODE_S)) { raw |= INP_DOWN; }
    if (inp_keypressed(KEYCODE_D)) { raw |= INP_RIGHT; }
    if (inp_keypressed(KEYCODE_LEFTARROW)) { raw |= INP_FIRE_LEFT; }
    if (inp_keypressed(KEYCODE_UPARROW)) { raw |= INP_FIRE_UP; }
    if (inp_keypressed(KEYCODE_DOWNARROW)) { raw |= INP_FIRE_DOWN; }
    if (inp_keypressed(KEYCODE_RIGHTARROW)) { raw |= INP_FIRE_RIGHT; }

    // Read the first _real_ joypad, if plugged in
    JoyState j1 = cx16_k_joystick_get(1);
    if (!(j1.data0 & JOY_UP_MASK)) {raw |= INP_UP;}
    if (!(j1.data0 & JOY_DOWN_MASK)) {raw |= INP_DOWN;}
    if (!(j1.data0 & JOY_LEFT_MASK)) {raw |= INP_LEFT;}
    if (!(j1.data0 & JOY_RIGHT_MASK)) {raw |= INP_RIGHT;}
    if (!(j1.data0 & JOY_BTN_B_MASK)) {raw |= INP_FIRE_DOWN;}
    if (!(j1.data0 & JOY_BTN_A_MASK)) {raw |= INP_FIRE_RIGHT;}

    if (!(j1.data1 & JOY_BTN_X_MASK)) {raw |= INP_FIRE_UP;}
    if (!(j1.data1 & JOY_BTN_Y_MASK)) {raw |= INP_FIRE_LEFT;}
    return raw;
}

uint8_t plat_raw_gamepad()
{
    uint8_t state = 0;
    JoyState j1 = cx16_k_joystick_get(1);
    if (!(j1.data0 & JOY_UP_MASK)) {state |= INP_UP;}
    if (!(j1.data0 & JOY_DOWN_MASK)) {state |= INP_DOWN;}
    if (!(j1.data0 & JOY_LEFT_MASK)) {state |= INP_LEFT;}
    if (!(j1.data0 & JOY_RIGHT_MASK)) {state |= INP_RIGHT;}
    if (!(j1.data0 & JOY_BTN_A_MASK)) {state |= INP_PAD_A;}
    if (!(j1.data0 & JOY_BTN_B_MASK)) {state |= INP_PAD_B;}
    if (!(j1.data0 & JOY_START_MASK)) {state |= INP_PAD_START;}
    
    return state;
}

uint8_t plat_raw_keys()
{
    uint8_t state = 0;

    if (inp_keypressed(KEYCODE_UPARROW)) { state |= INP_UP; }
    if (inp_keypressed(KEYCODE_LEFTARROW)) { state |= INP_LEFT; }
    if (inp_keypressed(KEYCODE_DOWNARROW)) { state |= INP_DOWN; }
    if (inp_keypressed(KEYCODE_RIGHTARROW)) { state |= INP_RIGHT; }
    if (inp_keypressed(KEYCODE_ENTER)) { state |= INP_KEY_ENTER; }
    if (inp_keypressed(KEYCODE_ESC)) { state |= INP_KEY_ESC; }
    
    return state;
}


uint8_t plat_raw_cheatkeys()
{
    uint8_t state = 0;
    if (inp_keypressed(KEYCODE_F1)) { state |= INP_CHEAT_POWERUP; }
    if (inp_keypressed(KEYCODE_F2)) { state |= INP_CHEAT_EXTRALIFE; }
    if (inp_keypressed(KEYCODE_F3)) { state |= INP_CHEAT_NEXTLEVEL; }
    return state;
}


/*
void debug_getin()
{

    static char buf[40] = {0};
    static uint8_t n = 0;

    while (1) {
        char c = plat_textentry_getchar();
        if (c == 0) {
            break;
        }
        if (c==0x0a) {
            c = 'X';
        }
        if (c==0x7f) {
            // del
            if (n>0) {
                --n;
                buf[n] = 0;
            }
            continue;
        }
        if (n<40) {
            buf[n++] = c;
        }
    }
    plat_textn(0,23,buf,n,2);
}
*/

#ifdef PLAT_HAS_TEXTENTRY

void plat_textentry_start()
{
    inp_enabletextentry();
}

void plat_textentry_stop()
{
    inp_disabletextentry();
}

char plat_textentry_getchar()
{
    char c = cbm_k_getin();
    if (c==0) {
        return 0;
    }
    // map keys DEL to ASCII DEL
    if (c == 0x14) {
        return 0x7F;
    }
    // map LEFT to ASCII non-destructive backspace
    if (c == 0x9D ) {
        return '\b';
    }

    // map CR to LF
    if (c==0x0D) {
        return '\n';
    }

    // Strip out shifted petscii graphics.
    c = c & ~0x80;

    // suppress other control codes.
    if (c < 32) {   // || (c >= 0x80 && c < 0xa0)) {
        return 0;
    }
    return c;
}

#endif // PLAT_HAS_TEXTENTRY
 
bool plat_savescores(const void* begin, int nbytes)
{
    cbm_k_setnam("@:SCUMBOSCORES"); // "@:" to allow overwriting
    cbm_k_setlfs(1,8,1);
    // copy into banked ram before saving, just so 2-byte header will be 0xa000.
    // (at time of writing bsave isn't in llvm-mos-sdk, so we'll just pretend
    // the 0xa000 is a magic cookie instead ;-)
    void* tmp = (void*)0xA000;
    cx16_k_memory_copy((void*)begin, tmp, nbytes);
    // cbm_k_save prepends the 2byte address header :-(
    char result = cbm_k_save((void*)tmp, (void*)(tmp + nbytes));
    if (result != 0) {
        return false;
    }
    return true;
}

bool plat_loadscores(void* begin, int nbytes)
{
    cbm_k_setnam("SCUMBOSCORES");
    cbm_k_setlfs(1,8,0);    // 0=ignore header, 2= headerless load
    // load into banked RAM for safety.
    void* tmp = (void*)0xA000;
    void* result = cbm_k_load(0, tmp);

    // if return is < 256 then it's an error code, not the end address
    if ((uint16_t)result <= 255) {

        // 4 = file not found, that's ok. might be first run.
        if ((uint16_t)result == 4) {
            // Clear the flashing drive light
            // https://commanderx16.com/forum/viewtopic.php?p=27600#p27600
            cbm_k_listen(8);
            cbm_k_second(15);
            cbm_k_ciout('I');
            cbm_k_unlsn();
        }
        return false;   // didn't load.
    }
    cx16_k_memory_copy(tmp, begin, nbytes);
    return true;
}


int main(void) {

    cx16_init();
    game_init();
    while(1) {
        waitvbl();
        gfx_vera_render_start();
        game_render();
        if (mouse_watchdog > 0) {
            sprout16(plat_mouse_x, plat_mouse_y, 0);
        }
        gfx_vera_render_finish();
        update_inp_mouse();
        game_tick();
    }
}

