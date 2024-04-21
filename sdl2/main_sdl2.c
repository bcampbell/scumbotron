#include "../plat.h"
#include "plat_sdl2.h"
#include "vera_psg.h"

#include <SDL.h>

volatile uint8_t tick = 0;

static SDL_Window* fenster = NULL;
static bool quit = false;
static char* scoreFile = NULL;

static bool startup();
static void shutdown();
static void pumpevents();

// from gfx_sdl2.c:
bool gfx_init(SDL_Window* fenster);
bool gfx_screen_rethink(int w, int h);
bool gfx_shutdown();
bool gfx_render_start();
bool gfx_render_finish();
extern SDL_Renderer* renderer;

// from input_sdl2.c:
void controller_init();
void controller_added(int id);
void controller_removed(SDL_JoystickID joyid);
void controller_shutdown();
void mouse_update(SDL_Renderer* rr);
void mouse_render();
#ifdef PLAT_HAS_TEXTENTRY
void textentry_put(char c);
#endif // PLAT_HAS_TEXTENTRY


void plat_gatso(uint8_t t)
{
}

static bool startup()
{
    Uint32 flags = SDL_WINDOW_RESIZABLE;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0) {
        goto bailout;
    }

    // sort out the name for the high score file
    {
        scoreFile = NULL;
        char* path = SDL_GetPrefPath(NULL, "scumbotron");
        if (path) {
            size_t n = strlen(path);
            path = SDL_realloc(path, n + 5);    // + "score"
            if (path) {
                strcpy(path + n, "score");
                scoreFile = path;
            }
        }
    }

//    if (fullscreen)
    flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    fenster = SDL_CreateWindow("Scumbotron",
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              640, 480, flags);
    if (!fenster) {
        goto bailout;
    }
    SDL_ShowCursor(SDL_DISABLE);

    if (!gfx_init(fenster)) {
        goto bailout;
    }

    psg_reset();
    if (!audio_init()) {
        goto bailout;
    }

    controller_init(); 
    return true;

bailout:
    fprintf(stderr, "Init failed: %s\n", SDL_GetError());
    return false;
}

static void shutdown()
{
    controller_shutdown(); 
    if (scoreFile) {
        SDL_free(scoreFile);
        scoreFile = NULL;
    }
    gfx_shutdown();
}

static void pumpevents()
{
    SDL_PumpEvents();
    SDL_Event event;
    while (SDL_PollEvent(&event) == 1) {
      switch (event.type) {
        case SDL_QUIT:
          quit = true;
          return;
        case SDL_KEYDOWN:
          switch (event.key.keysym.sym) {
            case SDLK_F11:
              if (SDL_GetWindowFlags(fenster) & SDL_WINDOW_FULLSCREEN_DESKTOP) {
                SDL_SetWindowFullscreen(fenster, 0);
              } else {
                SDL_SetWindowFullscreen(fenster, SDL_WINDOW_FULLSCREEN_DESKTOP);
              }
              break;
#ifdef PLAT_HAS_TEXTENTRY
            case SDLK_RETURN:
              if (SDL_IsTextInputActive()) {
                  textentry_put('\n');
              }
              break;
            case SDLK_BACKSPACE:
              if (SDL_IsTextInputActive()) {
                  textentry_put(0x7f);
              }
              break;
            case SDLK_LEFT:
              if (SDL_IsTextInputActive()) {
                  textentry_put('\b');  // nondestructive backspace
              }
              break;
#endif //PLAT_HAS_TEXTENTRY
            default:
              //KeyDown(event.key);
              break;
          }
          break;
        case SDL_WINDOWEVENT:
          {
            if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
              int w,h;
              SDL_GetWindowSize(fenster, &w, &h);
              gfx_screen_rethink(w, h); // TODO: handle failure?
            }
          }
          break;
#ifdef PLAT_HAS_TEXTENTRY
        case SDL_TEXTINPUT:
          {
            for (char *p = event.text.text; *p; ++p) {
                // Strip non-ascii chars from the utf-8.
                if ((*p & 0x80) == 0) {
                    textentry_put(*p);
                }
            }
          }
          break;
#endif // PLAT_HAS_TEXTENTRY
        case SDL_CONTROLLERDEVICEADDED:
          controller_added((int)event.cdevice.which);
          break;
        case SDL_CONTROLLERDEVICEREMOVED:
          controller_removed((SDL_JoystickID)event.cdevice.which);
          break;
      }
    }
}



bool plat_savescores(const void* begin, int nbytes)
{
    if (!scoreFile) {
        return false;
    }
    SDL_RWops* f = SDL_RWFromFile(scoreFile, "wb");
    if (!f) {
        return false;
    }
    // TODO: should write to temp file then rename.
    size_t n = SDL_RWwrite(f, begin, 1, nbytes);
    SDL_RWclose(f);
    return (n == nbytes);
}

bool plat_loadscores(void* begin, int nbytes)
{
    if (!scoreFile) {
        return false;
    }
    SDL_RWops* f = SDL_RWFromFile(scoreFile, "rb");
    if (!f) {
        return false;
    }
    // TODO: should read via a temp buffer
    size_t n = SDL_RWread(f, begin, 1, nbytes);
    SDL_RWclose(f);
    return (n == nbytes);
}

void plat_quit()
{
    quit = true;
}

int main(int argc, char* argv[]) {
    if (!startup()) {
        shutdown();
        return 1;
    }
    game_init();

    uint64_t starttime = SDL_GetTicks64();
    int frame = 0;
    quit = false;
    while(1) {
        pumpevents();
        if (quit) {
            break;
        }
        // Renderer needed for coord mapping.
        mouse_update(renderer);

        gfx_render_start();
        game_render();
        mouse_render();
        gfx_render_finish();

        game_tick();
        ++tick; // The 8bit byte ticker.

        audio_render();

        ++frame;

        uint64_t now = SDL_GetTicks64();
        uint64_t target = starttime + ((frame*1000) / TARGET_FPS);
        int64_t delta = target - now;
        if (delta > 0) {
            SDL_Delay(delta);
        }

    }
    shutdown();
    return 0;
}


