#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include <stdbool.h>

#define STATE_ATTRACT 0
#define STATE_TITLESCREEN 1
// gameplay states
#define STATE_NEWGAME 2
#define STATE_GETREADY 3        // pause, get ready
#define STATE_PLAY 4            // main gameplay
#define STATE_CLEARED 5         // level is cleared of bad dudes
#define STATE_KILLED 6          // player just died (but has another life)
#define STATE_GAMEOVER 7
// highscore states
#define STATE_HIGHSCORES 8      // show high scores
#define STATE_ENTERHIGHSCORE 9  // enter a new high score
// gallery sequence
#define STATE_GALLERY_BADDIES 10 // dude gallery screen
#define STATE_GALLERY_GOODIES 11 // hero gallery screen
// story sequence
#define STATE_STORY_INTRO 12 // story part 1
#define STATE_STORY_OHNO 13 // story part 2
#define STATE_STORY_ATTACK 14 // story part 3
#define STATE_STORY_RUNAWAY 15 // story part 4
#define STATE_STORY_WHATNOW 16 // story part 5
#define STATE_STORY_DONE 17 // story finished
 
extern uint8_t state;
extern uint16_t statetimer;

// In game.c:
void enter_STATE_ATTRACT();
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
void enter_STATE_STORY_WHATNOW();
void tick_STATE_STORY_WHATNOW();
void render_STATE_STORY_WHATNOW();
void enter_STATE_STORY_DONE();
void tick_STATE_STORY_DONE();
void render_STATE_STORY_DONE();

#endif // GAME_H
