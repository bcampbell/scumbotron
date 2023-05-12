#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include <stdbool.h>

#define STATE_TITLESCREEN 0
#define STATE_GETREADY 1        // pause, get ready
#define STATE_PLAY 2            // main gameplay
#define STATE_CLEARED 3         // level is cleared of bad dudes
#define STATE_KILLED 4          // player just died (but has another life)
#define STATE_GAMEOVER 5
#define STATE_HIGHSCORES 6      // show high scores
#define STATE_ENTERHIGHSCORE 7  // enter a new high score
#define STATE_GALLERY_BADDIES 8 // dude gallery screen
#define STATE_GALLERY_GOODIES 9 // hero gallery screen

extern uint8_t state;
extern uint8_t statetimer;

// In game.c:
void enter_STATE_TITLESCREEN();
void enter_STATE_GETREADY();
void enter_STATE_PLAY();
void enter_STATE_CLEARED();
void enter_STATE_KILLED();
void enter_STATE_GAMEOVER();
// In highscore.c:
void enter_STATE_HIGHSCORES();
void tick_STATE_HIGHSCORES();
void render_STATE_HIGHSCORES();
void enter_STATE_ENTERHIGHSCORE(uint32_t score);
void tick_STATE_ENTERHIGHSCORE();
void render_STATE_ENTERHIGHSCORE();
// In story.c
void enter_STATE_GALLERY_BADDIES();
void tick_STATE_GALLERY_BADDIES();
void render_STATE_GALLERY_BADDIES();
void enter_STATE_GALLERY_GOODIES();
void tick_STATE_GALLERY_GOODIES();
void render_STATE_GALLERY_GOODIES();
void enter_STATE_STORY_OHNO();
void tick_STATE_STORY_OHNO();
void render_STATE_STORY_OHNO();

#endif // GAME_H
