#include "../plat.h"
#include "../gob.h" // for ZAPPER_*
#include "../misc.h"
#include "plat_sdl2.h"
#include "vera_psg.h"

#include <SDL.h>

volatile uint8_t tick = 0;

extern unsigned char export_palette_bin[];
extern unsigned int export_palette_bin_len;
static SDL_Window* fenster = NULL;
static SDL_Renderer* renderer = NULL;

// screen is our virtual screen (8bit indexed)
SDL_Surface* screen = NULL;
// conversionSurface and screenTexture are used to get screen to
// the renderer.
static SDL_Surface* conversionSurface = NULL;
static SDL_Texture* screenTexture = NULL;

//
uint16_t screen_w;
uint16_t screen_h;

static bool quit = false;

static SDL_Palette *palette;
static char* scoreFile = NULL;


static void plat_render_start();
static void plat_render_finish();

static bool startup();
static void shutdown();
static bool screen_rethink();
static void pumpevents();

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

    // Set up palette
    {
        palette = SDL_AllocPalette(256);

        const uint8_t *src = export_palette_bin;
        SDL_Color colors[256];
        for (int i = 0; i < 16; ++i) {
            colors[i].r = *src++;
            colors[i].g = *src++;
            colors[i].b = *src++;
            colors[i].a = *src++;
        }
        // fill out unused colours.
        for (int i = 16; i < 256; ++i) {
            colors[i].r = 255;
            colors[i].g = 255;
            colors[i].b = 255;
            colors[i].a = 255;
        }
        SDL_SetPaletteColors(palette, colors, 0, 256);
    }

    //
    renderer = SDL_CreateRenderer(fenster, -1, SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        goto bailout;
    }

    if (!screen_rethink()) {
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
}


// Sets up screen on startup, or reconfigures screen when window is resized.
static bool screen_rethink()
{
    // Free old stuff if we're already running.
    SDL_FreeSurface(screen);
    screen = NULL;
    SDL_FreeSurface(conversionSurface); 
    conversionSurface = NULL;
    if (screenTexture) {
        SDL_DestroyTexture(screenTexture); 
        screenTexture = NULL;
    }

    // Peek at the window size to get the aspect ratio and use
    // that to decide our virtual screen size.
    int w,h;
    SDL_GetWindowSize(fenster, &w, &h);

    // could use floating point, but why start now?
    int aspect = (1024 * w) / h;

    // We'll clip aspect ratio to reasonable bounds.
    const int min_aspect = (1024 * 4)/3;    // 4:3 eg 640x480
    const int max_aspect = (1024 * 16)/9;   // 16:9 eg 1280x768
    if (aspect < min_aspect) {
        aspect = min_aspect;
    } else if (aspect > max_aspect) {
        aspect = max_aspect;
    }

    // We'll allow wider playfield, but always 240 high.
    screen_h = 240;
    screen_w = (512 + (screen_h * aspect)) / 1024;

    //printf("screen_rethink: window: %dx%d screen: %dx%d (aspect=%f)\n",w,h,screen_w, screen_h, (float)aspect/1024.0f);

    SDL_RenderSetLogicalSize(renderer, screen_w, screen_h);

    screenTexture = SDL_CreateTexture(renderer,
                               SDL_PIXELFORMAT_ARGB8888,
                               SDL_TEXTUREACCESS_STREAMING,
                               screen_w, screen_h);
    if (screenTexture == NULL) {
        goto bailout;
    }


    // Set up screen
    screen = SDL_CreateRGBSurfaceWithFormat(0, screen_w, screen_h, 8,
        SDL_PIXELFORMAT_INDEX8);
    if (screen == NULL) {
        goto bailout;
    }
    SDL_SetSurfacePalette(screen, palette);
    //SDL_SetColorKey(screen, SDL_TRUE, 0);


    // set up intermediate surface to convert to argb to update texture.
    conversionSurface = SDL_CreateRGBSurfaceWithFormat(0, screen_w, screen_h, 32,
                               SDL_PIXELFORMAT_ARGB8888);
    if (conversionSurface == NULL) {
        goto bailout;
    }
    return true;

bailout:
    return false;
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
              screen_rethink();
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




void plat_render_start()
{
    uint8_t i;
    // clear the screen
    memset(screen->pixels, 0, SCREEN_H*screen->pitch);
    // cycle colour 15
    i = (((tick >> 1)) & 0x7) + 2;
    SDL_SetPaletteColors(palette, &palette->colors[i], 15,1);
}

void plat_render_finish()
{

    // Convert screen to argb.
    SDL_BlitSurface(screen, NULL, conversionSurface, NULL);
    // Load into texture.
    SDL_UpdateTexture(screenTexture, NULL, conversionSurface->pixels, conversionSurface->pitch);
    SDL_SetRenderDrawColor(renderer, 32,32,32, 255);
    SDL_RenderClear(renderer);
    // Display screen.
    SDL_RenderCopy(renderer, screenTexture, NULL, NULL);
    SDL_RenderPresent(renderer);
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

        plat_render_start();
        game_render();
        mouse_render();
        plat_render_finish();

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


