#include "plat.h"
#include "game.h"
#include "misc.h"
#include "highscore.h"

highscore highscore_table[HIGHSCORE_COUNT];

// for highscore entry - slot and position of focused char.
static uint8_t entry_slot;
static uint8_t entry_cursor;

void highscore_init()
{
    uint32_t score = 10000;
    for (uint8_t i = 0; i < HIGHSCORE_COUNT; ++i) {
        highscore* h = &highscore_table[i];
        h->score = score;
        score -= 1000;
        h->name[0] = 'A';
        h->name[1] = 'C';
        h->name[2] = 'E';
        for (uint8_t j = 3; j<HIGHSCORE_NAME_SIZE; ++j) {
            h->name[j] = ' ';
        }
    }
}


static void draw() {
    uint8_t slot;
    char buf[8];
    const uint8_t cx = (SCREEN_TEXT_W - (HIGHSCORE_NAME_SIZE + 2 + 8))/2; 
    const uint8_t cy = 4;
    for (slot = 0; slot<HIGHSCORE_COUNT; ++slot) {
        highscore* hs = &highscore_table[slot];
        // Don't flash the entry being entered
        uint8_t c;
        if (slot == entry_slot) {
            c = 1;
        } else {
            c = ((tick+slot)/2) & 0x0f;
        }
        // Draw the name.
        plat_textn(cx, cy + slot*2, hs->name, HIGHSCORE_NAME_SIZE, c);
        // Draw cursor
        if (slot == entry_slot && entry_cursor < HIGHSCORE_NAME_SIZE) {
            // draw the cursor.
            int16_t x = ((cx + entry_cursor) * 8 - 4) << FX;
            int16_t y = ((cy + (slot * 2)) * 8 - 4) << FX;
            plat_cursor_render(x, y);
            //plat_textn(cx + entry_cursor, cy + (slot*2) - 1, "v", 1, (tick>>3) & 7);
            //plat_textn(cx + entry_cursor, cy + (slot*2) + 1, "^", 1, (tick>>3) & 7);
        }

        // Draw the score (8 digits).
        hex32(bin2bcd32(hs->score), buf);
        plat_textn(cx + 2 + HIGHSCORE_NAME_SIZE, cy + slot*2, buf, 8, c );
    }
}



// returns position for new score, HIGHSCORE_COUNT if not on score table.
static uint8_t pick_slot(uint32_t newscore)
{
    uint8_t slot;
    for (slot = 0; slot < HIGHSCORE_COUNT; ++slot) {
        if (newscore > highscore_table[slot].score) {
            break;    // it's a highscore!
        }
    }
    return slot;
}

static void insert_new_score(uint8_t slot, uint32_t score)
{
    uint8_t j;
    uint8_t i = HIGHSCORE_COUNT-1;

    for (i = HIGHSCORE_COUNT - 1; i > slot; --i) {
        highscore_table[i] = highscore_table[i-1];
    }
    // Insert new score with clear name.
    highscore* hs = &highscore_table[slot];
    hs->score = score;
    for (j = 0; j < HIGHSCORE_NAME_SIZE; ++j) {
        hs->name[j] = ' ';
    }
}

/*
 * STATE_HIGHSCORES
 */
void enter_STATE_HIGHSCORES()
{
    state = STATE_HIGHSCORES;
    statetimer = 0;
    // for drawing
    entry_slot = HIGHSCORE_COUNT;
    entry_cursor = HIGHSCORE_NAME_SIZE;
    plat_clr();
}

void tick_STATE_HIGHSCORES()
{
    uint8_t inp = plat_inp_menu();
    if (inp & (INP_MENU_START | INP_MENU_A)) {
        enter_STATE_NEWGAME();
        return;
    }

    if (++statetimer > 300 || inp) {
        enter_STATE_ATTRACT();  // back to attract state
    }
}

void render_STATE_HIGHSCORES()
{
    const uint8_t cx = (SCREEN_TEXT_W/2);
    plat_text(cx-5, 1, "HIGH SCORES", 1);
    draw();
}


/*
 * STATE_ENTERHIGHSCORE
 */
void enter_STATE_ENTERHIGHSCORE(uint32_t score)
{
    state = STATE_ENTERHIGHSCORE;
    statetimer = 0;
    plat_clr();
    entry_slot = pick_slot(score);
    entry_cursor = 0;
    if (entry_slot >= HIGHSCORE_COUNT) {
        // not a high score. bail out.
        enter_STATE_HIGHSCORES();
        return;
    }
    insert_new_score(entry_slot, score);
    //names[entry_slot][0] = 'A';

#ifdef PLAT_HAS_TEXTENTRY
    plat_textentry_start();
#endif
}

// Char cycle order for joystick entry.
static const char *validchars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789*[]! ";

static char nextchar(char c)
{
    const char *p = validchars;
    while( *p != c && *p != '\0') {
        ++p;
    }
    if (*p == '\0') {
        return validchars[0];
    }
    ++p;    // next char
    if (*p == '\0') {
        p = validchars; // wrapped.
    }
    return *p;
}

static char prevchar(char c)
{
    const char *p = validchars;
    while( *p != c && *p != '\0') {
        ++p;
    }
    if (p == validchars) {
        return ' '; // wrap
    }
    --p;
    return *p;
}

void tick_STATE_ENTERHIGHSCORE()
{
    bool finished = false;
    bool left = false;
    bool del = false;
    highscore* hs = &highscore_table[entry_slot];

    // Joystick editing
    {
        uint8_t inp = plat_inp_menu();
        if (inp & INP_MENU_START) {
            finished = true;
        }
        if (inp & (INP_LEFT | INP_MENU_B)) {
            del = true;
        }
        if (inp & (INP_RIGHT | INP_MENU_A)) {
            if (entry_cursor < HIGHSCORE_NAME_SIZE) {
                ++entry_cursor;
            } else {
                finished = true;
            }
        }

        if (inp & INP_DOWN) {
            if (entry_cursor < HIGHSCORE_NAME_SIZE) {
                hs->name[entry_cursor] = prevchar(hs->name[entry_cursor]);
            }
        }
        if (inp & INP_UP) {
            if (entry_cursor < HIGHSCORE_NAME_SIZE) {
                hs->name[entry_cursor] = nextchar(hs->name[entry_cursor]);
            }   
        }
    }

#ifdef PLAT_HAS_TEXTENTRY
    // Keyboard editing
    {
        char c = plat_textentry_getchar();
        if (c==0x7f) {
            del = true;
        } else if (c == '\b') {
            // backspace (non-destructive)
            del = true;
        } else if (c=='\n') {
            // enter
            finished = true;
        } else if (c>=32) {
            // printable ascii
            if (entry_cursor < HIGHSCORE_NAME_SIZE) {
                hs->name[entry_cursor] = c;
                ++entry_cursor;
            }
        }
    } 
#endif // PLAT_HAS_TEXTENTRY
    if (left && entry_cursor > 0) {
        // non-destructive backspace
        --entry_cursor;
    }
    if (del && entry_cursor > 0) {
        // delete
        if (entry_cursor < HIGHSCORE_NAME_SIZE) {
            hs->name[entry_cursor] = ' ';
        }
        --entry_cursor;
        hs->name[entry_cursor] = ' ';
    }

    if (finished) {
#ifdef PLAT_HAS_TEXTENTRY
        plat_textentry_stop();
#endif
        enter_STATE_HIGHSCORES();
    }
}

void render_STATE_ENTERHIGHSCORE()
{
    const uint8_t cx = (SCREEN_TEXT_W/2);
    plat_text(cx - 14, 1, "HIGH SCORE - ENTER YOUR NAME", 1);
    draw();
}


