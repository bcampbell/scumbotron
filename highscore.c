#include "sys.h"
#include "game.h"
#include "misc.h"

#define NUM_HIGHSCORES 10
#define HIGHSCORE_NAME_SIZE 3


static uint8_t entry_slot;
static uint8_t entry_cursor;

static char names[NUM_HIGHSCORES][HIGHSCORE_NAME_SIZE] = {
    {'a','c','e'},
    {'a','c','e'},
    {'a','c','e'},
    {'a','c','e'},
    {'a','c','e'},
    {'a','c','e'},
    {'a','c','e'},
    {'a','c','e'},
    {'a','c','e'},
    {'a','c','e'}
};

static uint32_t scores[NUM_HIGHSCORES] = {9000000,8000000,70000,60000,50000,40000,30000,20000,10000,0};


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
        sys_textn(cx, cy + slot*2, names[slot], HIGHSCORE_NAME_SIZE, c);
        if (slot == entry_slot) {
            // Flash the cursor.
            if (tick & 0x08) {
                sys_textn(cx + entry_cursor, cy + slot*2, "*", 1, 15);
            }
        }

        // Draw the score (8 digits).
        hex32(bin2bcd(scores[slot]), buf);
        sys_textn(cx + 2 + HIGHSCORE_NAME_SIZE, cy + slot*2, buf, 8, c );

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
    sys_clr();
}

void tick_STATE_HIGHSCORES()
{
    if (++statetimer > 200 || sys_inp_menu()) {
        enter_STATE_TITLESCREEN();
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
    sys_clr();
    entry_slot = pick_slot(score);
    entry_cursor = 0;
    if (entry_slot >= NUM_HIGHSCORES) {
        // not a high score. bail out.
        enter_STATE_HIGHSCORES();
        return;
    }
    insert_new_score(entry_slot, score);
}

void tick_STATE_ENTERHIGHSCORE()
{
    uint8_t inp = sys_inp_menu();
    if (inp & INP_MENU_ACTION) {
        enter_STATE_HIGHSCORES();
        return;
    }


    if (inp & INP_UP) {
        --names[entry_slot][entry_cursor];
    }
    if (inp & INP_DOWN) {
        ++names[entry_slot][entry_cursor];
    }

    if (inp & INP_LEFT) {
        if (entry_cursor > 0) {
            --entry_cursor;
        }
    }
    if (inp & INP_RIGHT) {
        if (entry_cursor < HIGHSCORE_NAME_SIZE - 1) {
            ++entry_cursor;
        }
    }
}

void render_STATE_ENTERHIGHSCORE()
{
    draw();
}


