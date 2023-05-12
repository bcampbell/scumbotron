#include "plat.h"
#include "game.h"
#include "misc.h"

#define NUM_HIGHSCORES 10
#define HIGHSCORE_NAME_SIZE 3


static uint8_t entry_slot;
static uint8_t entry_cursor;

static char names[NUM_HIGHSCORES][HIGHSCORE_NAME_SIZE] = {
    {'A','C','E'}, {'A','C','E'}, {'A','C','E'}, {'A','C','E'},
    {'A','C','E'}, {'A','C','E'}, {'A','C','E'}, {'A','C','E'},
    {'A','C','E'}, {'A','C','E'},
};

static uint32_t scores[NUM_HIGHSCORES] = {10000,9000,8000,7000,6000,5000,4000,3000,2000,1000};


static void draw() {
    uint8_t slot;
    char buf[8];
    const uint8_t cx = (SCREEN_TEXT_W - (HIGHSCORE_NAME_SIZE + 2 + 8))/2; 
    const uint8_t cy = 3;
    for (slot = 0; slot<NUM_HIGHSCORES; ++slot) {
        uint8_t c = ((tick+slot)/2) & 0x0f;
        if (slot == entry_slot) {
            c = 1;
        }
        // Draw the name.
        plat_textn(cx, cy + slot*2, names[slot], HIGHSCORE_NAME_SIZE, c);
        if (slot == entry_slot) {
            // Flash the cursor.
            if ((tick & 0x3f) > 0x20) {
                plat_textn(cx + entry_cursor, cy + slot*2, &names[slot][entry_cursor], 1, 10);
            }
        }

        // Draw the score (8 digits).
        hex32(bin2bcd(scores[slot]), buf);
        plat_textn(cx + 2 + HIGHSCORE_NAME_SIZE, cy + slot*2, buf, 8, c );

    }
}



// returns position for new score, NUM_HIGHSCORES if not on score table.
static uint8_t pick_slot(uint32_t newscore)
{
    uint8_t slot;
    for (slot = 0; slot < NUM_HIGHSCORES; ++slot) {
        if (newscore > scores[slot]) {
            break;    // it's a highscore!
        }
    }
    return slot;
}

static void insert_new_score(uint8_t slot, uint32_t score)
{
    uint8_t j;
    uint8_t i = NUM_HIGHSCORES-1;

    for (i = NUM_HIGHSCORES - 1; i > slot; --i) {
        scores[i] = scores[i-1];
        for (j = 0; j < HIGHSCORE_NAME_SIZE; ++j) {
            names[i][j] = names[i-1][j];
        }
    }
    // Insert new score with clear name.
    scores[slot] = score;
    for (j = 0; j < HIGHSCORE_NAME_SIZE; ++j) {
        names[slot][j] = ' ';
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
    entry_slot = NUM_HIGHSCORES;
    entry_cursor = HIGHSCORE_NAME_SIZE;
    plat_clr();
}

void tick_STATE_HIGHSCORES()
{
    if (++statetimer > 200 || plat_inp_menu()) {
        enter_STATE_GALLERY_BADDIES();
    }
}

void render_STATE_HIGHSCORES()
{
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
    if (entry_slot >= NUM_HIGHSCORES) {
        // not a high score. bail out.
        enter_STATE_HIGHSCORES();
        return;
    }
    insert_new_score(entry_slot, score);
    names[entry_slot][0] = 'A';
}


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
    char c;
    uint8_t inp = plat_inp_menu();
    if ((inp & INP_MENU_ACTION || inp & INP_RIGHT) &&
        entry_cursor == (HIGHSCORE_NAME_SIZE - 1)) {
        enter_STATE_HIGHSCORES();
        return;
    }

    if (inp & INP_DOWN) {
        c = names[entry_slot][entry_cursor];
        names[entry_slot][entry_cursor] = nextchar(c);
    }
    if (inp & INP_UP) {
        c = names[entry_slot][entry_cursor];
        names[entry_slot][entry_cursor] = prevchar(c);
    }

    if (inp & INP_LEFT) {
        if (entry_cursor > 0) {
            names[entry_slot][entry_cursor] = ' ';
            --entry_cursor;
        }
    }
    if (inp & INP_RIGHT || inp & INP_MENU_ACTION) {
        if (entry_cursor < HIGHSCORE_NAME_SIZE - 1) {
            ++entry_cursor;
            names[entry_slot][entry_cursor] = 'A';
        }
    }
}

void render_STATE_ENTERHIGHSCORE()
{
    draw();
}


