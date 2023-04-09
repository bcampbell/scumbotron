#include "plat_sdl2.h"
#include "vera_psg.h"

#include <stdint.h>
#include <stdio.h>
//#include <string.h>
//#include <stdlib.h>
#include <SDL.h>

#define AUDIO_SAMPLERATE (25000000 / 512)

static SDL_AudioDeviceID audio_dev;

bool audio_init()
{
    audio_dev = 0;
	SDL_AudioSpec desired;
	SDL_AudioSpec obtained;

	// Setup SDL audio
	memset(&desired, 0, sizeof(desired));
	desired.freq     = AUDIO_SAMPLERATE;
	desired.format   = AUDIO_S16SYS;
    // At 48.8KHz, we need 813 samples per game frame. So make sure to eat the
    // the queued data in smaller chunks, so our audio_render() doesn't end
    // up ignoring frames.
	desired.samples  = 256;
	desired.channels = 2;
	desired.callback = NULL;

	audio_dev = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, 0);
	if (audio_dev <= 0) {
		fprintf(stderr, "SDL_OpenAudioDevice failed: %s\n", SDL_GetError());
        return false;
    }
	// Start playback
	SDL_PauseAudioDevice(audio_dev, 0);
    //printf("Audio %d\n", audio_dev);
    return true;
}

void audio_close(void)
{
	if (audio_dev == 0) {
		return;
	}

	SDL_CloseAudioDevice(audio_dev);
	audio_dev = 0;
}


// To be called once per game frame, to generate sound data.
void audio_render()
{
	if (audio_dev == 0) {
		return;
	}
    // Try and keep a few (game) frames worth of data queued up.
    const uint32_t sampleFramesDesired = 3 * (AUDIO_SAMPLERATE / TARGET_FPS);

    Uint32 bytesQueued = SDL_GetQueuedAudioSize(audio_dev);
    uint32_t sampleFramesQueued = bytesQueued / (sizeof(int16_t) * 2);

    if (sampleFramesQueued > sampleFramesDesired) {
        return;
    }
    uint32_t nframes = sampleFramesDesired - sampleFramesQueued;

    #define MAX_BUF_FRAMES 2048
    uint8_t buf[MAX_BUF_FRAMES * sizeof(int16_t) * 2];
    //printf("q: %d/%d nframes: %d\n", (int)sampleFramesQueued, (int)sampleFramesDesired, (int)nframes);
    while (nframes > 0) {
        uint64_t n = (nframes > MAX_BUF_FRAMES) ? MAX_BUF_FRAMES : nframes;
        psg_render((int16_t*)buf, n);
        int r = SDL_QueueAudio(audio_dev, buf, 2 * n * sizeof(int16_t));
        if (r < 0) {
		    fprintf(stderr, "SDL_QueueAudio failed: %s\n", SDL_GetError());
        }
        nframes -= n;
    }
    //printf("]\n");
}

