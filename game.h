#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include <stdbool.h>

#define STATE_TITLESCREEN 0
#define STATE_NEWGAME 1
#define STATE_GETREADY 2        // pause, get ready
#define STATE_PLAY 3            // main gameplay
#define STATE_CLEARED 4         // level is cleared of bad dudes
#define STATE_KILLED 5          // player just died (but has another life)
#define STATE_GAMEOVER 6
#define STATE_HIGHSCORES 7      // show high scores
#define STATE_ENTERHIGHSCORE 8  // enter a new high score
#define STATE_GALLERY_BADDIES 9 // dude gallery screen
#define STATE_GALLERY_GOODIES 10 // hero gallery screen
#define STATE_STORY_INTRO 11 // story part 1
#define STATE_STORY_OHNO 12 // story part 2
#define STATE_STORY_ATTACK 13 // story part 3
#define STATE_STORY_RUNAWAY 14 // story part 4
#define STATE_STORY_DONE 15 // story finished
 
extern uint8_t state;
extern uint16_t statetimer;

// In game.c:
void enter_STATE_TITLESCREEN();
void enter_STATE_NEWGAME();
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
void enter_STATE_STORY_INTRO();
void tick_STATE_STORY_INTRO();
void render_STATE_STORY_INTRO();
void enter_STATE_STORY_OHNO();
void tick_STATE_STORY_OHNO();
void render_STATE_STORY_OHNO();
void enter_STATE_STORY_ATTACK();
void tick_STATE_STORY_ATTACK();
void render_STATE_STORY_ATTACK();
void enter_STATE_STORY_RUNAWAY();
void tick_STATE_STORY_RUNAWAY();
void render_STATE_STORY_RUNAWAY();
void enter_STATE_STORY_DONE();
void tick_STATE_STORY_DONE();
void render_STATE_STORY_DONE();

#endif // GAME_H
